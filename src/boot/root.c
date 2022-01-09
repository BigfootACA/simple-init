/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<linux/loop.h>
#ifdef ENABLE_BLKID
#include<blkid/blkid.h>
#endif
#include"str.h"
#include"boot.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"init_internal.h"
#define TAG "switchroot"
#define EGOTO(_num){e=(_num);goto fail;}
static blkid_cache cache=NULL;

static char*get_block(char*path,int wait){
	struct stat st;
	char*block=strdup(path);
	if(!block)EPRET(ENOMEM);

	// wait root block
	if(wait_block(block,wait,TAG)<0){
		telog_error("wait rootfs block %s",block);
		free(block);
		return NULL;
	}

	// resolve root block tag
	#ifdef ENABLE_BLKID
	if(block[0]!='/'){
		char*x=blkid_evaluate_tag(block,NULL,NULL);
		free(block);
		if(!x){
			telog_error("resolve tag %s",path);
			return NULL;
		}
		block=x;
	}
	#endif

	// stat root block
	if(stat(block,&st)<0){
		telog_error("stat rootfs block %s",block);
		free(block);
		return NULL;
	}

	// root block is a block device
	if(!S_ISBLK(st.st_mode)){
		telog_error("rootfs block %s is not a block",block);
		free(block);
		errno=ENOTBLK;
		return NULL;
	}

	return block;
}

static char*get_fstype(boot_config*boot,const char*path,char*key,char*buf,size_t len){
	char*t1=NULL,*t2=NULL,mod[64];
	memset(buf,0,len);
	memset(mod,0,sizeof(mod));
	if(key&&(t1=kvarr_get(boot->data,key,NULL)))goto success;

	#ifdef ENABLE_BLKID
	// fstype not set, auto detect
	errno=0;
	if(!cache)blkid_get_cache(&cache,NULL);
	if((t2=blkid_get_tag_value(cache,"TYPE",path)))goto success;
	else telog_warn("cannot determine fstype in %s",path);
	#endif

	EPRET(ENOTSUP);
	success:
	strncpy(buf,t1?t1:t2,len-1);
	if(t2)free(t2);
	#ifdef ENABLE_KMOD
	// load modules for filesystem
	snprintf(mod,63,"fs-%s",buf);
	insmod(mod,false);
	#endif
	errno=0;
	return buf;
}

static char*get_flags(boot_config*boot,char*key,bool ro,char*buf,size_t len){
	memset(buf,0,len);
	char*flags=kvarr_get(boot->data,key,NULL);
	buf[0]='r',buf[1]=ro?'o':'w';
	if(flags)snprintf(buf+2,len-3,",%s",flags);
	return buf;
}

static int setup_loop(boot_config*boot,bool ro,char*path,size_t len){
	size_t bs;
	uint64_t offset=0;
	struct stat st;
	struct loop_info64 li;
	int loop_fd=-1,img_fd=-1;
	int e=-1,partno=0,sector=0;
	char*loop,*part,*sec,*off;
	char blk[256],point[256],pblk[256],fstype[64];
	char buf[PATH_MAX],image[PATH_MAX],flags[PATH_MAX];

	if(!boot||!path||!*path)return -1;
	if(!(loop=kvarr_get(boot->data,"loop",NULL))||!*loop)return 0;
	if(*loop=='/')loop++;
	if((sec=kvarr_get(boot->data,"loop_sector",NULL)))
		if((sector=parse_int(sec,0))<=0)
			EGOTO(trlog_error(ENUM(EINVAL),"invalid sector size"));
	if((part=kvarr_get(boot->data,"loop_partno",NULL)))
		if((partno=parse_int(part,0))<=0)
			EGOTO(trlog_error(ENUM(EINVAL),"invalid partition number"));
	if((off=kvarr_get(boot->data,"loop_offset",NULL)))
		if((offset=parse_long(off,0))<=0)
			EGOTO(trlog_error(ENUM(EINVAL),"invalid loop offset"));

	if((bs=strlen(path))>=255)return -1;
	if(path[bs-1]!='/')path[bs]='/',path[bs+1]=0;

	snprintf(buf,sizeof(buf)-1,"%s%s",path,loop);
	if(!realpath(buf,image))EGOTO(terlog_error(-errno,"realpath failed"));

	// check image
	if(stat(image,&st)!=0)EGOTO(terlog_error(-errno,"stat %s failed",image));
	if(!S_ISREG(st.st_mode))EGOTO(trlog_error(ENUM(ENOTSUP),"%s is not a file",image));
	if(st.st_size<=0)EGOTO(trlog_error(ENUM(EINVAL),"%s is an empty file",image));

	// get a free loop device
	#ifdef ENABLE_KMOD
	insmod("loop",false);
	#endif
	if(loop_get_free(blk,sizeof(blk))<0)
		EGOTO(terlog_error(-errno,"get free loop failed"));

	// open device and image file
	if((loop_fd=open(blk,O_CLOEXEC|O_RDWR))<0)
		EGOTO(terlog_error(-errno,"open loop %s failed",loop));
	if((img_fd=open(image,O_CLOEXEC|(ro?O_RDONLY:O_RDWR)))<0)
		EGOTO(terlog_error(-errno,"open image %s failed",image));

	// configure loop
	if(ioctl(loop_fd,LOOP_SET_FD,img_fd)!=0)
		EGOTO(terlog_error(-errno,"associate loop failed"));
	memset(&li,0,sizeof(li));
	li.lo_offset=offset;
	li.lo_flags=LO_FLAGS_AUTOCLEAR;
	if(ro)li.lo_flags|=LO_FLAGS_READ_ONLY;
	if(partno>0)li.lo_flags|=LO_FLAGS_PARTSCAN;
	strncpy((char*)li.lo_file_name,image,sizeof(li.lo_file_name)-1);
	if(ioctl(loop_fd,LOOP_SET_STATUS,&li)!=0)
		EGOTO(terlog_error(-errno,"configure loop failed"));
	if(sector>0&&ioctl(loop_fd,LOOP_SET_BLOCK_SIZE,sector)!=0)
		EGOTO(terlog_error(-errno,"set loop sector size failed"));
	if(partno<=0)strncpy(pblk,blk,sizeof(pblk)-1);
	else snprintf(pblk,sizeof(pblk)-1,"%sp%d",blk,partno);

	if(!(get_fstype(boot,pblk,"loop_fstype",fstype,sizeof(fstype))))
		EGOTO(terlog_error(-errno,"cannot get fstype"));
	get_flags(boot,"loop_flags",ro,flags,sizeof(flags));

	// get a mountpoint
	if(!auto_mountpoint(point,sizeof(point)))
		EGOTO(terlog_error(-errno,"cannot get new mountpoint"));
	if(xmount(false,pblk,point,fstype,flags,true)!=0)EGOTO(-errno);

	// change root path
	memset(path,0,len);
	strncpy(path,point,len-1);

	return 0;
	fail:
	if(loop_fd>0){
		ioctl(loop_fd,LOOP_CLR_FD);
		close(loop_fd);
	}
	if(img_fd>0)close(img_fd);
	if(errno==0)errno=e==0?ENOTSUP:0;
	if(e==0)e=-1;
	return e;
}

int run_boot_root(boot_config*boot){
	if(!boot)ERET(EINVAL);
	if(boot->mode!=BOOT_SWITCHROOT)ERET(ENOTSUP);

	int e,wait;
	char*definit,*init,*path,flags[PATH_MAX],point[256],type[64];
	bool ro=parse_int(kvarr_get(boot->data,"rw","0"),0)==0;

	// unimportant variables
	definit=kvarr_get(boot->data,"init",NULL);
	wait=parse_int(kvarr_get(boot->data,"wait",NULL),5);
	if(wait<0)wait=5;

	// root block path must set
	if(!(path=kvarr_get(boot->data,"path",NULL)))
		EGOTO(trlog_error(ENUM(EINVAL),"rootfs block path not set"));

	if(!(path=get_block(path,wait)))EGOTO(-errno);
	if(!(get_fstype(boot,path,"fstype",type,sizeof(type))))
		EGOTO(terlog_error(-errno,"cannot get fstype"));
	get_flags(boot,"flags",ro,flags,sizeof(flags));

	// get a mountpoint
	if(!auto_mountpoint(point,sizeof(point)))
		EGOTO(terlog_error(-errno,"cannot get new mountpoint"));

	// mount root block
	if(xmount(false,path,point,type,flags,true)!=0)EGOTO(-errno);

	// setup loop root
	if(setup_loop(boot,ro,point,sizeof(point))!=0)EGOTO(-errno);

	// try to search init
	if(!(init=search_init(definit,point)))EGOTO(-errno);
	tlog_info("found init %s in %s",init,point);

	// execute switchroot
	errno=0;
	struct init_msg msg,response;
	size_t s1=sizeof(msg.data.newroot.root);
	size_t s2=sizeof(msg.data.newroot.init);
	if(strlen(point)>=s1||strlen(init)>=s2)
		EGOTO(terlog_error(ENAMETOOLONG,"argument too long"));
	init_initialize_msg(&msg,ACTION_SWITCHROOT);
	strncpy(msg.data.newroot.root,point,s1-1);
	strncpy(msg.data.newroot.init,init,s2-1);
	init_send(&msg,&response);
	if(errno!=0)telog_warn("send command");
	if(response.data.status.ret!=0)tlog_warn(
		"switchroot failed: %s",
		strerror(response.data.status.ret)
	);
	e=response.data.status.ret;

	done:
	if(path)free(path);
	#ifdef ENABLE_BLKID
	if(cache)blkid_put_cache(cache);
	#endif
	return e;
	fail:
	if(errno==0)errno=e==0?ENOTSUP:0;
	if(e==0)e=-1;
	goto done;
}

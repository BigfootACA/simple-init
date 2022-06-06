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
#include<blkid/blkid.h>
#include"str.h"
#include"boot.h"
#include"confd.h"
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
	if(block[0]!='/'){
		char*x=blkid_evaluate_tag(block,NULL,NULL);
		free(block);
		if(!x){
			telog_error("resolve tag %s",path);
			return NULL;
		}
		block=x;
	}

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
	if(key&&(t1=confd_get_string_base(boot->key,key,NULL)))goto success;

	// fstype not set, auto detect
	errno=0;
	if(!cache)blkid_get_cache(&cache,NULL);
	if((t2=blkid_get_tag_value(cache,"TYPE",path)))goto success;
	else telog_warn("cannot determine fstype in %s",path);

	EPRET(ENOTSUP);
	success:
	strncpy(buf,t1?t1:t2,len-1);
	if(t2)free(t2);
	// load modules for filesystem
	snprintf(mod,63,"fs-%s",buf);
	insmod(mod,false);
	errno=0;
	return buf;
}

static char*get_flags(boot_config*boot,char*key,bool ro,char*buf,size_t len){
	memset(buf,0,len);
	char*flags=confd_get_string_base(boot->key,key,NULL);
	buf[0]='r',buf[1]=ro?'o':'w';
	if(flags)snprintf(buf+2,len-3,",%s",flags);
	return buf;
}

static int setup_loop(boot_config*boot,bool ro,char*path,size_t len){
	uint64_t offset=0;
	struct stat st;
	struct loop_info64 li;
	int loop_fd=-1,img_fd=-1;
	int e=-1,partno=0,sector=0;
	char*loop;
	char blk[256],point[256],pblk[256],fstype[64];
	char buf[PATH_MAX],image[PATH_MAX],flags[PATH_MAX];

	if(!boot||!path||!*path)return -1;
	if(!(loop=confd_get_string_base(boot->key,"loop",NULL))||!*loop)return 0;
	if(strcasecmp(loop,"none")==0)return 0;
	if(*loop=='/')loop++;
	if((sector=confd_get_integer_base(boot->key,"loop_sector",0))<=0)
		EGOTO(trlog_error(ENUM(EINVAL),"invalid sector size"));
	if((partno=confd_get_integer_base(boot->key,"loop_partno",0))<=0)
		EGOTO(trlog_error(ENUM(EINVAL),"invalid partition number"));
	if((offset=confd_get_integer_base(boot->key,"loop_offset",0))<=0)
		EGOTO(trlog_error(ENUM(EINVAL),"invalid loop offset"));

	if(!add_right_slash(path,len))return -1;

	snprintf(buf,sizeof(buf)-1,"%s%s",path,loop);
	if(!realpath(buf,image))EGOTO(terlog_error(-errno,"realpath failed"));

	// check image
	if(stat(image,&st)!=0)EGOTO(terlog_error(-errno,"stat %s failed",image));
	if(!S_ISREG(st.st_mode))EGOTO(trlog_error(ENUM(ENOTSUP),"%s is not a file",image));
	if(st.st_size<=0)EGOTO(trlog_error(ENUM(EINVAL),"%s is an empty file",image));
	tlog_info("use loop image %s (size %zu)",loop,st.st_size);

	// get a free loop device
	insmod("loop",false);
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

static int setup_overlay(boot_config*boot,int wait,char*path,size_t len){
	int e=0;
	char*data,*data_blk=NULL,*size,*prefix;
	char overlay_point[256],overlay_flags[PATH_MAX];
	char data_type[64],data_point[256],data_flags[PATH_MAX];
	char upper_path[PATH_MAX],work_path[PATH_MAX],data_path[PATH_MAX];

	if(!boot||!path||!*path)return -1;
	if(!(data=confd_get_string_base(boot->key,"data",NULL))||!*data)return 0;
	if(strcasecmp(data,"none")==0)return 0;
	prefix=trim_slash(confd_get_string_base(boot->key,"data_prefix",NULL));
	size=confd_get_string_base(boot->key,"data_size",NULL);
	if(size&&!*size)size=NULL;

	// generate data flags
	memset(data_path,0,sizeof(data_path));
	size_t fs,fb=sizeof(data_flags);
	get_flags(boot,"data_flags",false,data_flags,fb);
	if(strcmp(data,"tmpfs")==0){
		strcpy(data_type,"tmpfs");
		if(!(data_blk=strdup(data_type)))EGOTO(-1);
		fs=strlen(data_flags);
		snprintf(data_flags+fs,fb-fs-1,",size=%s,mode=0755",size?size:"50%");
	}else{
		if(!(data_blk=get_block(data,wait)))EGOTO(-errno);
		if(!(get_fstype(boot,data_blk,"data_fstype",data_type,sizeof(data_type))))
			EGOTO(terlog_error(-errno,"cannot get fstype"));
	}
	tlog_info("use %s(%s) as overlay data block",data_blk,data_type);

	// mount data partition
	if(!auto_mountpoint(data_point,sizeof(data_point)))
		EGOTO(terlog_error(-errno,"cannot get new mountpoint"));
	if(xmount(false,data_blk,data_point,data_type,data_flags,true)!=0)EGOTO(-errno);

	// init overlay data partition
	memset(data_path,0,sizeof(data_path));
	if(!prefix)strncpy(data_path,data_point,sizeof(data_path)-1);
	else{
		snprintf(data_path,sizeof(data_path)-1,"%s/%s",data_point,prefix);
		mkdir(data_path,0700);
		if(!is_folder(data_path))
			EGOTO(terlog_error(-errno,"data %s is not a folder",data_path));
	}
	memset(work_path,0,sizeof(work_path));
	memset(upper_path,0,sizeof(upper_path));
	snprintf(work_path,sizeof(work_path)-1,"%s/work",data_path);
	snprintf(upper_path,sizeof(upper_path)-1,"%s/upper",data_path);
	mkdir(work_path,0700);
	mkdir(upper_path,0700);
	if(!is_folder(work_path))EGOTO(terlog_error(-errno,"work %s is not a folder",work_path));
	if(!is_folder(upper_path))EGOTO(terlog_error(-errno,"upper %s is not a folder",upper_path));

	// generate overlay flags
	memset(overlay_flags,0,sizeof(overlay_flags));
	snprintf(
		overlay_flags,sizeof(overlay_flags)-1,
		"rw,lowerdir=%s,upperdir=%s,workdir=%s",
		path,upper_path,work_path
	);

	// mount overlay
	insmod("fs-overlay",false);
	if(!auto_mountpoint(overlay_point,sizeof(overlay_point)))
		EGOTO(terlog_error(-errno,"cannot get new mountpoint"));
	if(xmount(
		false,confd_get_string_base(boot->key,"overlay_name","rootfs"),
		overlay_point,"overlay",overlay_flags,true
	)!=0)EGOTO(-errno);

	// change root path
	memset(path,0,len);
	strncpy(path,overlay_point,len-1);

	return 0;
	fail:
	if(data_blk)free(data_blk);
	if(errno==0)errno=e==0?ENOTSUP:0;
	if(e==0)e=-1;
	return e;
}

int run_boot_root(boot_config*boot){
	if(!boot)ERET(EINVAL);
	if(boot->mode!=BOOT_SWITCHROOT)ERET(ENOTSUP);

	bool ro=true;
	int e,wait=5;
	char*definit,*init,*path=NULL,*b;
	char flags[PATH_MAX],point[256],type[64];
	ro=confd_get_integer_base(boot->key,"rw",0)==0;

	// unimportant variables
	definit=confd_get_string_base(boot->key,"init",NULL);
	wait=confd_get_integer_base(boot->key,"wait",5);
	if(wait<0)wait=5;

	// root block path must set
	if(!(path=confd_get_string_base(boot->key,"path",NULL)))
		EGOTO(trlog_error(ENUM(EINVAL),"rootfs block path not set"));

	if(!(b=get_block(path,wait)))EGOTO(-errno);
	free(path);
	path=b;
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

	// setup overlay fs
	if(setup_overlay(boot,wait,point,sizeof(point))!=0)EGOTO(-errno);

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
	if(cache)blkid_put_cache(cache);
	return e;
	fail:
	if(errno==0)errno=e==0?ENOTSUP:0;
	if(e==0)e=-1;
	goto done;
}

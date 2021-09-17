#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<sys/stat.h>
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

int run_boot_root(boot_config*boot){
	if(!boot)ERET(EINVAL);
	if(boot->mode!=BOOT_SWITCHROOT)ERET(ENOTSUP);

	int e,wait;
	char*definit,*init,*path,*type,*xflags,flags[PATH_MAX],point[256];
	bool ro=parse_int(kvarr_get(boot->data,"rw","0"),0)==0;

	// unimportant variables
	definit=kvarr_get(boot->data,"init",NULL);
	xflags=kvarr_get(boot->data,"flags",NULL);
	type=kvarr_get(boot->data,"type",NULL);
	wait=parse_int(kvarr_get(boot->data,"wait",NULL),5);
	if(wait<0)wait=5;

	// root block path must set
	if(!(path=kvarr_get(boot->data,"path",NULL)))
		return trlog_error(ENUM(EINVAL),"rootfs block path not set");

	if(!(path=get_block(path,wait)))return -errno;

	// apppend rw/ro and flags
	flags[0]='r',flags[1]=ro?'o':'w';
	if(xflags)snprintf(flags+2,PATH_MAX-3,",%s",xflags);
	else flags[2]=0;

	#ifdef ENABLE_BLKID
	// fstype not set, auto detect
	blkid_cache cache=NULL;
	if(!type){
		blkid_get_cache(&cache,NULL);
		if(!(type=blkid_get_tag_value(cache,"TYPE",path)))
			telog_warn("cannot determine fstype in %s",path);

	}
	#endif

	#ifdef ENABLE_KMOD
	// load modules for filesystem
	if(type){
		char mod[64]={0};
		snprintf(mod,63,"fs-%s",type);
		insmod(mod,false);
	}
	#endif

	// get a mountpoint
	if(!auto_mountpoint(point,256)){
		e=terlog_error(-errno,"cannot get new mountpoint");
		goto ex;
	}

	// mount root block
	if(xmount(false,path,point,type,flags,true)!=0){
		e=-errno;
		free(path);
		goto ex;
	}
	free(path);

	// try to search init
	if(!(init=search_init(definit,point)))return -errno;
	tlog_info("found init %s in %s",init,point);

	// execute switchroot
	struct init_msg msg,response;
	size_t s1=sizeof(msg.data.newroot.root);
	size_t s2=sizeof(msg.data.newroot.init);
	if(strlen(point)>=s1||strlen(init)>=s2){
		e=terlog_error(ENAMETOOLONG,"argument too long");
		goto ex;
	}
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

	ex:
	#ifdef ENABLE_BLKID
	if(cache){
		if(type)free(type);
		blkid_put_cache(cache);
	}
	#endif
	return e;
}

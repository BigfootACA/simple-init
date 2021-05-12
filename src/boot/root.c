#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<sys/stat.h>
#include<blkid/blkid.h>
#include"str.h"
#include"boot.h"
#include"init.h"
#include"logger.h"
#include"system.h"
#include"cmdline.h"
#include"defines.h"
#define TAG "switchroot"

bool check_init(bool force,char*root,char*init){
	static char path[PATH_MAX];
	struct stat st;
	if(!root||!init)return false;

	// init must start with /
	if(init[0]!='/'){
		tlog_error("invalid init %s",init);
		return false;
	}

	// generate init realpath
	memset(path,0,PATH_MAX);
	snprintf(path,PATH_MAX-1,"%s%s",root,init);

	// stat init
	if(stat(path,&st)<0){
		if(errno!=ENOENT||force)telog_warn("stat init %s",path);
		return false;
	}

	// init is a normal file
	if(!S_ISREG(st.st_mode)){
		tlog_error("init %s is not a file",path);
		return false;
	}

	// init has execute permission
	if(!(st.st_mode&S_IXUSR)){
		tlog_error("init %s is not an executable file",path);
		return false;
	}

	return true;
}

char*search_init(char*root){
	if(!root)EPRET(EINVAL);

	// init specified in kernel cmdline
	char*init=boot_options.init;
	if(init)return check_init(true,root,init)?init:NULL;

	// search init in list
	for(int i=0;(init=init_list[i]);i++){
		if(!check_init(false,root,init))continue;
		tlog_info("found init %s in %s",init,root);
		return init;
	}

	// no any init found
	tlog_error("no init found in %s",root);
	EPRET(ENOENT);
}

int run_boot_root(boot_config*boot){
	if(!boot)ERET(EINVAL);
	if(boot->mode!=BOOT_SWITCHROOT)ERET(ENOTSUP);

	int e;
	struct stat st;
	char*init,*path,*type,*xflags,flags[PATH_MAX],point[256];
	bool ro=parse_int(kvarr_get(boot->data,"rw","0"),0)==0;
	blkid_cache cache=NULL;

	// unimportant variables
	xflags=kvarr_get(boot->data,"flags",NULL);
	type=kvarr_get(boot->data,"type",NULL);

	// root block path must set
	if(!(path=kvarr_get(boot->data,"path",NULL)))
		return trlog_error(ENUM(EINVAL),"rootfs block path not set");

	// stat root block
	if(stat(path,&st)<0)
		return terlog_error(errno,"stat rootfs block %s",path);

	// root block is a block device
	if(!S_ISBLK(st.st_mode))return terlog_error(
		ENUM(ENOTBLK),
		"rootfs block %s is not a block",
		path
	);

	// apppend rw/ro and flags
	flags[0]='r',flags[1]=ro?'o':'w';
	if(xflags)snprintf(flags+2,PATH_MAX-3,",%s",xflags);
	else flags[2]=0;

	// fstype not set, auto detect
	if(!type){
		blkid_get_cache(&cache,NULL);
		if(!(type=blkid_get_tag_value(cache,"TYPE",path)))
			telog_warn("cannot determine fstype in %s",path);
	}

	// load modules for filesystem
	if(type){
		char mod[64]={0};
		snprintf(mod,63,"fs-%s",type);
		insmod(mod,false);
	}

	// get a mountpoint
	if(!auto_mountpoint(point,256)){
		e=terlog_error(-errno,"cannot get new mountpoint");
		goto ex;
	}

	// mount root block
	if(xmount(false,path,point,type,flags,true)!=0){
		e=-errno;
		goto ex;
	}

	// try to search init
	if(!(init=search_init(point)))return -errno;

	// execute switchroot
	e=run_switch_root(point,init);

	ex:
	if(cache){
		if(type)free(type);
		blkid_put_cache(cache);
	}
	return e;
}
/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<blkid/blkid.h>
#include"init.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"defines.h"

#define TAG "conffs"
#define DEFAULT_CONFFS_FILE "simple-init.linux.conf"
#define DEFAULT_CONFFS_BLOCK "PARTLABEL=logfs"

static pthread_t t_conffs;

static int _setup_conffs(){
	int e=0;
	static char*base="runtime.cmdline";
	char*conffs=confd_get_string_base(base,"conffs",DEFAULT_CONFFS_BLOCK);
	char*conffile=confd_get_string_base(base,"conffile",DEFAULT_CONFFS_FILE);
	if(!conffs)ERET(ENOMEM);

	if(strcasecmp(conffs,"none")==0){
		free(conffs);
		if(conffile)free(conffile);
		tlog_warn("skip mount conffs");
		return 0;
	}

	wait_block(conffs,10,TAG);

	if(conffs[0]!='/'){
		char*x=blkid_evaluate_tag(conffs,NULL,NULL);
		free(conffs);
		conffs=x;
	}
	if(!conffs){
		if(conffile)free(conffile);
		return tlog_warn("conffs not found");
	}

	char*type=NULL;
	blkid_cache cache=NULL;
	blkid_get_cache(&cache,NULL);
	if(!(type=blkid_get_tag_value(cache,"TYPE",conffs))){
		telog_warn("cannot determine fstype in conffs %s",conffs);
		type="vfat";
		blkid_put_cache(cache);
		cache=NULL;
	}

	char mod[64]={0};
	snprintf(mod,63,"fs-%s",type);
	insmod(mod,false);

	char point[PATH_MAX];
	if((e=auto_mount(conffs,type,point,PATH_MAX))!=0)goto ex;

	char path[PATH_MAX]={0};
	snprintf(path,sizeof(path)-1,"%s/simple-init.static.linux.conf",point);
	confd_include_file(path);
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,"%s/%s",point,conffile);
	e=confd_set_default_config(path);
	confd_load_file(path);
	ex:
	if(cache){
		if(type)free(type);
		blkid_put_cache(cache);
	}
	if(conffile)free(conffile);
	free(conffs);
	return e;
}

static void*_conffs_thread(void*d __attribute__((unused))){
	_setup_conffs();
	return NULL;
}

int setup_conffs(){
	pthread_create(&t_conffs,NULL,_conffs_thread,NULL);
	if(!t_conffs)return -1;
	pthread_setname_np(t_conffs,"ConfFS Setup");
	return 0;
}

int wait_conffs(){
	if(!t_conffs)return -1;
	pthread_join(t_conffs,NULL);
	return 0;
}

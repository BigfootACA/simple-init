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
#include"str.h"
#include"init.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"cmdline.h"

#define TAG "logfs"
#define DEFAULT_LOGFS_FILE "simple-init.%N.log"
#define DEFAULT_LOGFS_BLOCK "PARTLABEL=logfs"

static pthread_t t_logfs;

static int _setup_logfs(){
	int e=0;
	static char*base="runtime.cmdline";
	char*logfs=confd_get_string_base(base,"logfs",DEFAULT_LOGFS_BLOCK);
	char*logfile=confd_get_string_base(base,"logfile",DEFAULT_LOGFS_FILE);
	if(!logfs)ERET(ENOMEM);

	if(strcasecmp(logfs,"none")==0){
		free(logfs);
		if(logfile)free(logfile);
		tlog_warn("skip mount logfs");
		return 0;
	}

	wait_block(logfs,10,TAG);

	if(logfs[0]!='/'){
		char*x=blkid_evaluate_tag(logfs,NULL,NULL);
		free(logfs);
		logfs=x;
	}
	if(!logfs){
		if(logfile)free(logfile);
		return tlog_warn("logfs not found");
	}

	char*type=NULL;
	blkid_cache cache=NULL;
	blkid_get_cache(&cache,NULL);
	if(!(type=blkid_get_tag_value(cache,"TYPE",logfs))){
		telog_warn("cannot determine fstype in logfs %s",logfs);
		type="vfat";
		blkid_put_cache(cache);
		cache=NULL;
	}

	char mod[64]={0};
	snprintf(mod,63,"fs-%s",type);
	insmod(mod,false);

	char point[PATH_MAX];
	if((e=auto_mount(logfs,type,point,PATH_MAX))!=0)goto ex;

	char cnum[8],path[PATH_MAX];
	keyval*v[]={
		&KV("N",cnum),
		NULL
	};
	size_t s=strlen(point);
	strcpy(path,point);
	path[s++]='/';
	for(int i=0;i<256;i++){
		memset(cnum,0,8);
		memset(path+s,0,PATH_MAX-s);
		snprintf(cnum,7,"%d",i);
		if(!replace(v,'%',path+s,logfile,PATH_MAX-s-1)){
			tlog_error("failed to parse logfile %s",logfile);
			goto ex;
		}
		if(access(path,F_OK)==0)continue;
		if(errno==ENOENT)break;
		telog_error("access %s",path);
		goto ex;
	}
	e=logger_open(path);
	ex:
	if(cache){
		if(type)free(type);
		blkid_put_cache(cache);
	}
	if(logfile)free(logfile);
	free(logfs);
	return e;
}

static void*_logfs_thread(void*d __attribute__((unused))){
	_setup_logfs();
	return NULL;
}

int setup_logfs(){
	pthread_create(&t_logfs,NULL,_logfs_thread,NULL);
	if(!t_logfs)return -1;
	pthread_setname_np(t_logfs,"LogFS Setup");
	return 0;
}

int wait_logfs(){
	if(!t_logfs)return -1;
	pthread_join(t_logfs,NULL);
	return 0;
}

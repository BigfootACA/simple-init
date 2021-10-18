/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include"pathnames.h"
#include"defines.h"
#include"logger.h"
#include"init.h"
#define TAG "environ"

void init_environ(){
	tlog_debug("set default environment variables.");
	setenv("PATH",_PATH_DEFAULT_PATH,1);
	setenv("TERM","linux",1);
	setenv("USER","root",1);
}

void dump_environ(int fd){
	if(fd<0)return;
	char*x=NULL;
	for(int i=0;(x=environ[i]);i++)dprintf(fd,"%s\n",x);
}

void dump_environ_to_file(char*path){
	int fd=open(path,O_WRONLY|O_CREAT|O_SYNC);
	if(fd<0){
		tlog_error("failed to open %s",path);
		return;
	}
	dump_environ(fd);
	close(fd);
}

void auto_dump_environ_file(){
	char n[128]={0};
	snprintf(n,127,_PATH_TMP"/environ.%d.txt",getpid());
	dump_environ_to_file(n);
}

void log_environ(enum log_level level,char*tag){
	if(!tag)return;
	char*x=NULL;
	for(int i=0;(x=environ[i]);i++)logger_printf(level,tag,"%s",x);
}

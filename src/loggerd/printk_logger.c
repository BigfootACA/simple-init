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
#include<unistd.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include"pathnames.h"
#include"logger.h"
#include"defines.h"

static int init_kmsg(){
	struct stat st;
	bool mk=false;
	if(stat(_PATH_DEV_KMSG,&st)==0){
		if(!S_ISCHR(st.st_mode))mk=true;
		if(major(st.st_dev)!=1)mk=true;
		if(minor(st.st_dev)!=11)mk=true;
	}else mk=true;
	if(mk){
		unlink(_PATH_DEV_KMSG);
		mknod(_PATH_DEV_KMSG,S_IFCHR|0644,makedev(1,11));
	}
	return open(_PATH_DEV_KMSG,O_WRONLY|O_NOCTTY|O_CLOEXEC);
}

int printk_logger(char*name __attribute__((unused)),struct log_item *log){
	static int fd=-1;
	if(!log->time)ERET(EFAULT);
	if(strcmp(log->tag,"kernel")==0&&log->pid==0)return 0;
	size_t size=sizeof(char)*(20+
		strlen(log->tag)+
		strlen(log->content)
	);
	char*buff=malloc(size);
	if(!buff)ERET(ENOMEM);
	memset(buff,0,size);
	size=snprintf(
		buff,
		size,
		"<%d>simple-init %s: %s\n",
		logger_level2klevel(log->level),
		log->tag,
		log->content
	);
	int r=-1;
	errno=0;
	if(fd<0)fd=init_kmsg();
	if(fd>=0)r=write(fd,buff,size);
	free(buff);
	errno=r;
	return r==0?(int)size:-1;
}

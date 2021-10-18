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
#include"pathnames.h"
#include"logger.h"
#include"defines.h"

static char*device=NULL;

int init_klog_manual(char*dev){
	return access(device=dev,F_OK);
}

int init_klog(){
	return init_klog_manual(_PATH_DEV_KMSG);
}

int printk_logger(char*name __attribute__((unused)),struct log_item *log){
	if(!log->time)ERET(EFAULT);
	if(!device)ERET(ENOENT);
	FILE*klog=NULL;
	size_t size=sizeof(char)*(16+
		strlen(log->tag)+
		strlen(log->content)
	);
	char*buff=malloc(size);
	if(!buff)ERET(ENOMEM);
	memset(buff,0,size);
	size=snprintf(
		buff,
		size,
		"<%d>%s: %s\n",
		logger_level2klevel(log->level),
		log->tag,
		log->content
	);
	int r=0;
	errno=0;
	if(!(klog=fopen(device,"w"))){
		free(buff);
		return -errno;
	}
	if((size_t)fputs(buff,klog)!=size)
		r=(errno==0?-1:-errno);
	fflush(klog);
	fclose(klog);
	free(buff);
	errno=r;
	return r==0?(int)size:-1;
}

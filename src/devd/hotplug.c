/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"logger.h"
#include"defines.h"
#include"devd_internal.h"
#define TAG "hotplug"

int hotplug_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	open_socket_logfd_default();
	int ds;
	size_t s=0;
	char*p,*d;
	struct devd_msg msg;
	if((ds=open_default_devd_socket(TAG))<0)return terlog_error(-errno,"open devd socket failed");
	for(size_t i=0;environ[i];i++)s+=strlen(environ[i])+1;
	if(!(d=p=malloc(s)))return terlog_error(-errno,"malloc");
	memset(p,0,s);
	for(size_t i=0;environ[i];i++){
		for(size_t e=0;environ[i][e]!=0;e++)*(d++)=environ[i][e];
		*(d++)='\n';
	}
	devd_internal_send_msg(ds,DEV_ADD,p,s);
	free(p);
	if(devd_internal_read_msg(ds,&msg)>=0&&msg.size>0)lseek(ds,(size_t)msg.size,SEEK_CUR);
	close(ds);
	return 0;
}

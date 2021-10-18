/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<sys/uio.h>
#include<stdbool.h>
#include<stdlib.h>
#include"defines.h"
#include"devd_internal.h"

void devd_internal_init_msg(struct devd_msg*msg,enum devd_oper oper,size_t size){
	if(!msg)return;
	memset(msg,0,sizeof(struct devd_msg));
	msg->magic0=DEVD_MAGIC0;
	msg->magic1=DEVD_MAGIC1;
	msg->oper=oper;
	msg->size=size;
}

bool devd_internal_check_magic(struct devd_msg*msg){
	return msg&&msg->magic0==DEVD_MAGIC0&&msg->magic1==DEVD_MAGIC1;
}

char*devd_read_data(int fd,struct devd_msg*msg){
	if(!msg||msg->size<=0)return NULL;
	char*data=malloc(msg->size);
	if(data&&(size_t)read(fd,data,msg->size)!=msg->size){
		free(data);
		return NULL;
	}
	return data;
}

int devd_internal_send_msg(int fd,enum devd_oper oper,void*data,size_t size){
	static size_t xs=sizeof(struct devd_msg);
	struct devd_msg msg;
	if(fd<0)ERET(EINVAL);
	devd_internal_init_msg(&msg,oper,size);
	struct iovec i[2]={
		{&msg,xs},
		{data,size}
	};
	size_t s=xs+size;
	return ((size_t)writev(fd,i,size==0?1:2))==s?(int)s:-1;
}

int devd_internal_read_msg(int fd,struct devd_msg*buff){
	if(!buff||fd<0)ERET(EINVAL);
	size_t size=sizeof(struct devd_msg),s;
	memset(buff,0,size);
	errno=0;
	s=read(fd,buff,size);
	if(errno>0&&errno!=EAGAIN)return -2;
	if(s==0)return EOF;
	return (s!=size||!devd_internal_check_magic(buff))?-2:0;
}

int devd_internal_send_msg_string(int fd,enum devd_oper oper,char*data){
	return devd_internal_send_msg(fd,oper,data,data?strlen(data)*sizeof(char):0);
}

int devd_command_with_data(enum devd_oper oper,void*data,size_t size){
	devd_internal_send_msg(devfd,oper,data,size);
	struct devd_msg msg;
	if(devd_internal_read_msg(devfd,&msg)<0)return -1;
	if(msg.size>0)lseek(devfd,(size_t)msg.size,SEEK_CUR);
	return msg.oper==DEV_OK;
}

int devd_command(enum devd_oper oper){
	return devd_command_with_data(oper,NULL,0);
}
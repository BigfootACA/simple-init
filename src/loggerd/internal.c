/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdarg.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include"defines.h"
#include"list.h"
#include"logger_internal.h"

list*loggers=NULL;

static int _free_logger(void*data){
	if(!data)ERET(EINVAL);
	struct logger*l=(struct logger*)data;
	free(l->name);
	free(l);
	return 0;
}

void logger_internal_clean(){
	list*f=list_first(loggers);
	if(!f)return;
	list_free_all(loggers,_free_logger);
	loggers=NULL;
}

int logger_internal_add(char*name,enum log_level min_level,on_log log){
	if(!name||!log)ERET(EINVAL);
	size_t s=sizeof(struct logger);
	struct logger*logger;
	if(!(logger=malloc(s)))ERET(ENOMEM);
	memset(logger,0,s);
	if(!(logger->name=strdup(name))){
		free(logger);
		ERET(ENOMEM);
	}
	logger->min_level=min_level;
	logger->logger=log;
	logger->enabled=false;
	if(!loggers)loggers=list_new(logger);
	else list_push_new(loggers,logger);
	if(errno!=0)_free_logger(logger);
	return -errno;
}

int logger_internal_write(struct log_item*log){
	if(!log)ERET(EINVAL);
	if(!loggers)ERET(EFAULT);
	int len=-1;
	list*i;
	struct logger*l;
	logger_internal_buffer_push(log);
	if(!(i=list_first(loggers)))ERET(ENOENT);
	do{
		l=LIST_DATA(i,struct logger*);
		if(!l||!l->name||!l->logger||!l->enabled)continue;
		if((l->min_level)>(log->level))continue;
		len+=l->logger(l->name,log);
	}while((i=i->next));
	return len<0?-ENOENT:len;
}

int logger_internal_print(enum log_level level,char*tag,char*content){
	struct log_item log;
	log.level=level;
	strncpy(log.tag,tag,sizeof(log.tag)-1);
	strncpy(log.content,content,sizeof(log.content)-1);
	time(&log.time);
	log.pid=getpid();
	return logger_internal_write(&log);
}

static int logger_internal_printf_x(enum log_level level,char*tag,const char*fmt,va_list ap){
	size_t size=sizeof(char)*BUFFER_SIZE;
	char*content=malloc(size+1);
	if(!content)return -errno;
	memset(content,0,size+1);
	if(!vsnprintf(content,size,fmt,ap)){
		free(content);
		return -errno;
	}
	int r=logger_internal_print(level,tag,content);
	free(content);
	return r;
}

int logger_internal_printf(enum log_level level,char*tag,const char*fmt,...){
	int er=errno;
	va_list ap;
	va_start(ap,fmt);
	int r=logger_internal_printf_x(level,tag,fmt,ap);
	va_end(ap);
	errno=er;
	return r;
}

int logger_internal_set(char*name,bool enabled){
	if(!name)ERET(EINVAL);
	if(!loggers)ERET(EFAULT);
	list*i;
	struct logger*l;
	if(!(i=list_first(loggers)))ERET(ENOENT);
	do{
		l=LIST_DATA(i,struct logger*);
		if(!l||!l->name)continue;
		if(strcmp(l->name,name)!=0)continue;
		l->enabled=enabled;
		if(enabled)flush_buffer(l);
		logger_internal_printf(
			LEVEL_INFO,
			"logger",
			"Logger '%s' %sabled.",
			l->name,
			enabled?"en":"dis"
		);
		return 0;
	}while((i=i->next));
	ERET(ENOENT);
}

int logger_internal_set_level(char*name,enum log_level level){
	if(!name)ERET(EINVAL);
	if(!loggers)ERET(EFAULT);
	list*i;
	struct logger*l;
	if(!(i=list_first(loggers)))ERET(ENOENT);
	do{
		l=LIST_DATA(i,struct logger*);
		if(!l||!l->name)continue;
		if(strcmp(l->name,name)!=0)continue;
		l->min_level=level;
		return 0;
	}while((i=i->next));
	ERET(ENOENT);
}

bool logger_internal_check_magic(struct log_msg*msg){
	return msg&&msg->magic0==LOGD_MAGIC0&&msg->magic1==LOGD_MAGIC1;
}

void logger_internal_init_msg(struct log_msg*msg,enum log_oper oper){
	if(!msg)return;
	memset(msg,0,sizeof(struct log_msg));
	msg->magic0=LOGD_MAGIC0;
	msg->magic1=LOGD_MAGIC1;
	msg->oper=oper;
}

int logger_internal_send_code(int fd,enum log_oper oper,int code){
	struct log_msg msg;
	size_t xs=sizeof(struct log_msg);
	if(fd<0)ERET(EINVAL);
	logger_internal_init_msg(&msg,oper);
	msg.data.code=code;
	return ((size_t)write(fd,&msg,xs))==xs?(int)xs:-1;
}

int logger_internal_send_string(int fd,enum log_oper oper,char*string){
	struct log_msg msg;
	size_t xs=sizeof(struct log_msg);
	if(fd<0)ERET(EINVAL);
	logger_internal_init_msg(&msg,oper);
	if(string)strncpy(msg.data.string,string,sizeof(msg.data.string)-1);
	return ((size_t)write(fd,&msg,xs))==xs?(int)xs:-1;
}

int logger_internal_read_msg(int fd,struct log_msg*buff){
	if(!buff||fd<0)ERET(EINVAL);
	size_t size=sizeof(struct log_msg),s;
	memset(buff,0,size);
	errno=0;
	while(1){
		errno=0;
		s=read(fd,buff,size);
		if(errno==0)break;
		switch(errno){
			case EINTR:continue;
			case EAGAIN:return 0;
			default:return -2;
		}
	}
	if(s==0)return EOF;
	return (s!=size||!(logger_internal_check_magic(buff)))?-2:1;
}


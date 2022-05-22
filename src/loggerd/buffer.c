/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#ifdef ENABLE_UEFI
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#endif
#include"list.h"
#include"logger_internal.h"

list*logbuffer=NULL;

static int _buff_free(void*data){
	if(!data)ERET(EINVAL);
	struct log_buff*l=(struct log_buff*)data;
	if(l->tag)free(l->tag);
	if(l->content)free(l->content);
	free(l);
	return 0;
}

struct log_buff*logger_internal_item2buff(struct log_item*log){
	errno=EINVAL;
	if(!log)return NULL;
	size_t s=sizeof(struct log_buff);
	struct log_buff*buff=malloc(s);
	if(!buff)goto fail;
	memset(buff,0,s);
	buff->pid=log->pid;
	buff->time=log->time;
	buff->level=log->level;
	buff->tag=strdup(log->tag);
	buff->content=strdup(log->content);
	if(!buff->tag||!buff->content)goto fail;
	errno=0;
	return buff;
	fail:
	if(buff){
		if(buff->content)free(buff->content);
		if(buff->tag)free(buff->tag);
		free(buff);
	}
	errno=ENOMEM;
	return NULL;
}

struct log_item*logger_internal_buff2item(struct log_buff*log){
	errno=EINVAL;
	if(!log)return NULL;
	size_t s=sizeof(struct log_item);
	struct log_item*item=malloc(s);
	if(!item)return NULL;
	memset(item,0,s);
	item->pid=log->pid;
	item->time=log->time;
	item->level=log->level;
	strncpy(item->tag,log->tag,sizeof(item->tag)-1);
	strncpy(item->content,log->content,sizeof(item->content)-1);
	errno=0;
	return item;
}

int logger_internal_buffer_push(struct log_item*log){
	struct log_buff*buff=logger_internal_item2buff(log);
	if(!buff)return -errno;
	if(list_obj_add_new(&logbuffer,buff)!=0)goto fail;
	return 0;
	fail:
	if(errno==0)errno=ENOMEM;
	_buff_free(buff);
	return -(errno);
}

char*logger_oper2string(enum log_oper oper){
	switch(oper){
		case LOG_OK:return "OK";
		case LOG_ADD:return "Add";
		case LOG_OPEN:return "Open";
		case LOG_CLOSE:return "Close";
		case LOG_CLEAR:return "Clear";
		case LOG_KLOG:return "Klog";
		case LOG_LISTEN:return "Listen";
		case LOG_QUIT:return "Quit";
		default:return "Unknown";
	}
}

int logger_internal_free_buff(void*d){
	struct log_buff*b=(struct log_buff*)d;
	if(!b)return -1;
	if(b->content)free(b->content);
	if(b->tag)free(b->tag);
	free(b);
	return 0;
}

void clean_log_buffers(){
	if(!logbuffer)return;
	list_free_all(logbuffer,logger_internal_free_buff);
	logbuffer=NULL;
}

#ifdef ENABLE_UEFI
void flush_buffer(){
	if(!logbuffer)return;
	char*str=NULL;
	UINTN len=0,xz=0;
	list*head,*cur,*next;
	struct log_buff*buff;
	if(!(head=list_first(logbuffer)))return;
	cur=head;
	do{
		next=cur->next;
		if(!(buff=LIST_DATA(cur,struct log_buff*)))continue;
		xz=AsciiStrLen(buff->tag)+AsciiStrLen(buff->content)+8;
		if(len<xz||!str){
			if(str)FreePool(str);
			if(!(str=AllocateZeroPool(xz)))return;
			len=xz;
		}
		AsciiSPrint(str,xz,"%a: %a",buff->tag,buff->content);
		logger_out_write(str);
	}while((cur=next));
	if(str)FreePool(str);
}
#else
void flush_buffer(struct logger*log){
	if(
		!log||
		!log->name||
		!log->enabled||
		!log->logger||
		log->flushed||
		!logbuffer
	)return;
	list*head,*cur,*next;
	struct log_item*item;
	struct log_buff*buff;
	if(!(head=list_first(logbuffer)))return;
	cur=head;
	do{
		next=cur->next;
		if(!(buff=LIST_DATA(cur,struct log_buff*)))continue;
		if(!(item=logger_internal_buff2item(buff)))continue;
		log->logger(log->name,item);
		free(item);
	}while((cur=next));
	log->flushed=true;
}
#endif

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
#include<string.h>
#include<stdbool.h>
#include"list.h"
#include"service.h"
#include"defines.h"

list*services=NULL;
mutex_t services_lock;

struct service*svc_lookup_by_name(char*name){
	if(!name)EPRET(EINVAL);
	if(services){
		list*cur,*next=list_first(services);
		if(next)do{
			cur=next,next=cur->next;
			LIST_DATA_DECLARE(s,cur,struct service*);
			if(!s)continue;
			if(strcmp(name,s->name)==0)return s;
		}while(next);
	}
	EPRET(ENOENT);
}

bool svc_is_running(struct service*svc){
	if(!svc)return false;
	switch(svc->status){
		case STATUS_STARTING:
		case STATUS_STARTED:
		case STATUS_RUNNING:
		case STATUS_STOPPING:
			return true;
		default:
			return false;
	}
}

char*svc_get_desc(struct service*svc){
	if(!svc)EPRET(EINVAL);
	char*desc=svc->description&&strlen(svc->description)>0?
		  svc->description:svc->name;
	return desc;
}

#define _SVC_INIT_EXEC(_oper)\
	int svc_init_exec_##_oper(struct service*svc){\
                if(!svc)ERET(EINVAL);\
		char name[BUFSIZ]={0};\
		snprintf(name,BUFSIZ-1,"%s "#_oper,svc->name);\
		if(svc->_oper)svc_free_exec(svc->_oper);\
		if(!(svc->_oper=svc_new_exec(name)))return -errno;\
		svc->_oper->prop.svc=svc;\
		return 0;\
	}\
	int svc_set_##_oper##_function(struct service*svc,svc_main*main){\
		if(!svc)ERET(EINVAL);\
		if(!svc->_oper&&svc_init_exec_##_oper(svc)!=0)return -errno;\
		return svc_exec_set_function(svc->_oper,main);\
	}\
	int svc_set_##_oper##_library(struct service*svc,char*library,char*symbol){\
		if(!svc)ERET(EINVAL);\
		if(!svc->_oper&&svc_init_exec_##_oper(svc)!=0)return -errno;\
		return svc_exec_set_library(svc->_oper,library,symbol);\
	}\
	int svc_set_##_oper##_command(struct service*svc,char*path,char**args,char**envs){\
		if(!svc)ERET(EINVAL);\
		if(!svc->_oper&&svc_init_exec_##_oper(svc)!=0)return -errno;\
		return svc_exec_set_command(svc->_oper,path,args,envs);\
	}
_SVC_INIT_EXEC(start)
_SVC_INIT_EXEC(stop)
_SVC_INIT_EXEC(restart)
_SVC_INIT_EXEC(reload)

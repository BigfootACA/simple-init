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
#include"lock.h"
#include"system.h"
#include"logger.h"
#include"service.h"
#include"defines.h"
#define TAG "service"

bool auto_restart=false;
struct service*svc_default,*svc_system,*svc_network;

int svc_add_depend(struct service*svc,struct service*dep){
	if(
		!svc||!svc->name||
		!dep||!dep->name||
		svc==dep
	)ERET(EINVAL);
	MUTEX_LOCK(svc->lock);
	MUTEX_LOCK(dep->lock);
	list*l=list_new(dep),*v=list_new(svc);
	int er;
	if(!l||!v)goto fail;
	if(!svc->depends_on)svc->depends_on=l;
	else if(list_push(svc->depends_on,l)!=0)goto fail;
	if(!dep->depends_of)dep->depends_of=v;
	else if(list_push(dep->depends_of,v)!=0)goto fail;
	MUTEX_UNLOCK(svc->lock);
	MUTEX_UNLOCK(dep->lock);
	time(&svc->last_update);
	return 0;
	fail:
	if(errno==0)errno=ENOMEM;
	er=errno;
	list_remove(l);
	list_remove(v);
	if(l)free(l);
	if(v)free(v);
	MUTEX_UNLOCK(svc->lock);
	MUTEX_UNLOCK(dep->lock);
	ERET(er);
}

int svc_add_service(struct service*svc){
	if(!svc||!svc->name)ERET(EINVAL);
	int e;
	list*l=list_new(svc);
	if(!l)goto fail;
	if(!services){
		services=l;
		return 0;
	}
	if(svc_lookup_by_name(svc->name)){
		errno=EEXIST;
		goto fail;
	}else if(errno!=ENOENT)goto fail;
	MUTEX_LOCK(services_lock);
	e=list_push(services,l);
	MUTEX_UNLOCK(services_lock);
	time(&svc->last_update);
	if(e==0)return 0;
	fail:
	e=errno;
	if(l)free(l);
	ERET(e);
}

int svc_set_desc(struct service*svc,char*desc){
	if(!svc)ERET(EINVAL);
	if(desc&&strlen(desc)>1024)ERET(ENAMETOOLONG);
	MUTEX_LOCK(svc->lock);
	if(svc->description)free(svc->description);
	svc->description=strdup(desc);
	MUTEX_UNLOCK(svc->lock);
	time(&svc->last_update);
	if(!svc->description&&desc)ERET(ENOMEM);
	else return 0;
}

int svc_set_name(struct service*svc,char*name){
	if(!svc)ERET(EINVAL);
	if(strlen(name)>64)ERET(ENAMETOOLONG);
	MUTEX_LOCK(svc->lock);
	if(svc->name)free(svc->name);
	svc->name=strdup(name);
	MUTEX_UNLOCK(svc->lock);
	time(&svc->last_update);
	if(!svc->name)ERET(ENOMEM);
	else return 0;
}

int service_wait_all_stop(){
	auto_restart=false;
	service_stop_all();
	list*cur,*next,*first;
	if(!services||!(first=list_first(services)))return 0;
	bool complete=false,show=false;
	while(!complete){
		next=first,complete=true;
		do{
			cur=next,next=cur->next;
			LIST_DATA_DECLARE(s,cur,struct service*);
			if(!s)continue;
			switch(s->status){
				case STATUS_UNKNOWN:
				case STATUS_STOPPED:
				case STATUS_FAILED:break;
				case STATUS_STARTED:
				case STATUS_RUNNING:service_stop(s);//fallthrough
				case STATUS_STARTING:
				case STATUS_STOPPING:complete=false;
			}
		}while(next);
		if(!complete){
			if(!show)tlog_notice("wait all services stop");
			show=true;
			xsleep(1);
		}else tlog_info("all services stopped");
	}
	return 0;
}

int service_init(){
	MUTEX_INIT(services_lock);
	svc_default=svc_create_service("default",WORK_FAKE);
	svc_system=svc_create_service("system",WORK_FAKE);
	svc_network=svc_create_service("network",WORK_FAKE);
	if(!svc_default||!svc_system||!svc_network){
		tlog_emerg("FAILED TO CREATE DEFAULT SERVICE");
		abort();
	}
	svc_set_desc(svc_default,"Default Service");
	svc_set_desc(svc_system,"Default System");
	svc_set_desc(svc_network,"Network Environment");
	start_scheduler();
	auto_restart=true;
	return 0;
}

int svc_restart_service(struct service*svc){
	if(!svc)ERET(EINVAL);
	char*name=svc_get_desc(svc);
	if(!svc->restart){
		tlog_warn("service %s has no restart operation",name);
		ERET(ENOTSUP);
	}
	time(&svc->last_update);
	tlog_notice("Restarting service %s",name);
	svc_run_exec(svc->restart);
	if(errno>0)tlog_warn("Restart service %s failed",name);
	return errno;
}

int svc_reload_service(struct service*svc){
	if(!svc)ERET(EINVAL);
	char*name=svc_get_desc(svc);
	if(!svc->reload){
		tlog_warn("service %s has no restart operation",name);
		ERET(ENOTSUP);
	}
	switch(svc->mode){
		case WORK_UNKNOWN:
		case WORK_FAKE:
		case WORK_ONCE:ERET(trlog_warn(
			ENOTSUP,
			"service %s with mode %s does not support reload",
			name,svc_work_string(svc->mode)
		));
		default:;
	}
	switch(svc->status){
		case STATUS_STARTED:
		case STATUS_STOPPED:
		case STATUS_FAILED:ERET(ECHILD);
		case STATUS_UNKNOWN:ERET(ENOTSUP);
		case STATUS_STARTING:
		case STATUS_STOPPING:ERET(EINPROGRESS);
		case STATUS_RUNNING:break;
	}
	time(&svc->last_update);
	tlog_notice("Restarting service %s",name);
	svc_run_exec(svc->restart);
	if(errno>0)tlog_warn("Restart service %s failed",name);
	return errno;
}

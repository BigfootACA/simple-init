/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<pthread.h>
#include"service.h"
#include"defines.h"
#include"logger.h"
#define TAG "service"

static int check_stop_service(struct service*svc){
	if(
		!svc||
		!svc->name||
		svc->mode==WORK_UNKNOWN
	)ERET(EINVAL);
	switch(svc->status){
		case STATUS_UNKNOWN:
			ERET(EINVAL);
		case STATUS_STARTING:
		case STATUS_STOPPING:
			ERET(EBUSY);
		case STATUS_FAILED:
			if(svc->mode==WORK_ONCE)break;
			// fallthrough
		case STATUS_STOPPED:
			ERET(ECHILD);
		case STATUS_RUNNING:
		case STATUS_STARTED:
			break;
	}
	return 0;
}

int svc_stop_service_nodep(struct service*svc){
	char*name=svc_get_desc(svc);
	MUTEX_LOCK(svc->lock);
	if(check_stop_service(svc)!=0){
		if(errno!=EBUSY)telog_warn("stop service %s",name);
		goto done;
	}
	tlog_notice("Stopping service %s",name);
	enum svc_status old=svc->status;
	svc->status=STATUS_STOPPING;
	if(svc->mode==WORK_FAKE)goto stopped;
	if(!svc->stop){
		if(svc->mode==WORK_ONCE)goto stopped;
		errno=ENOTSUP;
		tlog_warn("service %s has no stop operation",name);
		goto failed;
	}
	if(svc_run_exec(svc->stop)!=0)goto failed;
	if(svc->stop->status.exit_code>0)goto failed;
	if(svc->stop->status.exit_signal>0)goto failed;
	if(!svc->stop->status.running)goto stopped;
	errno=0;
	goto done;
	stopped:
	tlog_notice("Stopped service %s",name);
	svc->status=STATUS_STOPPED,errno=0;
	goto done;
	failed:
	telog_warn("Stop service %s failed",name);
	svc->status=old;
	done:
	MUTEX_UNLOCK(svc->lock);
	return errno;
}

static int _svc_stop_service(struct service*svc,list*chain){
	if(check_stop_service(svc)!=0)return -errno;
	if(svc->depends_of){
		list*cur,*next;
		if((next=list_first(chain)))do{
			cur=next;
			LIST_DATA_DECLARE(s,cur,struct service*);
			if(s!=svc)continue;
			tlog_error(
				"Depends chain loop detect on %s",
				svc_get_desc(svc)
			);
			ERET(ELOOP);
		}while((next=cur->next));
		if(list_push_new(chain,svc)!=0)return -errno;
		int e=0;
		if((next=list_first(svc->depends_of)))do{
			cur=next;
			if(!cur->data)continue;
			LIST_DATA_DECLARE(s,cur,struct service*);
			if(check_stop_service(s)!=0)continue;
			errno=0;
			if(_svc_stop_service(s,chain)<0){
				e=errno;
				telog_error(
					"Depend %s for %s stop failed",
					svc_get_desc(s),
					svc_get_desc(svc)
				);
			}
		}while((next=cur->next));
		if(e!=0)return -(errno=e);
	}
	errno=0;
	return svc_stop_service_nodep(svc);
}

int svc_stop_service(struct service*svc){
	list*l=list_new(NULL);
	if(!l)return -errno;
	_svc_stop_service(svc,l);
	int r=errno;
	list_free_all(l,NULL);
	return -(errno=r);
}

int svc_stop_service_by_name(char*name){
	if(!name)ERET(EINVAL);
	struct service*svc=svc_lookup_by_name(name);
	return svc?svc_stop_service(svc):-errno;
}
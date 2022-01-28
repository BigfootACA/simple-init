/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<string.h>
#include<stddef.h>
#include"str.h"
#include"lock.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"service.h"
#include"defines.h"
#include"pathnames.h"
#define TAG "service"

static int check_start_service(struct service*svc){
	if(
		!svc||
		!svc->name||
		svc->mode==WORK_UNKNOWN
	)ERET(EINVAL);
	switch(svc->status){
		case STATUS_UNKNOWN:
			ERET(EINVAL);
		case STATUS_STARTING:
		case STATUS_STARTED:
		case STATUS_RUNNING:
		case STATUS_STOPPING:
			ERET(EBUSY);
		case STATUS_STOPPED:
		case STATUS_FAILED:
			break;
	}
	return 0;
}

int svc_daemon_get_pid(struct service*svc){
	char*name=svc_get_desc(svc);
	if(!svc->pid_file){
		tlog_error("daemon service %s has no PID file",name);
		memset(&svc->process,0,sizeof(struct proc_status));
		svc->status=STATUS_FAILED;
		return -1;
	}
	char buff[128]={0};
	if(read_file(buff,128,false,svc->pid_file)<0){
		telog_error("failed to read service %s PID file",name);
		memset(&svc->process,0,sizeof(struct proc_status));
		svc->status=STATUS_FAILED;
		return -1;
	}
	pid_t p;
	if((p=parse_int(buff,0))<=0){
		telog_error("failed to parse service %s PID file",name);
		memset(&svc->process,0,sizeof(struct proc_status));
		svc->status=STATUS_FAILED;
		return -1;
	}
	if(!is_file(_PATH_PROC"/%d/comm",p)){
		tlog_warn("PID %d of service %s not found",p,name);
		memset(&svc->process,0,sizeof(struct proc_status));
		svc->status=STATUS_STOPPED;
		return -1;
	}
	confd_set_integer_base("runtime.pid",svc->name,p);
	svc->status=STATUS_RUNNING;
	svc->process.pid=p;
	svc->process.running=true;
	svc->process.start=svc->start->status.start;
	svc->process.active=svc->process.start;
	return 0;
}

int svc_start_service_nodep(struct service*svc){
	char*name=svc_get_desc(svc);
	if(check_start_service(svc)!=0)
		return terlog_warn(errno,"start service %s",name);
	MUTEX_LOCK(svc->lock);
	svc->status=STATUS_STARTING;
	tlog_notice("Starting service %s",name);
	if(svc->mode==WORK_FAKE)goto started;
	if(!svc->start){
		errno=ENOTSUP;
		tlog_warn("service %s has no start operation",name);
		goto failed;
	}
	if(svc_run_exec(svc->start)!=0)goto failed;
	if(svc->start->status.exit_code>0)goto failed;
	if(svc->start->status.exit_signal>0)goto failed;
	bool run=svc->start->status.running;
	switch(svc->mode){
		case WORK_FOREGROUND:
			if(run){
				memcpy(&svc->process,&svc->start->status,sizeof(struct proc_status));
				memset(&svc->start->status,0,sizeof(struct proc_status));
				confd_set_integer_base("runtime.pid",svc->name,svc->process.pid);
				goto running;
			}else goto stopped;
		case WORK_ONCE:
			if(run)goto done;
			goto started;
		case WORK_DAEMON:
			if(run||svc_daemon_get_pid(svc)<0)goto done;
			else goto running;
		default:goto failed;
	}
	errno=0;
	running:
	tlog_notice("Started service %s",name);
	svc->status=STATUS_RUNNING,errno=0;
	goto done;
	stopped:
	tlog_notice("Stopped service %s",name);
	svc->status=STATUS_STOPPED,errno=0;
	goto done;
	started:
	tlog_notice("Finish service %s",name);
	svc->status=STATUS_STARTED,errno=0;
	goto done;
	failed:
	telog_warn("Start service %s failed",name);
	svc->status=STATUS_FAILED;
	done:
	MUTEX_UNLOCK(svc->lock);
	return errno;
}

static int _svc_start_service(struct service*svc,list*chain){
	if(check_start_service(svc)!=0)return -errno;
	if(svc->depends_on){
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
		if((next=list_first(svc->depends_on)))do{
			cur=next;
			if(!cur->data)continue;
			LIST_DATA_DECLARE(s,cur,struct service*);
			if(check_start_service(s)!=0)continue;
			errno=0;
			if(_svc_start_service(s,chain)<0){
				e=errno;
				telog_error(
					"Depend %s for %s start failed",
					svc_get_desc(s),
					svc_get_desc(svc)
				);
			}
		}while((next=cur->next));
		if(e!=0)return -(errno=e);
	}
	errno=0;
	return svc_start_service_nodep(svc);
}

int svc_start_service(struct service*svc){
	list*l=list_new(NULL);
	if(!l)return -errno;
	_svc_start_service(svc,l);
	int r=errno;
	list_free_all(l,NULL);
	return -(errno=r);
}

int svc_start_service_by_name(char*name){
	if(!name)ERET(EINVAL);
	struct service*svc=svc_lookup_by_name(name);
	return svc?svc_start_service(svc):-errno;
}

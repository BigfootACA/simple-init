/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<time.h>
#include<errno.h>
#include<signal.h>
#include<string.h>
#include"system.h"
#include"logger.h"
#include"service.h"
#include"init_internal.h"
#include"defines.h"
#include"pathnames.h"
#define TAG "service"

static int _get_pid(struct service*svc,pid_t*pid){
	pid_t p=0;
	char*name=svc_get_desc(svc);
	if(svc->process.running)p=svc->process.pid;
	if(svc->start&&svc->start->status.running)p=svc->start->status.pid;
	if(p<=0)return terlog_error(-ENODATA,"failed to get service %s pid",name);
	*pid=p;
	return 0;
}

int svc_default_stop(struct service*svc){
	if(!svc)ERET(EINVAL);
	switch(svc->mode){
		case WORK_UNKNOWN:
		case WORK_FAKE:
		case WORK_ONCE:return 0;
		default:;
	}
	pid_t p=0;
	char*name=svc_get_desc(svc);
	if(_get_pid(svc,&p)<0)return -errno;
	kill(p,SIGTERM);
	time_t t=svc->stop->prop.timeout/3*2;
	tlog_notice("try to stopping %s",name);
	for(int i=0;i<t;i++){
		if(!is_file(_PATH_PROC"/%d/comm",p))return 0;
		xsleep(1);
	}
	tlog_notice("try to kill %s",name);
	kill(p,SIGKILL);
	while(is_file(_PATH_PROC"/%d/comm",p))xsleep(1);
	return 0;
}

int svc_default_restart(struct service*svc){
	open_socket_initfd(DEFAULT_INITD,true);
	struct init_msg msg,response;
	tlog_notice("try to stopping service %s",svc_get_desc(svc));
	init_initialize_msg(&msg,ACTION_SVC_STOP);
	strcpy(msg.data.data,svc->name);
	init_send(&msg,&response);
	tlog_debug("wait service %s stopped",svc_get_desc(svc));
	for(;;){
		init_initialize_msg(&msg,ACTION_SVC_STATUS);
		strcpy(msg.data.data,svc->name);
		init_send(&msg,&response);
		enum svc_status st=response.data.svc_status;
		if(st==STATUS_STOPPED||st==STATUS_FAILED)break;
		sleep(1);
	}
	tlog_notice("try to starting %s",svc_get_desc(svc));
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,svc->name);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		if(errno==0)errno=response.data.status.ret;
		telog_warn("start failed");
	}
	return 0;
}

int svc_default_reload(struct service*svc){
	if(!svc)ERET(EINVAL);
	pid_t p=0;
	if(_get_pid(svc,&p)<0)return -errno;
	kill(p,SIGHUP);
	return 0;
}

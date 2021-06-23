#include<time.h>
#include<errno.h>
#include<signal.h>
#include"system.h"
#include"logger.h"
#include"service.h"
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
	if(svc_default_stop(svc)<0)return -errno;
	tlog_notice("try to starting %s",svc_get_desc(svc));
	service_start(svc);
	return 0;
}

int svc_default_reload(struct service*svc){
	if(!svc)ERET(EINVAL);
	pid_t p=0;
	if(_get_pid(svc,&p)<0)return -errno;
	kill(p,SIGHUP);
	return 0;
}

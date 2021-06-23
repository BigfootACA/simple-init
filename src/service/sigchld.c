#include<errno.h>
#include<stddef.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<pthread.h>
#include"list.h"
#include"logger.h"
#include"defines.h"
#include"service.h"
#define TAG "service"

static int svc_on_exit_main(struct service*svc){
	char*name=svc_get_desc(svc);
	switch(svc->mode){
		case WORK_UNKNOWN:return -1;
		case WORK_FAKE:
			tlog_warn("fake service %s should not have main",name);
			svc->status=STATUS_STARTED;
			memset(&svc->process,0,sizeof(struct proc_status));
		break;
		case WORK_DAEMON:
		case WORK_FOREGROUND:
			if(
				svc->status!=STATUS_RUNNING&&
				svc->status!=STATUS_STOPPING
			){
				tlog_warn("service %s does not running %s",name,svc_status_string(svc->status));
				memset(&svc->process,0,sizeof(struct proc_status));
			}else{
				tlog_info("Stopped service %s",name);
				if(svc->auto_restart&&auto_restart){
					tlog_info("Trigger service %s auto restart",name);
					service_start(svc);
				}
			}
			svc->status=STATUS_STOPPED;
		break;
		case WORK_ONCE:
			if(svc->status!=STATUS_STARTING){
				tlog_warn("service %s does not starting",name);
				memset(&svc->process,0,sizeof(struct proc_status));
			}else tlog_info("Started service %s",name);
			svc->status=STATUS_STARTED;
		break;
	}
	return 0;
}

static int svc_on_exit_start(struct svc_exec*exec,struct service*svc){
	if(exec->prop.svc!=svc)ERET(EINVAL);
	char*name=svc_get_desc(svc);
	if(svc->status!=STATUS_STARTING){
		tlog_warn("service %s is not starting but start exec exited",name);
		memset(&svc->process,0,sizeof(struct proc_status));
		svc->status=STATUS_STOPPED;
		return 0;
	}
	switch(svc->mode){
		case WORK_DAEMON:
			if(svc_daemon_get_pid(svc)<0)return -1;
			break;
		case WORK_ONCE:
			svc->status=STATUS_RUNNING;
			svc->process.running=false;
			svc->process.start=exec->status.start;
			svc->process.active=svc->process.start;
			break;
		default:
			tlog_warn("service %s is not daemon or once mode, but in starting status",name);
			memset(&svc->process,0,sizeof(struct proc_status));
			svc->status=STATUS_STOPPED;
			return 0;
	}
	tlog_info("Started service %s",name);
	return 0;
}

static int svc_on_exit_stop(struct svc_exec*exec,struct service*svc){
	if(exec->prop.svc!=svc)ERET(EINVAL);
	char*name=svc_get_desc(svc);
	if(
		svc->status!=STATUS_STOPPING&&
		svc->status!=STATUS_STOPPED
	){
		tlog_warn("service %s is not stopping but stop exec exited",name);
		memset(&svc->process,0,sizeof(struct proc_status));
		return 0;
	}
	if(
		!svc->process.finish||
		(svc->start&&!svc->start->status.running)
	)svc->status=STATUS_STOPPED;
	tlog_info("Stopped service %s",name);
	return 0;
}

static int svc_exec_on_exit(pid_t p,struct svc_exec*exec,struct service*svc,int st){
	if(p!=(exec?exec->status:svc->process).pid)return 1;
	struct proc_status*status=exec?&exec->status:&svc->process;
	if(!status->running){
		tlog_warn("%d already stopped",status->pid);
		ERET(EALREADY);
	}
	int c;
	pthread_mutex_lock(&svc->lock);
	if(exec)pthread_mutex_lock(&exec->lock);
	if(WIFEXITED(st)){
		c=WEXITSTATUS(st);
		status->exit_code=c;
		status->exit_signal=0;
	}else if(WIFSIGNALED(st)){
		c=WTERMSIG(st);
		status->exit_code=128|c;
		status->exit_signal=c;
	}else goto finish;
	time(&status->finish);
	status->running=false;
	if(!exec)svc_on_exit_main(svc);
	else if(svc->start==exec)svc_on_exit_start(exec,svc);
	else if(svc->stop==exec)svc_on_exit_stop(exec,svc);
	finish:
	pthread_mutex_unlock(&svc->lock);
	if(exec)pthread_mutex_unlock(&exec->lock);
	return 0;
}

int svc_on_sigchld(pid_t pid,int st){
	if(!services)return 0;
	pthread_mutex_lock(&services_lock);
	list*cur,*next=list_first(services);
	if(next)do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(!s)continue;
		int e=1;
		if(e>0)e=svc_exec_on_exit(pid,NULL,s,st);
		if(e>0)e=svc_exec_on_exit(pid,s->stop,s,st);
		if(e>0)e=svc_exec_on_exit(pid,s->start,s,st);
		if(e>0)e=svc_exec_on_exit(pid,s->reload,s,st);
		if(e>0)e=svc_exec_on_exit(pid,s->restart,s,st);
		if(e<=0)break;
	}while(next);
	pthread_mutex_unlock(&services_lock);
	return 0;
}
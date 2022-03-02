/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include<sys/select.h>
#include"pool.h"
#include"lock.h"
#include"list.h"
#include"init.h"
#include"service.h"
#include"defines.h"
#include"service_scheduler.h"
#define TAG "service"
#define NAME "Service Scheduler"

struct pool*service_workers=NULL;
mutex_t queue_lock;
list*queue=NULL;
static pthread_t scheduler;
static mutex_t lock;
static int fds[2];

int free_scheduler_work(void*d){
	if(d)free((struct scheduler_work*)d);
	return 0;
}

void*scheduler_worker(void*data){
	if(!data)return NULL;
	bool found=false;
	list*cur,*next;
	struct scheduler_work*w=(struct scheduler_work*)data;
	MUTEX_LOCK(queue_lock);
	if((next=list_first(queue)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct scheduler_work*);
		if(s==w){
			list_obj_del(&queue,cur,NULL);
			found=true;
		}
	}while(next);
	MUTEX_UNLOCK(queue_lock);
	if(!found)return NULL;
	switch(w->action){
		case SCHED_START:switch(w->service->status){
			case STATUS_STARTING:
			case STATUS_STOPPING:
			case STATUS_STARTED:
			case STATUS_RUNNING:break;
			default:svc_start_service_nodep(w->service);
		}break;
		case SCHED_STOP:switch(w->service->status){
			case STATUS_STARTING:
			case STATUS_STOPPING:
			case STATUS_STOPPED:break;
			case STATUS_FAILED:
			default:svc_stop_service_nodep(w->service);
		}break;
		case SCHED_RELOAD:svc_reload_service(w->service);break;
		case SCHED_RESTART:svc_restart_service(w->service);break;
		default:;
	}
	free_scheduler_work(w);
	return NULL;
}

static int seconds_handler(){
	time_t cur_time;
	time(&cur_time);
	list*cur,*next;
	if((next=list_first(services)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(!s)continue;
		time_t finish_offset=cur_time-(s->process.finish);
		time_t update_offset=cur_time-(s->last_update);
		time_t reset_delay=MAX(1,s->restart_delay)*5;
		if(
			s->wait_restart&&
			s->restart_delay>0&&
			s->process.finish>0&&
			finish_offset>(s->restart_delay)
		){
			tlog_notice(
				"Trigger service %s auto restart times %d",
				svc_get_desc(s),s->retry
			);
			s->wait_restart=false;
			s->last_update=cur_time;
			add_queue(s,SCHED_START);
			continue;
		}
		if(
			s->retry>0&&s->last_update>0&&
			update_offset>reset_delay
		)s->retry=0,s->last_update=cur_time;
	}while(next);
	return 0;
}

static int scheduler_main(){
	open_socket_logfd_default();
	prctl(PR_SET_NAME,NAME);
	if(!(service_workers=pool_init_cpus(8192))){
		telog_crit("failed to init workers thread pool");
		return -1;
	}
	if(!(queue=list_new(NULL))){
		telog_crit("failed to init queue");
		pool_destroy(service_workers);
		service_workers=NULL;
		return -1;
	}
	MUTEX_INIT(queue_lock);
	fd_set fs;
	struct timeval tv;
	struct scheduler_msg msg;
	bool run=true;
	while(run){
		FD_ZERO(&fs);
		FD_SET(fds[0],&fs);
		tv.tv_sec=1,tv.tv_usec=0;
		if(select(FD_SETSIZE,&fs,NULL,NULL,&tv)<0){
			if(errno==EINTR)continue;
			perror("select");
			break;
		}
		seconds_handler();
		run_queue();
		if(!FD_ISSET(fds[0],&fs))continue;
		errno=0;
		ssize_t s=read(fds[0],&msg,sizeof(msg));
		if(errno==EAGAIN)continue;
		if(s<0&&errno!=EINTR)break;
		if(s!=sizeof(msg))continue;
		switch(msg.action){
			case SCHED_EXIT:run=false;
			case SCHED_UNKNOWN:break;
			case SCHED_CHILD:svc_on_sigchld(msg.data.exit.pid,msg.data.exit.stat);break;
			case SCHED_START:
			case SCHED_STOP:
			case SCHED_RELOAD:
			case SCHED_RESTART:add_queue(msg.data.service,msg.action);break;
			case SCHED_STOP_ALL:add_all_stop_queue();
			default:;
		}
		run_queue();
		memset(&msg,0,sizeof(struct scheduler_msg));
	}
	if(!run){
		close(fds[0]);
		close(fds[1]);
	}
	tlog_info("scheduler exit");
	MUTEX_DESTROY(queue_lock);
	return run?0:1;
}

static void*scheduler_thread(void*d __attribute__((unused))){
	int r=scheduler_main();
	if(r<0)telog_alert("service scheduler exited with %d",r);
	return NULL;
}

int start_scheduler(){
	static bool start=false;
	if(start)ERET(EEXIST);
	start=true;
	if(getpid()!=1)ERET(EACCES);
	if(socketpair(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0,fds)<0)return -errno;
	MUTEX_INIT(lock);
	if(pthread_create(&scheduler,NULL,scheduler_thread,NULL)!=0)return -errno;
	pthread_setname_np(scheduler,NAME);
	return 0;
}

int oper_scheduler(struct scheduler_msg*data){
	MUTEX_LOCK(lock);
	write(fds[1],data,sizeof(struct scheduler_msg));
	MUTEX_UNLOCK(lock);
	return 0;
}

int stop_scheduler(){
	struct scheduler_msg m;
	m.action=SCHED_EXIT;
	return oper_scheduler(&m);
}

int service_sigchld(pid_t p,int st){
	struct scheduler_msg m;
	m.action=SCHED_CHILD;
	m.data.exit.pid=p;
	m.data.exit.stat=st;
	return oper_scheduler(&m);
}

int service_stop_all(){
	struct scheduler_msg m;
	m.action=SCHED_STOP_ALL;
	return oper_scheduler(&m);
}

int service_terminal_output(){
	list*cur,*next;
	if(!services)ERET(EINVAL);
	if((next=list_first(services)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(
			!s||!s->terminal_output_signal||
			!s->process.running||s->process.pid<=0
		)continue;
		kill(s->process.pid,SIGTTOU);
	}while(next);
	return 0;
}

#define DEF_OPER(_oper,_act)\
	int service_##_oper(struct service*svc){\
		struct scheduler_msg m;\
		m.action=_act;\
		m.data.service=svc;\
		return oper_scheduler(&m);\
	}\
	int service_##_oper##_by_name(char*name){\
		struct service*svc=svc_lookup_by_name(name);\
		return svc?service_##_oper(svc):-errno;\
	}
DEF_OPER(stop,SCHED_STOP)
DEF_OPER(start,SCHED_START)
DEF_OPER(reload,SCHED_RELOAD)
DEF_OPER(restart,SCHED_RESTART)

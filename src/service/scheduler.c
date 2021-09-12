#define _GNU_SOURCE
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include"pool.h"
#include"list.h"
#include"init.h"
#include"service.h"
#include"defines.h"
#include"service_scheduler.h"
#define TAG "service"
#define NAME "Service Scheduler"

struct pool*service_workers=NULL;
pthread_mutex_t queue_lock;
list*queue=NULL;
static pthread_t scheduler;
static pthread_mutex_t lock;
static int fds[2];

int free_scheduler_work(void*d){
	if(d)free((struct scheduler_work*)d);
	return 0;
}

void*scheduler_worker(void*data){
	if(!data)return NULL;
	struct scheduler_work*w=(struct scheduler_work*)data;
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
			case STATUS_STOPPED:
			case STATUS_FAILED:break;
			default:svc_stop_service_nodep(w->service);
		}break;
		case SCHED_RELOAD:svc_reload_service(w->service);break;
		case SCHED_RESTART:svc_restart_service(w->service);break;
		default:return NULL;
	}
	list*cur,*next;
	pthread_mutex_lock(&queue_lock);
	if((next=list_first(queue)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct scheduler_work*);
		if(s!=w)continue;
		list_remove_free(cur,free_scheduler_work);
		break;
	}while(next);
	pthread_mutex_unlock(&queue_lock);
	return NULL;
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
	pthread_mutex_init(&queue_lock,NULL);
	struct scheduler_msg msg;
	bool run=true;
	while(run){
		ssize_t s=read(fds[0],&msg,sizeof(msg));
		if(s<0&&errno!=EINTR)break;
		if(s!=sizeof(msg))continue;
		switch(msg.action){
			case SCHED_EXIT:run=false;
			case SCHED_UNKNOWN:break;
			case SCHED_CHILD:
				svc_on_sigchld(
					msg.data.exit.pid,
					msg.data.exit.stat
				);
				run_queue();
			break;
			case SCHED_START:
			case SCHED_STOP:
			case SCHED_RELOAD:
			case SCHED_RESTART:
				add_queue(msg.data.service,msg.action);
				run_queue();
			break;
			case SCHED_STOP_ALL:
				add_all_stop_queue();
			default:;
		}
		memset(&msg,0,sizeof(struct scheduler_msg));
	}
	if(!run){
		close(fds[0]);
		close(fds[1]);
	}
	tlog_info("scheduler exit");
	pthread_mutex_destroy(&queue_lock);
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
	if(socketpair(AF_UNIX,SOCK_STREAM,0,fds)<0)return -errno;
	pthread_mutex_init(&lock,NULL);
	if(pthread_create(&scheduler,NULL,scheduler_thread,NULL)!=0)return -errno;
	pthread_setname_np(scheduler,NAME);
	return 0;
}

int oper_scheduler(struct scheduler_msg*data){
	pthread_mutex_lock(&lock);
	write(fds[1],data,sizeof(struct scheduler_msg));
	pthread_mutex_unlock(&lock);
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

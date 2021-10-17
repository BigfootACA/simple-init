#include<errno.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include"pool.h"
#include"list.h"
#include"init.h"
#include"defines.h"
#include"service.h"
#include"service_scheduler.h"
#define TAG "service"
#define NAME "Service Scheduler"

int add_queue(struct service*svc,enum scheduler_action act){
	list*cur,*next;
	switch(svc->status){
		case STATUS_STARTING:
		case STATUS_STOPPING:return 0;
		default:;
	}
	switch(act){
		case SCHED_START:switch(svc->status){
			case STATUS_STARTED:
			case STATUS_RUNNING:
				return 0;
			default:;
		}break;
		case SCHED_STOP:switch(svc->status){
			case STATUS_FAILED:
				if(svc->mode==WORK_ONCE)break;
				// fallthrough
			case STATUS_STOPPED:
				return 0;
			default:;
		}break;
		default:;
	}
	pthread_mutex_lock(&queue_lock);
	if(!(next=list_first(queue)))goto unlock;
	else do{
		cur=next;
		LIST_DATA_DECLARE(s,cur,struct scheduler_work*);
		if(!s)continue;
		if(s->service==svc){
			pthread_mutex_unlock(&queue_lock);
			ERET(EINPROGRESS);
		}
	}while((next=cur->next));
	struct scheduler_work*work=malloc(sizeof(struct scheduler_work));
	if(!work){
		telog_error("failed to create work");
		goto unlock;
	}
	work->service=svc,work->action=act;
	if(list_push_new(queue,work)<0){
		telog_error("add work to queue failed");
		goto unlock;
	}
	pthread_mutex_unlock(&queue_lock);
	int e=0;
	switch(work->action){
		case SCHED_START:
			if(!svc->depends_on||!(next=list_first(svc->depends_on)))break;
			do{
				cur=next;
				if(!cur->data)continue;
				LIST_DATA_DECLARE(s,cur,struct service*);
				if(add_queue(s,act)!=0)e=errno;
			}while((next=cur->next));
		break;
		case SCHED_STOP:
			if(!svc->depends_of||!(next=list_first(svc->depends_of)))break;
			do{
				cur=next;
				if(!cur->data)continue;
				LIST_DATA_DECLARE(s,cur,struct service*);
				if(add_queue(s,act)!=0)e=errno;
			}while((next=cur->next));
		break;
		default:;
	}
	return e;
	unlock:
	pthread_mutex_unlock(&queue_lock);
	return -1;
}

static bool task_can_run(struct scheduler_work*w){
	list*cur,*next;
	if(!w||!w->service)return false;
	switch(w->service->status){
		case STATUS_STARTING:
		case STATUS_STOPPING:return false;
		default:;
	}
	switch(w->action){
		case SCHED_START:
			if(!w->service->depends_on)return true;
			if(!(next=list_first(w->service->depends_on)))return false;
			do{
				cur=next;
				if(!cur->data)continue;
				switch(((struct service*)cur->data)->status){
					case STATUS_STOPPED:
					case STATUS_FAILED:
					case STATUS_STOPPING:
					case STATUS_STARTING:
						return false;
					default:;
				}
			}while((next=cur->next));
			break;
		case SCHED_STOP:
			if(!w->service->depends_of)return true;
			if(!(next=list_first(w->service->depends_of)))return false;
			do{
				cur=next;
				if(!cur->data)continue;
				switch(((struct service*)cur->data)->status){
					case STATUS_STARTED:
					case STATUS_RUNNING:
					case STATUS_STOPPING:
					case STATUS_STARTING:
						return false;
					default:;
				}
			}while((next=cur->next));
		default:;
	}
	return true;
}

int run_queue(){
	list*cur,*next;
	pthread_mutex_lock(&queue_lock);
	if(!(next=list_first(queue))){
		pthread_mutex_unlock(&queue_lock);
		return -1;
	}else do{
		cur=next;
		LIST_DATA_DECLARE(w,cur,struct scheduler_work*);
		if(task_can_run(w))pool_add(service_workers,scheduler_worker,w);
	}while((next=cur->next));
	pthread_mutex_unlock(&queue_lock);
	return 0;
}

int add_all_stop_queue(){
	list*cur,*next;
	pthread_mutex_lock(&queue_lock);
	if(!(next=list_first(queue))){
		pthread_mutex_unlock(&queue_lock);
		return -1;
	}else do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(w,cur,struct scheduler_work*);
		if(!w)continue;
		if(w->action!=SCHED_STOP)list_remove_free(cur,free_scheduler_work);
	}while(next);
	pthread_mutex_unlock(&queue_lock);
	if((next=list_first(services)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(!s||!s->stop_on_shutdown)continue;
		switch(s->status){
			case STATUS_STARTED:
			case STATUS_RUNNING:add_queue(s,SCHED_STOP);break;
			default:;
		}
	}while(next);
	return 0;
}

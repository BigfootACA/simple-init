#ifndef SERVICE_SCHEDULER_H
#define SERVICE_SCHEDULER_H
#include"pool.h"
#include"service.h"

enum scheduler_action{
	SCHED_UNKNOWN=0,
	SCHED_EXIT,
	SCHED_CHILD,
	SCHED_DONE,
	SCHED_START,
	SCHED_STOP,
	SCHED_RELOAD,
	SCHED_RESTART,
	SCHED_STOP_ALL,
};

struct scheduler_msg{
	enum scheduler_action action;
	union{
		struct{
			pid_t pid;
			int stat;
		}exit;
		struct service*service;
		char name[128-sizeof(enum scheduler_action)];
	}data;
};

struct scheduler_work{
	enum scheduler_action action;
	struct service*service;
};

#endif

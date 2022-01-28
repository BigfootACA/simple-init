/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef SERVICE_SCHEDULER_H
#define SERVICE_SCHEDULER_H
#include"lock.h"
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

// src/service/scheduler.c: service workers thread pool
extern struct pool*service_workers;

// src/service/scheduler.c: lock for variable queue
extern mutex_t queue_lock;

// src/service/scheduler.c: service worker queue
extern list*queue;

// src/service/scheduler.c: send scheduler command
extern int oper_scheduler(struct scheduler_msg*data);

// src/service/scheduler.c: scheduler service worker
extern void*scheduler_worker(void*data);

// src/service/scheduler.c: free scheduler_work
extern int free_scheduler_work(void*d);

// src/service/scheduler.c: add service to queue
extern int add_queue(struct service*svc,enum scheduler_action act);

// src/service/scheduler.c: execute queue
extern int run_queue(void);

// src/service/scheduler.c: add all stop action to queue
extern int add_all_stop_queue(void);

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _POOL_H
#define _POOL_H
#include<stdbool.h>
#include<pthread.h>

// job struct
struct job{
	void*(*callback)(void *arg);
	void*arg;
	struct job*next;
};

// pool struct
struct pool{
	int t_size,q_max,q_cur;
	bool q_closed,p_closed;
	struct job *first,*last;
	pthread_t *pthreads;
	pthread_mutex_t mutex;
	pthread_cond_t q_empty,q_nempty,q_nfull;
};

// src/lib/pool.c: init a thread pool
extern struct pool*pool_init(int t_size,int q_max);

// src/lib/pool.c: init a thread pool with CPUs*2
extern struct pool*pool_init_cpus(int q_max);

// src/lib/pool.c: add to thread pool
extern int pool_add(struct pool*pool,void*(*callback)(void*arg),void*arg);

// src/lib/pool.c: destroy thread pool
extern int pool_destroy(struct pool*pool);

#endif

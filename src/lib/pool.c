/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<sys/sysinfo.h>
#include<sys/prctl.h>
#include"pool.h"

static void*_pool_main(void*arg){
	struct pool*pool=(struct pool*)arg;
	struct job*pjob=NULL;
	prctl(PR_SET_NAME,"Pool worker",0,0,0);
	for(;;){
		pthread_mutex_lock(&(pool->mutex));
		while((pool->q_cur==0)&&!pool->p_closed)
			pthread_cond_wait(&(pool->q_nempty),&(pool->mutex));
		if(pool->p_closed){
			pthread_mutex_unlock(&(pool->mutex));
			pthread_exit(NULL);
		}
		pool->q_cur--;
		pjob=pool->first;
		pool->first=pool->q_cur==0?(pool->last=NULL):pjob->next;
		if(pool->q_cur==0)pthread_cond_signal(&(pool->q_empty));
		if(pool->q_cur==pool->q_max-1)pthread_cond_broadcast(&(pool->q_nfull));
		pthread_mutex_unlock(&(pool->mutex));
		(*(pjob->callback))(pjob->arg);
		free(pjob);
		pjob=NULL;
	}
}

struct pool*pool_init(int t_size,int q_max){
	struct pool*pool=NULL;
	if(!(pool=malloc(sizeof(struct pool))))goto fail;
	if(!(pool->pthreads=malloc(sizeof(pthread_t)*t_size)))goto fail;
	pool->t_size=t_size;
	pool->q_max=q_max,pool->q_cur=0;
	pool->first=NULL,pool->last=NULL;
	pool->q_closed=false,pool->p_closed=false;
	if(pthread_mutex_init(&(pool->mutex),NULL))goto fail;
	if(pthread_cond_init(&(pool->q_empty),NULL))goto fail;
	if(pthread_cond_init(&(pool->q_nempty),NULL))goto fail;
	if(pthread_cond_init(&(pool->q_nfull),NULL))goto fail;
	int i;
	for(i=0;i<pool->t_size;++i)
		pthread_create(&(pool->pthreads[i]),NULL,_pool_main,(void*)pool);
	return pool;
	fail:
	if(pool){
		if(pool->pthreads)free(pool->pthreads);
		free(pool);
	}
	return NULL;
}

struct pool*pool_init_cpus(int q_max){
	int r=get_nprocs()*2;
	return pool_init(r>0?r:2,q_max);
}

int pool_add(struct pool*pool,void*(*callback)(void*arg),void*arg){
	if(!pool||!callback)return -1;
	pthread_mutex_lock(&(pool->mutex));
	while((pool->q_cur==pool->q_max)&&!(pool->q_closed||pool->p_closed))
		pthread_cond_wait(&(pool->q_nfull),&(pool->mutex));
	if(pool->q_closed||pool->p_closed){
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}
	struct job*pjob;
	if(!(pjob=(struct job*)malloc(sizeof(struct job)))){
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}
	pjob->callback=callback;
	pjob->arg=arg;
	pjob->next=NULL;
	if(!pool->first){
		pool->first=pool->last=pjob;
		pthread_cond_broadcast(&(pool->q_nempty));
	}else{
		pool->last->next=pjob;
		pool->last=pjob;
	}
	pool->q_cur++;
	pthread_mutex_unlock(&(pool->mutex));
	return 0;
}

int pool_destroy(struct pool*pool){
	if(!pool)return -1;
	pthread_mutex_lock(&(pool->mutex));
	if(pool->q_closed||pool->p_closed){
		pthread_mutex_unlock(&(pool->mutex));
		return -1;
	}
	pool->q_closed=true;
	while(pool->q_cur!=0)
		pthread_cond_wait(&(pool->q_empty),&(pool->mutex));
	pool->p_closed=true;
	pthread_mutex_unlock(&(pool->mutex));
	pthread_cond_broadcast(&(pool->q_nempty));
	pthread_cond_broadcast(&(pool->q_nfull));
	for(int i=0;i<pool->t_size;++i)
		pthread_join(pool->pthreads[i],NULL);
	pthread_mutex_destroy(&(pool->mutex));
	pthread_cond_destroy(&(pool->q_empty));
	pthread_cond_destroy(&(pool->q_nempty));
	pthread_cond_destroy(&(pool->q_nfull));
	free(pool->pthreads);
	struct job*p;
	while(pool->first){
		p=pool->first;
		pool->first=p->next;
		free(p);
	}
	free(pool);
	return 0;
}
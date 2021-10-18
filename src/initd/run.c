/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/wait.h>
#include<string.h>
#include<stdio.h>
#define TAG "run"
#include"logger.h"
#include"init.h"
#include"proctitle.h"

static bool run=true;
static pid_t p,r;
typedef void (*sighandler)(int);

static void _signal_handler(int c){
	if(r!=getpid())return;
	signal(c,_signal_handler);
	switch(c){
		case SIGINT:case SIGTERM:case SIGQUIT:
			run=false;
			kill(p,SIGTERM);
		break;
	}
}

static void add_signal(sighandler s){
	signal(SIGINT,s);
	signal(SIGTERM,s);
	signal(SIGQUIT,s);
}

int run_loop_program(char*path,char**args){
	add_signal(_signal_handler);
	while(run){
		int i=-1;
		p=fork();
		if(p<0)return terlog_error(-1,"fork");
		else if(p==0){
			add_signal(NULL);
			setsid();
			i=execv(path,args);
			_exit(i);
			return i;
		}else while(waitpid(p,&i,0)<0&&errno==EINTR);
		sleep(1);
	}
	return 0;
}

int fork_run(char*tag,bool wait,pid_t*pp,void*data,runnable_t*runnable){
	if(!tag||!runnable)ERET(EINVAL);
	pid_t pid;
	switch((pid=fork())){
		case -1:return erlog_warn(-errno,tag,"fork");
		case 0:break;
		default:
			if(pp)*pp=pid;
			return wait?wait_cmd(pid):0;
	}
	setproctitle("%s",tag);
	int ret=runnable(data);
	_exit(ret);
	return ret;
}

pid_t run_program(char*path,char**args){
	pid_t pp=fork();
	if(pp<0)return terlog_error(-1,"fork");
	else if(pp==0)exit(run_loop_program(path,args));
	else return pp;
}

int wait_cmd(pid_t pid){
	int ret=-1,st=0,sig;
	while(waitpid(pid,&st,0)!=pid&&errno!=ECHILD);
	if(WIFEXITED(st))ret=WEXITSTATUS(st);
	else if(WIFSIGNALED(st)&&(sig=WTERMSIG(st))>0){
		if(sig==SIGINT)putchar('\r');
		else puts(strsignal(sig));
		ret=128|sig;
	}
	return ret;
}

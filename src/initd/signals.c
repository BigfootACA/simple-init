/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/reboot.h>
#include"system.h"
#include"logger.h"
#include"service.h"
#include"init_internal.h"
#define TAG "signals"

static bool handle=true;

#ifdef __GLIBC__
#include<execinfo.h>
#define BACKTRACE_SIZE 16

static void dump(){
	void*b[BACKTRACE_SIZE];
	char**s;
	int n=backtrace(b,BACKTRACE_SIZE);
	if(!(s=backtrace_symbols(b,n))){
		telog_emerg("backtrace_symbols");
		return;
	}
	tlog_emerg("Stack trace: ");
	for(int j=2;j<n;j++)tlog_emerg("  #%-2d %s\n",j-2,s[j]);
	free(s);
}
#else
static inline void dump(void){};
#endif

static void show_shutdown(int sig,pid_t pid){
	if(status==INIT_SHUTDOWN||pid<=0)return;
	char buff[BUFSIZ]={0},*type;
	switch(sig){
		case SIGUSR1:type="halt",action=ACTION_HALT;break;
		case SIGTERM:type="reboot",action=ACTION_REBOOT;break;
		case SIGUSR2:type="poweroff",action=ACTION_POWEROFF;break;
		default:return;
	}
	memset(&actiondata,0,sizeof(union action_data));
	status=INIT_SHUTDOWN;
	tlog_notice(
		"receive %s signal (%s) from %s",
		type,signame(sig),
		get_commname(pid,buff,BUFSIZ,true)
	);
}

static void signal_handlers(int s,siginfo_t*i,void*d __attribute__((unused))){
	if(getpid()!=1||!handle)return;
	static bool crash=false;
	pid_t pid;
	int st;
	switch(s){
		case SIGCHLD:
			while((pid=waitpid(-1,&st,WNOHANG))!=0){
				if(errno==ECHILD)break;
				if(WIFEXITED(st))tlog_debug(
					"clean process pid %d, exit code %d",
					pid,WEXITSTATUS(st)
				);
				else if(WIFSIGNALED(st))tlog_debug(
					"clean process pid %d, killed by signal %s",
					pid,signame(WTERMSIG(st))
				);
				else tlog_debug("clean process pid %d",pid);
				service_sigchld(pid,st);
			}
		break;
		case SIGSEGV:case SIGABRT:case SIGILL:case SIGBUS:
			if(i->si_pid!=0)break;
			if(crash){
				tlog_emerg("crash again");
				_exit(s);
			}
			crash=true;
			tlog_emerg("init crashed by %s!",signame(s));
			init_process_socket(-1);
			status=INIT_SHUTDOWN;
			dump();
			kill_all();
			xsleep(3);
			sync();
			exit(s);
		case SIGINT:
			if(status==INIT_SHUTDOWN||i->si_pid!=0)break;
			memset(&actiondata,0,sizeof(union action_data));
			tlog_alert("receive ctrl-alt-del key, reboot...");
			status=INIT_SHUTDOWN,action=ACTION_REBOOT;
		break;
		case SIGPWR:
			if(status==INIT_SHUTDOWN||i->si_pid!=0)break;
			memset(&actiondata,0,sizeof(union action_data));
			tlog_alert("receive SIGPWR, halt...");
			status=INIT_SHUTDOWN,action=ACTION_HALT;
		break;
		case SIGUSR1:case SIGTERM:case SIGUSR2:
			show_shutdown(s,i->si_pid);
		break;
		default:break;
	}
}

void disable_signals(){
	handle=false;
}

void setup_signals(){
	tlog_debug("setting signals");
	action_signals((int[]){
		SIGINT,
		SIGHUP,
		SIGQUIT,
		SIGTERM,
		SIGCHLD,
		SIGALRM,
		SIGUSR1,
		SIGUSR2,
		SIGSEGV,
		SIGABRT,
		SIGILL,
		SIGBUS
	},12,signal_handlers);
	tlog_debug("disable ctrl-alt-delete.");
	reboot(RB_DISABLE_CAD);
}

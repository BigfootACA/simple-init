/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include"system.h"

const char*sigmap[]={
	[SIGHUP]    = "SIGHUP",
	[SIGINT]    = "SIGINT",
	[SIGQUIT]   = "SIGQUIT",
	[SIGILL]    = "SIGILL",
	[SIGTRAP]   = "SIGTRAP",
	[SIGABRT]   = "SIGABRT",
	[SIGBUS]    = "SIGBUS",
	[SIGFPE]    = "SIGFPE",
	[SIGKILL]   = "SIGKILL",
	[SIGUSR1]   = "SIGUSR1",
	[SIGSEGV]   = "SIGSEGV",
	[SIGUSR2]   = "SIGUSR2",
	[SIGPIPE]   = "SIGPIPE",
	[SIGALRM]   = "SIGALRM",
	[SIGTERM]   = "SIGTERM",
	#if defined(SIGSTKFLT)
	[SIGSTKFLT] = "SIGSTKFLT",
	#elif defined(SIGEMT)
	[SIGEMT]    = "SIGEMT",
	#endif
	[SIGCHLD]   = "SIGCHLD",
	[SIGCONT]   = "SIGCONT",
	[SIGSTOP]   = "SIGSTOP",
	[SIGTSTP]   = "SIGTSTP",
	[SIGTTIN]   = "SIGTTIN",
	[SIGTTOU]   = "SIGTTOU",
	[SIGURG]    = "SIGURG",
	[SIGXCPU]   = "SIGXCPU",
	[SIGXFSZ]   = "SIGXFSZ",
	[SIGVTALRM] = "SIGVTALRM",
	[SIGPROF]   = "SIGPROF",
	[SIGWINCH]  = "SIGWINCH",
	[SIGPOLL]   = "SIGPOLL",
	[SIGPWR]    = "SIGPWR",
	[SIGSYS]    = "SIGSYS"
};

void handle_signals(int*sigs,int len,void(*handler)(int)){
	sigset_t signals;
	struct sigaction sa;
	sigfillset(&signals);
	for(int i=0;i<len;i++)sigdelset(&signals,sigs[i]);
	sigprocmask(SIG_SETMASK,&signals,NULL);
	memset(&sa,0,sizeof(sa));
	sa.sa_handler=handler;
	for(int i=0;i<len;i++)sigaction(sigs[i],&sa,NULL);
}

void action_signals(int*sigs,int len,void(*action)(int,siginfo_t*,void*)){
	sigset_t signals;
	struct sigaction sa;
	sigfillset(&signals);
	for(int i=0;i<len;i++)sigdelset(&signals,sigs[i]);
	sigprocmask(SIG_SETMASK,&signals,NULL);
	memset(&sa,0,sizeof(sa));
	sa.sa_sigaction=action;
	sa.sa_flags=SA_SIGINFO;
	for(int i=0;i<len;i++)sigaction(sigs[i],&sa,NULL);
}

int reset_signals(){
	static const struct sigaction sa={
		.sa_handler=SIG_DFL,
		.sa_flags=SA_RESTART,
	};
	int r=0;
	sigset_t ss;
	for(int s=1;s<_NSIG;s++){
		if(s==SIGKILL||s==SIGSTOP)continue;
		if(sigaction(s,&sa,NULL)<0)r=-errno;
	}
	if(sigemptyset(&ss)<0)return -errno;
	if(sigprocmask(SIG_SETMASK,&ss,NULL)<0)return -errno;
	return r;
}

const char*signame(int sig){
	return sig<0||sig>=(int)sizeof(sigmap)?NULL:sigmap[sig];
}

unsigned int xsleep(unsigned int n){
	while(n>0)n=sleep(n);
	return 0;
}

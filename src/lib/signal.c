#include<signal.h>
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
	for(int i=0;i<len;i++)sigaction(sigs[i],&sa,NULL);
}

const char*signame(int sig){
	return sig<0||sig>=(int)sizeof(sigmap)?NULL:sigmap[sig];
}

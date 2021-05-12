#include<signal.h>
#include<string.h>
#include"system.h"
void handle_signals(void(*handler)(int)){
	sigset_t signals;
	struct sigaction sa;
	sigfillset(&signals);
	sigdelset(&signals,SIGINT);
	sigdelset(&signals,SIGHUP);
	sigdelset(&signals,SIGQUIT);
	sigdelset(&signals,SIGTERM);
	sigdelset(&signals,SIGCHLD);
	sigdelset(&signals,SIGALRM);
	sigdelset(&signals,SIGUSR1);
	sigdelset(&signals,SIGUSR2);
	sigprocmask(SIG_SETMASK,&signals,NULL);
	memset(&sa,0,sizeof(sa));
	sa.sa_handler=handler;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGHUP,&sa,NULL);
	sigaction(SIGQUIT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
	sigaction(SIGCHLD,&sa,NULL);
	sigaction(SIGALRM,&sa,NULL);
	sigaction(SIGUSR1,&sa,NULL);
	sigaction(SIGUSR2,&sa,NULL);
}
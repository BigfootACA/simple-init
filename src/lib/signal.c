#include<signal.h>
#include<string.h>
#include"system.h"
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
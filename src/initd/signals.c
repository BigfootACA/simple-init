#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/reboot.h>
#include"system.h"
#include"logger.h"
#include"init.h"
#define TAG "signals"

void signal_handlers(int s){
	if(getpid()!=1)return;
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
			}
		break;
		default:break;
	}
}

void setup_signals(){
	tlog_debug("setting signals");
	handle_signals((int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM,SIGCHLD,SIGALRM,SIGUSR1,SIGUSR2},8,signal_handlers);
	tlog_debug("disable ctrl-alt-delete.");
	reboot(RB_DISABLE_CAD);
}

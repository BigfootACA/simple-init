#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/reboot.h>
#include"system.h"
#include"logger.h"
#include"init.h"
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
	for(int j=0;j<n;j++)tlog_emerg("  #%-2d %s\n",j,s[j]);
	free(s);
}
#else
static inline void dump(void){};
#endif

void signal_handlers(int s){
	if(getpid()!=1||!handle)return;
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
		case SIGSEGV:case SIGABRT:case SIGILL:case SIGBUS:
			handle=false;
			tlog_emerg("init crashed by %s!",signame(s));
			dump();
			sleep(3);
			logger_exit();
			sync();
			exit(s);
		break;
		default:break;
	}
}

void setup_signals(){
	tlog_debug("setting signals");
	handle_signals((int[]){
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

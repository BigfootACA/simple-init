#include<unistd.h>
#include<stdbool.h>
#include<stdlib.h>
#define TAG "init"
#include"init.h"
#include"shell.h"
#include"system.h"
#include"defines.h"
#include"cmdline.h"
#include"proctitle.h"

static int system_boot(){
	int r;
	if((r=preinit())!=0)return trlog_emerg(r,"preinit failed with %d",r);
	tlog_info("init system start");
	setproctitle("init");
	chdir(_PATH_ROOT)==0?
		tlog_debug("change directory to root."):
		telog_warn("failed to change directory: %m.");

	setup_signals();
	init_environ();

	wait_logfs();

	boot(boot_options.config);

	return r;
}

int system_down(){
	tlog_alert("system is going down...");
	return 0;
}

static void wait_loggerd(){
	// shutdown loggered
	logger_exit();
}

int init_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	int r;

	// precheck
	if(getpid()!=1)return trlog_emerg(1,"must be run as PID 1.");
	if(getuid()!=0||geteuid()!=0)return trlog_emerg(1,"must be run as USER 0(root)");
	if(getgid()!=0||getegid()!=0)return trlog_emerg(1,"must be run as GROUP 0(root)");

	// start loggerd
	#ifdef ENABLE_KMOD
	insmod("unix",true);// load unix socket for loggerd
	#endif
	start_loggerd(NULL);
	atexit(wait_loggerd);
	tlog_info("init started");

	// boot
	if((r=system_boot())!=0)return r;

	// while
	#ifdef ENABLE_INITSHELL
	if(fork()==0)while(1)run_shell();
	#endif
	while(true)usleep(100000000);
	return 0;
}

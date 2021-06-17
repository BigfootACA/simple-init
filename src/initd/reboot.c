#include<signal.h>
#include<unistd.h>
#include<sys/reboot.h>
#include"init_internal.h"
#include"system.h"
#include"logger.h"
#include"devd.h"
#define TAG "reboot"

int call_reboot(long rb,char*cmd){
	tlog_emerg("call kernel reboot.");
	devd_call_quit();
	logger_exit();
	close_all_fd(NULL,0);
	adv_reboot(rb,cmd);
	return 0;
}

int kill_all(){
	kill(-1,SIGTERM);
	tlog_alert("sending SIGTERM to all proceesses...");
	sync();
	usleep(1000000);
	kill(-1,SIGKILL);
	tlog_alert("sending SIGKILL to all proceesses...");
	sync();
	usleep(500000);
	return 0;
}
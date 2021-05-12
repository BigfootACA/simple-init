#include<signal.h>
#include<unistd.h>
#include<sys/reboot.h>
#include"logger.h"
#define TAG "reboot"

int call_reboot(int rb){
	tlog_emerg("call kernel reboot.");
	sync();
	reboot(rb);
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
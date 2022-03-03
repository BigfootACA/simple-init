/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<signal.h>
#include<unistd.h>
#include"service.h"
#include"init_internal.h"
#include"system.h"
#include"logger.h"
#define TAG "reboot"

int call_reboot(enum reboot_cmd rb,char*cmd){
	tlog_emerg("call kernel reboot.");
	kill_all();
	adv_reboot(rb,cmd);
	return 0;
}

int kill_all(){
	service_wait_all_stop();
	xsleep(1);
	kill(-1,SIGTERM);
	tlog_alert("sending SIGTERM to all proceesses...");
	sync();
	xsleep(3);
	init_do_exit();
	xsleep(1);
	kill(-1,SIGKILL);
	tlog_alert("sending SIGKILL to all proceesses...");
	sync();
	xsleep(1);
	return 0;
}
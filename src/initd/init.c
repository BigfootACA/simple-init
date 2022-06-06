/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/prctl.h>
#include<sys/reboot.h>
#include<sys/sysmacros.h>
#include<linux/reboot.h>

#define TAG "init"
#include"init_internal.h"
#include"devd.h"
#include"confd.h"
#include"shell.h"
#include"system.h"
#include"defines.h"
#include"cmdline.h"
#include"service.h"
#include"language.h"
#include"proctitle.h"

enum init_status status;
enum init_action action;
union action_data actiondata;
static int sfd=-1;

static int system_boot(){
	int r;
	if((r=preinit())!=0)return trlog_emerg(r,"preinit failed with %d",r);
	tlog_info("init system start");
	setproctitle("init");
	chdir(_PATH_ROOT)==0?
		tlog_debug("change directory to root."):
		telog_warn("failed to change directory");

	setup_signals();
	init_environ();

	wait_logfs();
	wait_conffs();

	char*lang=confd_get_string("language",NULL);
	if(lang)lang_set(lang);
	return r;
}

int system_down(){
	tlog_notice("prepare system clean");
	service_wait_all_stop();
	if(action==ACTION_SWITCHROOT){
		#define root actiondata.newroot.root
		#define init actiondata.newroot.init
		char*realinit=init,*newinit=NULL;
		if(realinit[0]==0)realinit=NULL;
		if(!(newinit=search_init(realinit,root)))return -errno;
		tlog_info("found init %s in %s",newinit,root);
		return run_switch_root(root,newinit);
	}
	confd_save_file(NULL);
	tlog_alert("system is going down...");
	enum reboot_cmd cmd=REBOOT_RESTART;
	char*data=NULL;
	switch(action){
		case ACTION_REBOOT:
			if(actiondata.data[0]!=0){
				cmd=REBOOT_DATA,data=actiondata.data;
				tlog_notice("reboot with '%s'...",data);
			}
		break;
		case ACTION_HALT:cmd=REBOOT_HALT;break;
		case ACTION_POWEROFF:cmd=REBOOT_POWEROFF;break;
		default:;
	}
	call_reboot(cmd,data);
	sleep(3);
	return 0;
}

void init_do_exit(){
	if(getpid()!=1)return;
	static bool clean=false;
	if(clean)return;
	clean=true;

	// terminate service scheduler
	stop_scheduler();

	// shutdown loggerd
	logger_exit();

	// shutdown confd
	confd_quit();

	// shutdown devd
	devd_call_quit();

	// clean fd
	close_all_fd(NULL,0);

	// disable signal handlers
	disable_signals();
}

static void init_console(){
	mkdir(_PATH_DEV,0755);
	mknod(_PATH_DEV_CONSOLE,S_IFCHR|0600,makedev(5,1));
	mknod(_PATH_DEV_NULL,S_IFCHR|0600,makedev(1,3));
	close_all_fd(NULL,0);
	close(0);
	close(1);
	close(2);
	int fd=open(_PATH_DEV_CONSOLE,O_RDWR);
	if(fd<0)fd=open(_PATH_DEV_NULL,O_RDWR);
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);
}

int init_main(int argc __attribute__((unused)),char**argv){
	int r;

	// pre check
	if(
		getpid()!=1||
		getuid()!=0||geteuid()!=0||
		getgid()!=0||getegid()!=0
	)return invoke_internal_cmd_nofork_by_name("simple-init",argv);
	status=INIT_BOOT;

	init_console();

	// start loggerd
	insmod("unix",true);// load unix socket for loggerd
	if(start_loggerd(NULL)<0){
		telog_emerg("start loggerd failed");
		abort();
	}
	atexit(init_do_exit);
	tlog_info("init started");

	// set title
	setproctitle("init");
	prctl(PR_SET_NAME,"Init Daemon",0,0,0);

	// boot
	if((r=system_boot())!=0)return r;

	// init service framework
	service_init();

	// register all services
	init_register_all_service();

	// load all services from config
	svc_conf_parse_services("service.services");

	// listen init control socket
	if((sfd=listen_init_socket())<0){
		telog_emerg("listen init control socket failed");
		abort();
	}

	// start default service
	service_start(svc_default);

	running:
	status=INIT_RUNNING;
	while(status==INIT_RUNNING){
		if(sfd<=0)sleep(1);
		else if(init_process_socket(sfd)<0)sfd=-1;
	}
	if(status!=INIT_SHUTDOWN)goto running;
	init_process_socket(-1);

	return system_down();
}

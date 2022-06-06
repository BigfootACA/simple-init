/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include"ttyd.h"
#include"logger.h"
#include"uevent.h"
#include"system.h"
#include"devd_internal.h"
#define TAG "devd"

int devfd=-1;

int open_devd_socket(char*tag,char*path){
	if(devfd>=0)return devfd;
	struct sockaddr_un n={0};
	n.sun_family=AF_UNIX;
	strncpy(n.sun_path,path,sizeof(n.sun_path)-1);
	if((devfd=socket(AF_UNIX,SOCK_STREAM,0))<0)
		return erlog_error(-errno,tag,"cannot create socket");
	if(connect(devfd,(struct sockaddr*)&n,sizeof(n))<0){
		elog_error(tag,"cannot connect devd socket %s",n.sun_path);
		close(devfd);
		devfd=-1;
	}
	return devfd;
}

void close_devd_socket(){
	if(devfd<0)return;
	close(devfd);
	devfd=-1;
}

int devd_call_init(){
	return devd_command(DEV_INIT);
}

int devd_call_modalias(){
	return devd_command(DEV_MODALIAS);
}

int devd_call_modload(){
	return devd_command(DEV_MODLOAD);
}

int devd_call_quit(){
	return devd_command(DEV_QUIT);
}

int process_module(uevent*event){
	if(!event->devpath)return -1;
	char*mod=strchr(event->devpath+1,'/')+1;
	switch(event->action){
		case ACTION_ADD:tlog_debug("loaded module '%s'",mod);break;
		case ACTION_REMOVE:tlog_debug("unloaded module '%s'",mod);break;
		default:break;
	}
	return 0;
}

int process_tty(uevent*event){
	switch(event->action){
		case ACTION_ADD:tlog_debug("add tty '%s'",event->devname);break;
		case ACTION_REMOVE:tlog_debug("remove tty '%s'",event->devname);break;
		default:return 0;
	}
	check_open_default_ttyd_socket(false,TAG);
	ttyd_reload();
	return 0;
}

int process_uevent(uevent*event){
	if(!event)return -1;
	if(event->subsystem){
		if(strcmp(event->subsystem,"tty")==0)process_tty(event);
		if(strcmp(event->subsystem,"firmware")==0)process_firmware_load(event);
		if(strcmp(event->subsystem,"module")==0)process_module(event);
	}
	if(event->major>=0&&event->minor>=0)process_new_node(0,event);
	if(event->modalias)insmod(event->modalias,false);
	return 0;
}

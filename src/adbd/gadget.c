/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/mount.h>
#include<sys/prctl.h>
#include"adbd_internal.h"
#include"proctitle.h"
#include"system.h"
#include"gadget.h"
#include"logger.h"
#include"confd.h"
#include"init_internal.h"
#define TAG "adbd"

int gadget_add_func_adbd(gadget*gadget,char*name,char*path){
	struct adb_data data;
	gadget_func func;
	init_adb_data(&data);
	memset(&func,0,sizeof(func));
	func.function="ffs";
	func.name=name;
	if(gadget_add_function(gadget,&func)<0)
		return terlog_warn(-1,"add adbd gadget function failed");
	if(mkdir(path,0755)<0&&errno!=EEXIST)
		return terlog_warn(-1,"adbd gadget ffs mkdir %s failed",path);
	if(mount(name,path,"functionfs",0,NULL)<0)
		return terlog_warn(-1,"adbd gadget ffs mount %s failed",path);
	data.proto=PROTO_USB;
	strcpy(data.ffs,path);
	int fds[2];
	if(pipe(fds)<0)return terlog_warn(-1,"adbd pipe failed");
	pid_t p=fork();
	if(p<0)return terlog_warn(-1,"adbd fork failed");
	else if(p==0){
		data.notifyfd=fds[1];
		close_all_fd((int[]){data.notifyfd},1);
		open_socket_logfd_default();
		open_default_confd_socket(false,TAG);
		open_socket_initfd(DEFAULT_INITD,false);
		prctl(PR_SET_NAME,"ADB Daemon");
		setproctitle("initadbd");
		_exit(adbd_init(&data));
	}else{
		close(fds[1]);
		char d[4];
		do{errno=0;read(fds[0],d,2);}
		while(errno==EINTR);
		close(fds[0]);
	}
	kvlst_free(data.prop);
	tlog_info("adbd gadget initialized");
	return 0;
}

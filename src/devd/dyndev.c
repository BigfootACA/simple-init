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
#define TAG "devnoded"
#include"devd.h"
#include"uevent.h"
#include"logger.h"

int process_new_node(int devdfd,uevent*event){
	char path[PATH_MAX];
	if(
		!event->action||
		!event->devname||
		event->major<0||
		event->minor<0||
		devdfd<0
	)return -1;
	switch(event->action){
		case ACTION_ADD:return init_devtmpfs(_PATH_DEV);
		case ACTION_REMOVE:
			if(faccessat(devdfd,event->devname,F_OK,0)!=0)return -errno;
			if(unlinkat(devdfd,event->devname,0)!=0)
				return terlog_warn(-errno,"unlink %s/%s",_PATH_DEV,event->devname);
			memset(path,0,sizeof(path));
			snprintf(
				path,
				sizeof(path)-1,
				"%s/%d:%d",
				event->devtype,
				event->major,
				event->minor
			);
			unlinkat(devdfd,path,0);
			return trlog_debug(0,"remove device %s/%s",_PATH_DEV,event->devname);
		default:return -2;
	}
}

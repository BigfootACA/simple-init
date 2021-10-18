/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"logger.h"
#include"gadget.h"
#define TAG "gadget"

int gadget_stop(gadget*g){
	if(!g)return -1;
	int x=gadget_stop_fd(g->dir_fd);
	if(x>=0)tlog_info("stopped gadget device '%s'",g->name);
	else telog_error("stop gadget device '%s' failed",g->name);
	return x;
}

int gadget_start(gadget*g,const char*device){
	if(!g||!device)return -1;
	int x=gadget_start_fd(g->dir_fd,device);
	if(x>=0)tlog_info("started gadget device '%s' on UDC '%s'",g->name,device);
	else telog_error("start gadget device '%s' on UDC '%s' failed",g->name,device);
	return x;
}

int gadget_stop_fd(int fd){
	return gadget_write_udc(fd,"");
}

int gadget_start_fd(int fd,const char*device){
	return gadget_write_udc(fd,device);
}

int gadget_start_auto_fd(int fd){
	return gadget_start_fd(fd,gadget_find_udc());
}

int gadget_start_auto(gadget*g){
	return gadget_start(g,gadget_find_udc());
}

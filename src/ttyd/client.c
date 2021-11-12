/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include"logger.h"
#include"ttyd_internal.h"

int ttyd=-1;

int open_ttyd_socket(bool quiet,char*tag,char*path){
	if(ttyd>=0)close(ttyd);
	struct sockaddr_un n={0};
	n.sun_family=AF_UNIX;
	strncpy(n.sun_path,path,sizeof(n.sun_path)-1);
	if((ttyd=socket(AF_UNIX,SOCK_STREAM,0))<0)
		return quiet?-errno:erlog_error(-errno,tag,"cannot create socket");
	if(connect(ttyd,(struct sockaddr*)&n,sizeof(n))<0){
		if(!quiet)elog_error(tag,"cannot connect ttyd socket %s",n.sun_path);
		close(ttyd);
		ttyd=-1;
	}
	return ttyd;
}

int check_open_ttyd_socket(bool quiet,char*tag,char*path){
	return ttyd>=0?ttyd:open_ttyd_socket(quiet,tag,path);
}

int set_ttyd_socket(int fd){
	return ttyd=fd;
}

void close_ttyd_socket(){
	if(ttyd<0)return;
	close(ttyd);
	ttyd=-1;
}

static int ttyd_command(enum ttyd_action action,int code){
	struct ttyd_msg msg;
	ttyd_internal_send_code(ttyd,action,code);
	return ttyd_internal_read_msg(ttyd,&msg)&&msg.action==TTYD_OK?0:msg.code;
}

int ttyd_quit(){return ttyd_command(TTYD_QUIT,0);}

int ttyd_reload(){
	if(ttyd<0)ERET(ENOTCONN);
	errno=0;
	struct ttyd_msg msg,res;
	ttyd_internal_init_msg(&msg,TTYD_RELOAD);
	if(ttyd_internal_send(ttyd,&msg)<0)return -1;
	if(ttyd_internal_read_msg(ttyd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int ttyd_reopen(){
	if(ttyd<0)ERET(ENOTCONN);
	errno=0;
	struct ttyd_msg msg,res;
	ttyd_internal_init_msg(&msg,TTYD_REOPEN);
	if(ttyd_internal_send(ttyd,&msg)<0)return -1;
	if(ttyd_internal_read_msg(ttyd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

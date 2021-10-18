/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include"defines.h"
#include"output.h"
#include"init_internal.h"

int initfd=-1;

int set_initfd(int fd){
	return initfd=fd<0?initfd:fd;
}

void close_initfd(){
	if(initfd<0)return;
	close(initfd);
	initfd=-1;
}

int open_socket_initfd(char*path,bool quiet){
	if(!path)ERET(EINVAL);
	struct sockaddr_un addr;
	int sock;
	if((sock=socket(AF_UNIX,SOCK_STREAM,0))<0){
		if(!quiet)stderr_perror("cannot create socket");
		return -1;
	}
	memset(&addr,0,sizeof(addr));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path,path,sizeof(addr.sun_path)-1);
	if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0){
		if(!quiet)stderr_perror("cannot connect socket %s",path);
		close(sock);
		return -1;
	}
	return set_initfd(sock);
}

int init_send_raw(struct init_msg*send){
	return init_send_data(initfd,send);
}

int init_recv_raw(struct init_msg*response){
	return init_recv_data(initfd,response);
}

int init_send(struct init_msg*send,struct init_msg*response){
	if(!send||!response)ERET(EINVAL);
	if(init_send_raw(send)!=0)return -errno;
	do{if(init_recv_raw(response))return -errno;}
	while(
		response->action!=ACTION_OK&&
		response->action!=ACTION_FAIL
	);
	return 0;
}
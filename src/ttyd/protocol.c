/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include"logger.h"
#include"ttyd_internal.h"
#define TAG "ttyd"

int ttyd_listen_socket(){
	int fd,er;
	struct sockaddr_un un={.sun_family=AF_UNIX};
	if(strlen(tty_sock)>=sizeof(un.sun_path))return trlog_error(-ENAMETOOLONG,"invalid socket path");
	strcpy(un.sun_path,tty_sock);
	if(access(un.sun_path,F_OK)==0)return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
	else if(errno!=ENOENT)return terlog_error(-errno,"failed to access %s",un.sun_path);
	if((fd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)return terlog_error(-errno,"cannot create socket");
	struct tty_data*new_data=malloc(sizeof(struct tty_data));
	if(!new_data){
		telog_warn("malloc failed");
		goto fail;
	}
	if(bind(fd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}
	if(listen(fd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}
	chmod(un.sun_path,0600);
	tlog_info("listen socket %s as %d",tty_sock,fd);
	memset(new_data,0,sizeof(struct tty_data));
	new_data->fd=fd;
	new_data->type=FD_SERVER;
	new_data->ev.events=EPOLLIN;
	new_data->ev.data.ptr=new_data;
	epoll_ctl(tty_epoll_fd,EPOLL_CTL_ADD,new_data->fd,&new_data->ev);
	return fd;
	fail:
	er=errno;
	close(fd);
	unlink(un.sun_path);
	if(new_data)free(new_data);
	ERET(er);
}

bool ttyd_internal_check_magic(struct ttyd_msg*msg){
	return msg&&msg->magic0==TTYD_MAGIC0&&msg->magic1==TTYD_MAGIC1;
}

void ttyd_internal_init_msg(struct ttyd_msg*msg,enum ttyd_action action){
	if(!msg)return;
	memset(msg,0,sizeof(struct ttyd_msg));
	msg->magic0=TTYD_MAGIC0;
	msg->magic1=TTYD_MAGIC1;
	msg->action=action;
}

int ttyd_internal_send(int fd,struct ttyd_msg*msg){
	if(fd<0)ERET(EINVAL);
	static size_t xs=sizeof(struct ttyd_msg);
	return ((size_t)write(fd,msg,xs))==xs?(int)xs:-1;
}

int ttyd_internal_send_code(int fd,enum ttyd_action action,int code){
	struct ttyd_msg msg;
	if(fd<0)ERET(EINVAL);
	ttyd_internal_init_msg(&msg,action);
	msg.code=code;
	return ttyd_internal_send(fd,&msg);
}

int ttyd_internal_read_msg(int fd,struct ttyd_msg*buff){
	if(!buff||fd<0)ERET(EINVAL);
	size_t size=sizeof(struct ttyd_msg),s;
	memset(buff,0,size);
	errno=0;
	while(1){
		errno=0;
		s=read(fd,buff,size);
		if(errno==0)break;
		switch(errno){
			case EINTR:continue;
			case EAGAIN:return 0;
			default:return -2;
		}
	}
	if(s==0)return EOF;
	return (s!=size||!(ttyd_internal_check_magic(buff)))?-2:1;
}

const char*ttyd_action2name(enum ttyd_action action){
	switch(action){
		case TTYD_OK:     return "OK";
		case TTYD_FAIL:   return "Failed";
		case TTYD_QUIT:   return "Quit";
		case TTYD_RELOAD: return "Reload";
		case TTYD_REOPEN: return "Reopen";
		default:          return "Unknown";
	}
}

void ttyd_epoll_server(struct tty_data*data){
	int n=accept(data->fd,NULL,NULL);
	if(n<0){
		telog_warn("ttyd socket accept failed");
		epoll_ctl(tty_epoll_fd,EPOLL_CTL_DEL,data->fd,&data->ev);
		free(data);
		return;
	}
	struct tty_data*new_data=malloc(sizeof(struct tty_data));
	if(!new_data){
		telog_warn("malloc failed");
		close(n);
		return;
	}
	memset(new_data,0,sizeof(struct tty_data));
	fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
	new_data->fd=n;
	new_data->type=FD_CLIENT;
	new_data->ev.events=EPOLLIN;
	new_data->ev.data.ptr=new_data;
	epoll_ctl(tty_epoll_fd,EPOLL_CTL_ADD,new_data->fd,&new_data->ev);
}

void ttyd_epoll_client(struct tty_data*data){
	struct ttyd_msg msg,ret;
	ssize_t s=ttyd_internal_read_msg(data->fd,&msg);
	if(s<=0){
		if(s<0){
			if(errno!=0)telog_warn("ttyd socket %d read failed",data->fd);
			epoll_ctl(tty_epoll_fd,EPOLL_CTL_DEL,data->fd,&data->ev);
			close(data->fd);
			free(data);
		}
		return;
	}
	ttyd_internal_init_msg(&ret,TTYD_OK);
	int retdata=0;
	switch(msg.action){
		// command response
		case TTYD_OK:case TTYD_FAIL:break;

		// terminate ttyd
		case TTYD_QUIT:
			tlog_notice("receive exit request");
			exit(0);
		break;

		// reload tty from confd
		case TTYD_RELOAD:
			tlog_notice("receive reload request");
			tty_conf_add_all();
			//fallthrough

		// reopen all tty
		case TTYD_REOPEN:
			tty_reopen_all();
		break;

		// unknown
		default:telog_warn(
			"action %s(0x%X) not implemented",
			ttyd_action2name(msg.action),msg.action
		);
	}
	if(retdata==0&&errno!=0)retdata=errno;
	ret.code=retdata;
	ttyd_internal_send(data->fd,&ret);
}

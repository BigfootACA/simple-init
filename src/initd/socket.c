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
#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include"init_internal.h"
#include"list.h"
#include"logger.h"
#include"defines.h"
#define TAG "init"

struct{
	int efd,size;
	list*clients;
	struct epoll_event*evs;
}ep={
	.efd=-1,
	.size=64,
	.clients=NULL,
	.evs=NULL
};

int listen_init_socket(){
	static struct sockaddr_un un={
		.sun_family=AF_UNIX,
		.sun_path=DEFAULT_INITD
	};
	int er,o=1,sfd;

	// create socket
	if((sfd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)
		return terlog_error(-errno,"cannot create socket");

	// check socket exists
	if(access(un.sun_path,F_OK)==0){
		if(connect(sfd,(struct sockaddr*)&un,sizeof(un))!=0||errno!=ECONNREFUSED){
			close(sfd);
			return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
		}else unlink(un.sun_path);
	}else if(errno!=ENOENT){
		close(sfd);
		return terlog_error(-errno,"failed to access %s",un.sun_path);
	}

	// bind socket
	if(bind(sfd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}

	// listen socket
	if(listen(sfd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}

	// change permission
	chmod(un.sun_path,0600);
	chown(un.sun_path,0,0);

	// require ucred
	setsockopt(sfd,SOL_SOCKET,SO_PASSCRED,&o,sizeof(o));
	return sfd;
	fail:
	er=errno;
	if(er<=0)er=1;
	close(sfd);
	unlink(un.sun_path);
	ERET(-er);
}

static int free_client(void*p){
	struct init_client*clt=p;
	if(clt){
		close(clt->fd);
		free(clt);
	}
	return 0;
}

static inline int ctl_fd(int act,struct init_client*clt){
	static struct epoll_event s={.events=EPOLLIN};
	struct epoll_event*p=NULL;
	int fd=clt->fd;
	switch(act){
		case EPOLL_CTL_ADD:
			s.data.ptr=clt,p=&s;
			list_obj_add_new(&ep.clients,clt);
		break;
		case EPOLL_CTL_DEL:
			list_obj_del_data(&ep.clients,clt,free_client);
		break;
	}
	return epoll_ctl(ep.efd,act,fd,p);
}

static int clean_epoll(){
	if(ep.efd>=0)close(ep.efd);
	if(ep.evs)free(ep.evs);
	list_free_all(ep.clients,free_client);
	ep.efd=-1,ep.evs=NULL,ep.clients=NULL;
	return 0;
}

static int init_epoll(int sfd){
	static size_t es=sizeof(struct epoll_event);
	if((ep.efd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(ep.evs=malloc(es*ep.size)))
		return terlog_error(-errno,"malloc failed");
	memset(ep.evs,0,es*ep.size);
	struct init_client*clt;
	if(!(clt=malloc(sizeof(struct init_client))))return 0;
	memset(clt,0,sizeof(struct init_client));
	clt->fd=sfd,clt->server=true;
	ctl_fd(EPOLL_CTL_ADD,clt);
	return 0;
}

static int recv_init_socket(struct init_client*clt){
	static socklen_t credsize=sizeof(struct ucred);
	struct init_msg msg;
	struct ucred cred;
	int z=init_recv_data(clt->fd,&msg);
	if(z<0&&errno==EAGAIN)return 0;
	if(
		z<0||getsockopt(
			clt->fd,SOL_SOCKET,SO_PEERCRED,
			&cred,&credsize
		)!=0||
		clt->cred.uid!=cred.uid||
		clt->cred.gid!=cred.gid||
		clt->cred.pid!=cred.pid
	){
		ctl_fd(EPOLL_CTL_DEL,clt);
		return -1;
	}
	return init_process_data(clt,&msg);
}

static int init_accept(int server){
	static socklen_t credsize=sizeof(struct ucred);
	struct init_client*clt;
	if(!(clt=malloc(sizeof(struct init_client))))return 0;
	memset(clt,0,sizeof(struct init_client));
	if((clt->fd=accept(server,NULL,NULL))<0){
		if(errno==EINTR||errno==EAGAIN)return 0;
		telog_error("accept on %d failed",server);
		close(server);
		return -1;
	}
	fcntl(clt->fd,F_SETFL,O_RDWR|O_NONBLOCK);
	if(getsockopt(
		clt->fd,SOL_SOCKET,SO_PEERCRED,
		&clt->cred,&credsize
	)!=0){
		ctl_fd(EPOLL_CTL_DEL,clt);
		return 0;
	}
	ctl_fd(EPOLL_CTL_ADD,clt);
	return 0;
}

int init_process_socket(int sfd){
	if(sfd<0)return clean_epoll();
	if(ep.efd<0&&init_epoll(sfd)<0)return -1;
	int r=epoll_wait(ep.efd,ep.evs,ep.size,-1);
	if(r==-1){
		if(errno==EINTR)return 0;
		return terlog_error(-1,"epoll failed");
	}else if(r==0)return 0;
	else for(int i=0;i<r;i++){
		struct init_client*clt=ep.evs[i].data.ptr;
		if(!clt)continue;
		if(!clt->server)recv_init_socket(clt);
		else if(init_accept(clt->fd)!=0)return -1;
	}
	return 0;
}

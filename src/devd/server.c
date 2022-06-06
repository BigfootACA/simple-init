/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<sys/un.h>
#include<sys/prctl.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include"devd_internal.h"
#include"proctitle.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"confd.h"
#include"devd.h"
#include"init.h"
#include"pool.h"
#define TAG "devd"

static int devdfd=-1;
static bool clean=false,run=true;

static int listen_devd_socket(){
	int fd,er;
	struct sockaddr_un un={
		.sun_family=AF_UNIX,
		.sun_path=DEFAULT_DEVD
	};
	if(access(un.sun_path,F_OK)==0)return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
	else if(errno!=ENOENT)return terlog_error(-errno,"failed to access %s",un.sun_path);
	if((fd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)return terlog_error(-errno,"cannot create socket");
	if(bind(fd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}
	if(listen(fd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}
	return fd;
	fail:
	er=errno;
	close(fd);
	unlink(un.sun_path);
	ERET(er);
}

static void devd_cleanup(int s __attribute__((unused))){
	if(clean)return;
	else clean=true,run=false;
	unlink(DEFAULT_DEVD);
	close(devdfd);
}

static void signal_handler(int s,siginfo_t *info,void*c __attribute__((unused))){
	if(info->si_pid<=1)return;
	devd_cleanup(s);
}

static void process_add(char*data){
	uevent event;
	uevent_parse(data,&event);
	process_uevent(&event);
	kvarr_free(event.environs);
}

static struct pool*pool;
struct save_data{
	int fd;
	struct devd_msg msg;
	char*data;
};
static void*process_thread(void*d){
	if(!d)EPRET(EINVAL);
	struct save_data*s=(struct save_data*)d;
	switch(s->msg.oper){
		case DEV_OK:case DEV_FAIL:break;

		// process kobject uevent
		case DEV_ADD:
			if(s->data)process_add(s->data);
		break;

		// scan /sys/dev and init /dev (create all nodes)
		case DEV_INIT:
			tlog_debug("receive init devtmpfs request");
			if(init_devtmpfs(_PATH_DEV)<0)telog_warn("init_devtmpfs failed");
		break;

		// scan /sys/devices and load all modalias
		case DEV_MODALIAS:
			tlog_debug("receive modalias request");
			if(load_modalias()<0)telog_warn("load_modalias failed");
		break;

		case DEV_MODLOAD:
			tlog_debug("receive modules load request");
			mods_conf_parse();
		break;

		// terminate devd
		case DEV_QUIT:run=false;break;
	}
	if(s->data)free(s->data);
	devd_internal_send_msg(s->fd,DEV_OK,NULL,0);
	free(s);
	return NULL;
}

static int _start_uevent_thread(void*d __attribute__((unused))){
	return uevent_netlink_thread();
}

static void ctl_fd(int efd,int oper,int fd){
	struct epoll_event ev;
	ev.events=EPOLLIN,ev.data.fd=fd;
	epoll_ctl(efd,oper,fd,oper==EPOLL_CTL_ADD?&ev:NULL);
	if(oper==EPOLL_CTL_DEL)close(fd);
}

static int devd_thread(int cfd){
	static size_t
		ss=sizeof(struct save_data),
		ds=sizeof(struct devd_msg),
		es=sizeof(struct epoll_event);
	int e=0,r,fd,efd;
	struct epoll_event*evs;
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	if((fd=listen_devd_socket())<0)return fd;
	tlog_info("devd start with pid %d",getpid());
	fork_run("netlink",false,NULL,NULL,_start_uevent_thread);
	signal(SIGCHLD,SIG_IGN);
	handle_signals((int[]){SIGUSR1,SIGUSR2,SIGCHLD},3,SIG_IGN);
	action_signals((int[]){SIGINT,SIGHUP,SIGTERM,SIGQUIT},4,signal_handler);
	if((devdfd=open(_PATH_DEV,O_RDONLY|O_DIRECTORY))<0)return terlog_warn(-errno,"open %s",_PATH_DEV);
	if((efd=epoll_create(64))<0)return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		e=-errno;
		goto ex;
	}
	if(!(pool=pool_init_cpus(8192))){
		telog_error("init pool failed");
		e=-errno;
		goto ex;
	}
	setproctitle("initdevd");
	prctl(PR_SET_NAME,"Device Daemon",0,0,0);
	ctl_fd(efd,EPOLL_CTL_ADD,fd);
	if(cfd>=0){
		devd_internal_send_msg(cfd,DEV_OK,NULL,0);
		close(cfd);
	}
	while(run){
		r=epoll_wait(efd,evs,64,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			e=-errno;
			goto ex;
		}else for(int i=0;i<r;i++){
			int f=evs[i].data.fd;
			if(f==fd){
				int n=accept(f,NULL,NULL);
				if(n<0)continue;
				fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
				ctl_fd(efd,EPOLL_CTL_ADD,n);
			}else for(;;){
				struct save_data*d=malloc(ss);
				if(!d)goto ex;
				memset(d,0,ss);
				ssize_t s=read(f,&d->msg,ds);
				if(s<0){
					if(errno==EINTR){
						free(d);
						continue;
					}else if(errno==EAGAIN){
						free(d);
						break;
					}else{
						telog_error("read %d failed",f);
						s=0;
					}
				}else if(
					(size_t)s!=ds||
					!devd_internal_check_magic(&d->msg)||
					(d->msg.size>0&&!(d->data=devd_read_data(f,&d->msg)))
				)s=0;
				if(s==0){
					if(d->data)free(d->data);
					free(d);
					ctl_fd(efd,EPOLL_CTL_DEL,f);
					break;
				}else{
					d->fd=f;
					pool_add(pool,process_thread,d);
				}
			}
		}
	}
	ex:
	if(efd>=0)close(efd);
	if(evs)free(evs);
	devd_cleanup(0);
	return e;
}

int initdevd_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	return devd_thread(-1);
}

int start_devd(char*tag,pid_t*p){
	int fds[2],r;
	if(devfd>=0)ERET(EEXIST);
	if(pipe(fds)<0)return -errno;
	pid_t pid=fork();
	switch(pid){
		case -1:return -errno;
		case 0:
			close_all_fd((int[]){fds[1]},1);
			r=devd_thread(fds[1]);
			exit(r);
	}
	close(fds[1]);
	struct devd_msg msg;
	do{if(devd_internal_read_msg(fds[0],&msg)<0)ERET(EIO);}
	while(msg.oper!=DEV_OK);
	if(p)*p=pid;
	close(fds[0]);
	return open_default_devd_socket(tag);
}
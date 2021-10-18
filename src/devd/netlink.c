/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include<linux/netlink.h>
#include"system.h"
#include"logger.h"
#include"pathnames.h"
#include"proctitle.h"
#include"devd_internal.h"
#define TAG "kobject"

static int init_hotplug_sock(){
	int s;
	struct sockaddr_nl n={
		.nl_family=AF_NETLINK,
		.nl_groups=1
	};
	n.nl_pid=getpid();
	if((s=socket(AF_NETLINK,SOCK_DGRAM,NETLINK_KOBJECT_UEVENT))<0)
		return terlog_error(-errno,"cannot create socket");
	if(bind(s,(struct sockaddr*)&n,sizeof(n))<0){
		telog_error("cannot bind netlink uevent socket");
		close(s);
		return -1;
	}
	return s;
}

int uevent_netlink_thread(){
	static size_t es=sizeof(struct epoll_event),ms=sizeof(struct devd_msg);
	close_all_fd(NULL,0);
	open_socket_logfd_default();
	tlog_info("kobject uevent forwarder start with pid %d",getpid());
	setproctitle("uevent");
	prctl(PR_SET_NAME,"UEvent Forward",0,0,0);
	char buf[8192],*v;
	size_t vs;
	ssize_t s;
	bool run=true;
	struct devd_msg msg;
	struct epoll_event ev,*evs=NULL;
	int hs,ds=-1,efd=-1,e=-1,r;
	if((hs=init_hotplug_sock())<0)goto fail;
	if((ds=open_default_devd_socket(TAG))<0)goto fail;
	if((efd=epoll_create(2))<0)return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*2)))goto fail;
	ev.events=EPOLLIN;
	ev.data.fd=hs;epoll_ctl(efd,EPOLL_CTL_ADD,hs,&ev);
	ev.data.fd=ds;epoll_ctl(efd,EPOLL_CTL_ADD,ds,&ev);
	while(run){
		e=-2;
		r=epoll_wait(efd,evs,2,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			goto fail;
		}else for(int i=0;i<r;i++){
			int f=evs[i].data.fd;
			if(f==hs){
				memset(buf,0,8192);
				if((s=recv(hs,&buf,8191,0))<0)break;
				for(ssize_t x=0;x<s;x++)if(buf[x]==0)buf[x]='\n';
				if(!(v=strchr(buf,'\n')))continue;
				v++,vs=s-(v-(char*)buf)+1;
				memset(&msg,0,ms);
				devd_internal_send_msg(ds,DEV_ADD,v,vs);
				do{if(devd_internal_read_msg(ds,&msg)<0)goto fail;}
				while(msg.oper!=DEV_OK&&msg.oper!=DEV_FAIL);
			}else if(f==ds){
				if(devd_internal_read_msg(ds,&msg)<0)goto fail;
				if(msg.size>0)lseek(ds,(size_t)msg.size,SEEK_CUR);
				if(msg.oper==DEV_QUIT){
					run=false;
					break;
				}
			}
		}
	}
	e=0;
	fail:
	simple_file_write(_PATH_PROC_SYS"/kernel/hotplug",_PATH_USR_BIN"/hotplug");
	if(hs>=0)close(hs);
	if(ds>=0)close(ds);
	if(efd>=0)close(efd);
	if(evs)free(evs);
	return e;
}

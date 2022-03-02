/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<sys/klog.h>
#include<sys/prctl.h>
#include<sys/select.h>
#include<sys/sysinfo.h>
#define TAG "klog"
#include"str.h"
#include"list.h"
#include"init.h"
#include"logger.h"
#include"system.h"
#include"proctitle.h"
#include"kloglevel.h"
#include"pathnames.h"
#include"logger_internal.h"
#ifndef SEEK_DATA
#include<linux/fs.h>
#endif

static bool run=true;
static int klogfd=-1;
static time_t boot_time=0;
struct log_item*read_kmsg_item(struct log_item*b,int fd,bool toff){
	char item[8192]={0},*level,*ktime,*content,*p0,*p1,*p2;
	ssize_t s=read(fd,&item,8191);
	if(s<0){
		if(errno==EAGAIN)errno=0;
		return NULL;
	}

	// log level
	level=item;
	if(!(p0=strchr(level,',')))EPRET(EINVAL);
	p0[0]=0,p0++;
	b->level=logger_klevel2level(parse_int(level,DEFAULT_KERN_LEVEL));

	if(!(ktime=strchr(p0,',')))EPRET(EINVAL);
	ktime++;

	// time
	if(!(p1=strchr(ktime,',')))EPRET(EINVAL);
	p1[0]=0,p1++;
	b->time=toff?(time_t)parse_long(ktime,0)/1000000+boot_time:time(NULL);

	if(!(content=strchr(p1,';')))EPRET(EINVAL);
	content++;

	// content
	if(!(p2=strchr(content,'\n')))EPRET(EINVAL);
	p2[0]=0,p2++;
	strncpy(b->content,content,sizeof(b->content)-1);

	strcpy(b->tag,"kernel");
	b->pid=0;

	return b;
}

static void kmsg_thread_exit(int p __attribute__((unused))){
	if(klogfd>=0)close(klogfd);
	if(logfd>=0){
		close_logfd();
		telog_warn("klog thread finished");
	}
	klogctl(SYSLOG_ACTION_CONSOLE_ON,NULL,0);
	run=false;
}

static void signal_handler(int s,siginfo_t *info,void*c __attribute__((unused))){
	if(info->si_pid<=1)return;
	kmsg_thread_exit(s);
}

static int read_kmsg_thread(void*data __attribute__((unused))){
	close_all_fd((int[]){klogfd},1);
	open_socket_logfd_default();

	if(fcntl(klogfd,F_SETFL,0)<0)return terlog_error(-errno,"fcntl klog fd");
	if(lseek(klogfd,0,SEEK_END)<0)return terlog_error(-errno,"lseek klog fd");

	klogctl(SYSLOG_ACTION_CONSOLE_OFF,NULL,0);
	tlog_info("kernel log forwarder start with pid %d",getpid());
	setproctitle("klog");
	prctl(PR_SET_NAME,"Kernel Logger",0,0,0);
	action_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM,SIGUSR1,SIGUSR2},
		6,signal_handler
	);

	fd_set fs;
	struct log_item item;
	struct timeval timeout;
	int m=MAX(klogfd,logfd)+1;
	while(run){
		FD_ZERO(&fs);
		FD_SET(klogfd,&fs);
		FD_SET(logfd,&fs);
		timeout.tv_sec=1,timeout.tv_usec=0;
		int r=select(m,&fs,NULL,NULL,&timeout);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("select failed");
			break;
		}else if(r==0)continue;
		else if(FD_ISSET(klogfd,&fs)){
			if(read_kmsg_item(&item,klogfd,false))
				logger_write(&item);
		}else if(FD_ISSET(logfd,&fs)){
			struct log_msg l;
			int x=logger_internal_read_msg(logfd,&l);
			if(x<0){
				close_logfd();
				break;
			}else if(x==0)continue;
		}
	}
	kmsg_thread_exit(0);

	return errno;
}

int init_kmesg(){
	struct log_item log;
	struct log_buff*buff=NULL;
	list*head=NULL,*conts=NULL,*item=NULL;

	struct sysinfo info;
	sysinfo(&info);
	boot_time=time(NULL)-(time_t)(info.uptime/1000);

	if(logbuffer&&!(head=list_first(logbuffer)))return -errno;
	if((klogfd=open(_PATH_DEV_KMSG,O_RDONLY|O_NONBLOCK))<0)goto fail;
	if(lseek(klogfd,0,SEEK_DATA)<0)goto fail;

	while(read_kmsg_item(&log,klogfd,true)){
		if(!(buff=logger_internal_item2buff(&log)))goto fail;
		if(!(item=list_new(buff)))goto fail;
		if(conts)list_add(conts,item);
		conts=item;
		item=NULL,buff=NULL;
	}

	if(logbuffer)list_insert(head,conts);
	else logbuffer=conts;
	conts=NULL;

	int x=fork_run("klog",false,NULL,NULL,read_kmsg_thread);
	close(klogfd);
	return x;
	fail:
	if(conts)list_free_all(conts,logger_internal_free_buff);
	if(buff)free(buff);
	if(klogfd>=0)close(klogfd);
	return -errno;
}

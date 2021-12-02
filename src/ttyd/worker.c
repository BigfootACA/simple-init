/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<termios.h>
#include<sys/prctl.h>
#include<sys/ioctl.h>
#include"init.h"
#include"confd.h"
#include"shell.h"
#include"logger.h"
#include"system.h"
#include"proctitle.h"
#include"ttyd_internal.h"
#define TAG data->name

static struct tty_data*data=NULL;
static char*pid_key="runtime.ttyd.client";

static int worker_tty_open(){
	int tty_fd;
	if((tty_fd=openat(tty_dev_fd,data->name,O_RDWR))<0)
		return terlog_error(-1,"open tty %s failed",data->name);
	close(tty_dev_fd);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	dup2(tty_fd,STDIN_FILENO);
	dup2(tty_fd,STDOUT_FILENO);
	dup2(tty_fd,STDERR_FILENO);
	close(tty_fd);
	fchown(STDIN_FILENO,0,0);
	fchmod(STDIN_FILENO,0620);
	return 0;
}

static int worker_tty_set_speed(){
	int sp=confd_get_integer_dict(tty_rt_ttys,data->name,"speed",-1);
	if(sp<0)sp=confd_get_integer_dict(tty_conf_ttys,data->name,"speed",-1);
	if(sp<=0)return 0;
	data->speed=tty_speed_convert(sp);
	if(data->speed<0)return trlog_warn(0,"invalid speed %d",data->speed);
	if(data->speed!=B0)cfsetspeed(&data->attrs,data->speed);
	return 0;
}

static int worker_tty_init_attrs(){
	if(tcgetattr(STDIN_FILENO,&data->attrs)<0)
		return terlog_warn(-1,"tcgetattr on %s failed",data->name);
	alarm(5);
        tcdrain(STDIN_FILENO);
        alarm(0);
        tcflush(STDIN_FILENO,TCIOFLUSH);
	worker_tty_set_speed();
	data->attrs.c_cflag&=(0
		#ifdef CSTOPB
                |CSTOPB
		#endif
		#ifdef PARENB
		|PARENB
		#endif
		#ifdef PARODD
		|PARODD
		#endif
		#ifdef CMSPAR
                |CMSPAR
		#endif
		#ifdef CBAUD
                |CBAUD
		#endif
		#ifdef CBAUDEX
                |CBAUDEX
		#endif
		#ifdef CIBAUD
                |CIBAUD
		#endif
        );
        data->attrs.c_cflag|=CS8|HUPCL|CREAD;
	#ifdef CLOCAL
	if(tty_confd_get_boolean(data,"clocal",false))data->attrs.c_cflag|=CLOCAL;
	#endif
	#ifdef CRTSCTS
	if(tty_confd_get_boolean(data,"flow",false))data->attrs.c_cflag|=CRTSCTS;
	#endif
	data->attrs.c_iflag=0;
        data->attrs.c_lflag=0;
        data->attrs.c_oflag=OPOST|ONLCR;
        data->attrs.c_cc[VMIN]=1;
        data->attrs.c_cc[VTIME]=0;
        data->attrs.c_line=0;
	if(tcsetattr(STDIN_FILENO,TCSANOW,&data->attrs)<0)
		return terlog_warn(-1,"tcsetattr on %s failed",data->name);
	return 0;
}

static int worker_tty_init(){
	if(worker_tty_open()!=0)return -1;
	pid_t pid=setsid();
        if(pid<0){
                int fd;
                if((pid=getpid())!=getsid(0))
			return terlog_error(-1,"setsid failed");
                if((fd=open(_PATH_DEV"/tty",O_RDWR|O_NONBLOCK))>=0){
                        sighandler_t old=signal(SIGHUP,SIG_IGN);
                        ioctl(fd,TIOCNOTTY);
                        close(fd);
                        signal(SIGHUP,old);
                }
        }
	pid_t s=tcgetsid(STDIN_FILENO);
        if((s<0||s!=pid)&&ioctl(STDIN_FILENO,TIOCSCTTY,(long)1)<0)
		return terlog_error(-1,"TIOCSCTTY on %s failed",data->name);
	if(tcsetpgrp(STDIN_FILENO,pid)<0)
		return terlog_error(-1,"tcsetpgrp on %s failed",data->name);
	if(worker_tty_init_attrs()<0)return -1;
	data->init_attr=true;
	return 0;
}

static void fin_tty_attrs(){
	if(!data->init_attr)return;
	data->init_attr=false;
	data->attrs.c_iflag|=IXON|IXOFF;
	if(data->eol=='\r')data->attrs.c_iflag|=ICRNL;
	data->attrs.c_lflag|=ICANON|ISIG|ECHO|ECHOE|ECHOK|ECHOKE;
	data->attrs.c_cc[VINTR]=CTL('C');
	data->attrs.c_cc[VQUIT]=CTL('\\');
	data->attrs.c_cc[VKILL]=CTL('U');
	data->attrs.c_cc[VEOF]=CTL('D');
	data->attrs.c_cc[VEOL]='\n';
	data->attrs.c_cc[VSWTC]=0;
	if(tcsetattr(STDIN_FILENO,TCSANOW,&data->attrs)<0)
		telog_warn("tcsetattr on %s failed",data->name);
}

static void worker_exit(){
	confd_delete_base(pid_key,data->name);
	check_open_default_ttyd_socket(false,TAG);
	ttyd_reopen();
	fin_tty_attrs();
	tlog_debug("tty worker for %s exited",data->name);
	_exit(0);
}

static int tty_worker(){
	char title[128]={0};
	if(!data||!data->name[0]||data->worker<=0)return -1;
	snprintf(title,127,"TTY Client %s",data->name);
	setproctitle("ttyc %s",data->name);
	prctl(PR_SET_NAME,title,0,0,0);
	atexit(worker_exit);
	if(worker_tty_init()<0)
		return trlog_error(-1,"%s init failed",data->name);
	tlog_debug("tty worker for %s started",data->name);
	if(tty_confd_get_boolean(data,"clear",true))
		puts("\033[H\033[2J\033[3J");
	if(tty_confd_get_boolean(data,"issue",true))
		tty_issue_write(STDOUT_FILENO,data);
	if(tty_login(data)){
		pid_t p=fork();
		switch(p){
			case -1:
				perror("fork failed");
				tlog_warn("fork failed");
			break;
			case 0:
				fin_tty_attrs();
				_exit(tty_start_session(data));
			default:return wait_cmd(p);
		}
	}else sleep(10);
	return 0;
}

int tty_start_worker(struct tty_data*d){
	if(!d||!d->name[0])return -1;
	int p=fork();
	if(p<0)return -1;
	else if(p==0){
		close_logfd();
		close_initfd();
		close_ttyd_socket();
		close_confd_socket();
		close_all_fd((int[]){tty_dev_fd},1);
		open_socket_logfd_default();
		open_default_confd_socket(false,TAG);
		setsid();
		p=fork();
		if(p<0)_exit(telog_error("fork failed"));
		else if(p==0){
			data=d,d->worker=getpid();
			exit(tty_worker());
		}else{
			confd_set_integer_base(pid_key,d->name,p);
			d->worker=p;
			_exit(0);
		}
	}else return wait_cmd(p);
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<sys/prctl.h>
#include"confd.h"
#include"getopt.h"
#include"logger.h"
#include"system.h"
#include"output.h"
#include"service.h"
#include"pathnames.h"
#include"proctitle.h"
#include"ttyd_internal.h"
#define TAG "ttyd"

static bool protect=false;

static void signal_handler(int s,siginfo_t*info,void*c __attribute__((unused))){
	if(info->si_pid<=1&&protect)return;
	tlog_info("ttyd exiting with signal %d",s);
	unlink(DEFAULT_TTYD);
	exit(0);
}

void ttyd_epoll_tty(struct tty_data*data){
	epoll_ctl(tty_epoll_fd,EPOLL_CTL_DEL,data->fd,&data->ev);
	tcflush(STDOUT_FILENO,TCIFLUSH);
	close(data->fd);
	memset(&data->ev,0,sizeof(data->ev));
	data->fd=-1;
	tty_start_worker(data);
}

int ttyd_thread(){
	static size_t es=sizeof(struct epoll_event);
	int r,e=0;
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	tlog_info("ttyd start with pid %d",getpid());
	struct epoll_event*evs;
	setproctitle("ttyd");
	prctl(PR_SET_NAME,"TTY Daemon",0,0,0);
	action_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM,SIGUSR1,SIGUSR2},
		6,signal_handler
	);
	signal(SIGCHLD,SIG_IGN);
        if((tty_dev_fd=open(_PATH_DEV,O_DIR|O_CLOEXEC))<0)
		exit(terlog_error(-errno,"open "_PATH_DEV" failed"));
	if((tty_epoll_fd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		e=-errno;
		goto ex;
	}
	ttyd_listen_socket();
	tty_conf_init();
	tty_conf_add_all();
	memset(evs,0,es*64);
	while(1){
		r=epoll_wait(tty_epoll_fd,evs,64,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			e=-1;
			goto ex;
		}else if(r==0)continue;
		else for(int i=0;i<r;i++){
			struct tty_data*data=evs[i].data.ptr;
			if(data)switch(data->type){
				case FD_TTY:ttyd_epoll_tty(data);break;
				case FD_SERVER:ttyd_epoll_server(data);break;
				case FD_CLIENT:ttyd_epoll_client(data);break;
			}
		}
	}
	ex:
	exit(e);
}

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: ttyd [OPTION]...\n"
		"Start TTY daemon.\n\n"
		"Options:\n"
		"\t-d, --daemon           Run in daemon\n"
		"\t-h, --help             Display this help and exit\n"
	);
}

int ttyd_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	static const struct option lo[]={
		{"help",   no_argument, NULL,'h'},
		{"daemon", no_argument, NULL,'d'},
		{NULL,0,NULL,0}
	};
	int o;
	bool daemon=false;
	while((o=b_getlopt(argc,argv,"hd",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'd':daemon=true;break;
		default:return 1;
	}
	if(daemon){
		chdir("/");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
		if(setsid()<0)return re_err(1,"setsid");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
	}
	return ttyd_thread();
}

static int ttyd_startup(struct service*svc __attribute__((unused))){
	return ttyd_main(0,NULL);
}

int register_ttyd(){
	struct service*ttyd=svc_create_service("ttyd",WORK_FOREGROUND);
	if(ttyd){
		svc_set_desc(ttyd,"TTY Daemon");
		svc_set_start_function(ttyd,ttyd_startup);
		ttyd->auto_restart=true;
		svc_add_depend(svc_system,ttyd);
	}
	return 0;
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/un.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"service.h"
#include"defines.h"
#include"proctitle.h"
#include"pathnames.h"
#define TAG "service"

#define EGOTO(err) {r=err;goto end;}

static void write_close(int fd,int data){
	write(fd,&data,sizeof(data));
	close(fd);
}

static bool init_stdio(struct svc_exec*exec){
	if(!exec->prop.svc->stdio_syslog)return true;
	int sock,sin;
	struct sockaddr_un addr;
	if((sock=socket(AF_UNIX,SOCK_DGRAM,0))<0)return false;
	memset(&addr,0,sizeof(addr));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path,_PATH_DEV"/log",sizeof(addr.sun_path)-1);
	if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0){
		close(sock);
		return false;
	}
	if((sin=open(_PATH_DEV_NULL,O_RDWR))>0){
		close(STDIN_FILENO);
		dup2(sin,STDIN_FILENO);
		close(sin);
	}
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	dup2(sock,STDOUT_FILENO);
	dup2(sock,STDERR_FILENO);
	close(sock);
	return true;
}

static void _run_exec_child(struct svc_exec*exec,int fd){
	int r=0;
	close_all_fd((int[]){fd},1);
	char name[BUFSIZ]={0};
	snprintf(name,BUFSIZ-1,"Service %s executor",exec->prop.name);
	prctl(PR_SET_NAME,name);
	setproctitle("initsvc %s",exec->prop.name);
	if(
		setuid(exec->prop.uid)<0||
		seteuid(exec->prop.uid)<0||
		setgid(exec->prop.gid)<0||
		setegid(exec->prop.gid)<0
	)EGOTO(errno);
	switch(exec->prop.type){
		case TYPE_UNKNOWN:EGOTO(EINVAL);
		case TYPE_FUNCTION:
			if(!exec->exec.func)EGOTO(EINVAL);
			write_close(fd,0);
			if(!init_stdio(exec))EGOTO(errno);
			close_all_fd(NULL,0);
			reset_signals();
			set_confd_socket(-1);
			open_socket_logfd_default();
			r=exec->exec.func(exec->prop.svc);
			MUTEX_DESTROY(services_lock);
			svc_free_all_services(services);
			exit(r);
		case TYPE_COMMAND:
			if(!exec->exec.cmd.path)EGOTO(EINVAL);
			if(!exec->exec.cmd.args)EGOTO(EINVAL);
			if(exec->exec.cmd.environ)clearenv();
			write_close(fd,0);
			if(!init_stdio(exec))EGOTO(errno);
			close_all_fd(NULL,0);
			execvpe(
				exec->exec.cmd.path,
				exec->exec.cmd.args,
				exec->exec.cmd.environ?:environ
			);
			fprintf(stderr,"execute %s failed",exec->exec.cmd.path);
			fputs(errno==0?".\n":strerror(errno),stderr);
		return;
		case TYPE_LIBRARY:EGOTO(ENOSYS);
	}
	end:
	write_close(fd,r);
	_exit(r);
}

int svc_run_exec(struct svc_exec*exec){
	if(!exec||!exec->prop.svc)ERET(EINVAL);
	if(
		getuid()!=0||
		geteuid()!=0||
		getgid()!=0||
		getegid()!=0
	)ERET(EPERM);
	memset(&exec->status,0,sizeof(exec->status));
	int ps[2];
	if(pipe(ps)<0)return -errno;
	pid_t p=fork();
	if(p<0){
		int e=errno;
		close(ps[0]);
		close(ps[1]);
		ERET(e);
	}
	if(p==0){
		_run_exec_child(exec,ps[1]);
		return 0;
	}
	close(ps[1]);
	exec->status.pid=p;
	time(&exec->status.start);
	exec->status.active=exec->status.start;
	exec->status.running=true;
	int e=0;
	ssize_t re;
	do{errno=0;re=read(ps[0],&e,sizeof(int));}while((errno==EINTR)&&re<=0);
	if(re!=sizeof(int)){
		memset(&exec->status,0,sizeof(exec->status));
		return -(errno==0?EFAULT:errno);
	}
	if(e<0){
		exec->status.running=false;
		time(&exec->status.finish);
	}else if(e>0){
		exec->status.running=false;
		exec->status.exit_code=e;
		time(&exec->status.finish);
		tlog_warn(
			"failed to execute %s: %s",
			exec->prop.name,
			strerror(e)
		);
		ERET(e);
	}
	return 0;
}

int svc_check_exec_timeout(struct svc_exec*exec){
	if(!exec||!exec->prop.svc)ERET(EINVAL);
	struct service*svc=exec->prop.svc;
	if(
		exec->prop.timeout<=0||
		!exec->status.running||
		exec->status.pid<=0||(
			svc->status!=STATUS_STOPPING&&
			svc->status!=STATUS_STARTING
		)
	)return 0;
	time_t cur;
	time(&cur);
	if(cur-exec->status.active<exec->prop.timeout)return 0;
	exec->status.active=cur;
	tlog_notice("Service %s timeout",exec->prop.name);
	int sig=exec->status.timeout?SIGKILL:SIGTERM;
	if(exec==svc->start||exec==svc->reload)kill(exec->status.pid,sig);
	else if(exec==svc->stop||exec==svc->restart){
		kill(exec->status.pid,sig);
		if(!svc->process.finish)kill(svc->process.pid,sig);
		else if(svc->start->status.running)kill(svc->start->status.pid,sig);
		if(exec==svc->restart&&exec->status.timeout)service_start(svc);
	}
	exec->status.timeout=true;
	return 0;
}

int svc_check_all_exec_timeout(){
	list*cur,*next=list_first(services);
	if(next)do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(!s)continue;
		svc_check_exec_timeout(s->start);
		svc_check_exec_timeout(s->stop);
		svc_check_exec_timeout(s->restart);
		svc_check_exec_timeout(s->reload);
	}while(next);
	return 0;
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include"str.h"
#include"shell.h"
#include"logger.h"
#include"init_internal.h"
#include"adbd_internal.h"
#define TAG "adbd"
typedef struct stinfo stinfo;
struct stinfo{
	void(*func)(int fd,void*cookie);
	int fd;
	void*cookie;
};
void *service_bootstrap_func(void*x){
	stinfo *sti=x;
	sti->func(sti->fd,sti->cookie);
	free(sti);
	return 0;
}
void restart_tcp_service(int fd,void*cookie){
	char buf[100];
	char value[92];
	int port=*(int*)cookie;
	if(port<=0){
		snprintf(buf,sizeof(buf),"invalid port\n");
		writex(fd,buf,strlen(buf));
		close(fd);
		return;
	}
	snprintf(value,sizeof(value),"%d",port);
	snprintf(buf,sizeof(buf),"restarting in TCP mode port: %d\n",port);
	writex(fd,buf,strlen(buf));
	close(fd);
}
void restart_usb_service(int fd,void*cookie __attribute__((unused))){
	char buf[100];
	snprintf(buf,sizeof(buf),"restarting in USB mode\n");
	writex(fd,buf,strlen(buf));
	close(fd);
}
void reboot_service(int fd,void*arg){
	char buf[256];
	struct init_msg msg;
	size_t ss=sizeof(msg.data.data);
	init_initialize_msg(&msg,ACTION_REBOOT);
	if(arg&&strlen(arg)!=0){
		telog_notice("remote execute reboot with '%s'",(char*)arg);
		if(strlen(arg)>=ss){
			snprintf(buf,sizeof(buf),"arguments too long\n");
			goto fail;
		}
		strncpy(msg.data.data,arg,ss-1);
	}else telog_notice("remote execute reboot");
	errno=0;
	struct init_msg response;
	init_send(&msg,&response);
	if(errno!=0){
		snprintf(buf,sizeof(buf),"send command: %m\n");
		goto fail;
	}
	if(response.data.status.ret!=0){
		snprintf(
			buf,sizeof(buf),
			"reboot: %s\n",
			strerror(response.data.status.ret)
		);
		goto fail;
	}
	goto done;
	fail:
	writex(fd,buf,strlen(buf));
	done:
	close(fd);
}
static int create_service_thread(void(*func)(int,void*),void*cookie){
	stinfo *sti;
	pthread_t t;
	int s[2];
	if(adb_socketpair(s))
		return terlog_warn(-1,"cannot create service socket pair");
	if(!(sti=malloc(sizeof(stinfo)))){
		telog_error("cannot allocate stinfo");
		exit(-1);
	}
	sti->func=func;
	sti->cookie=cookie;
	sti->fd=s[1];
	if(adb_thread_create(&t,service_bootstrap_func,sti)!=0){
		free(sti);
		close(s[0]);
		close(s[1]);
		return terlog_warn(-1,"cannot create service thread");
	}
	return s[0];
}
static int create_subprocess(const char *cmd,const char *arg0,const char *arg1,pid_t *pid){
	char *devname;
	int ptm;
	if((ptm=adb_open(_PATH_DEV"/ptmx",O_RDWR))<0)
		return terlog_warn(-1,"cannot open %s",_PATH_DEV"/ptmx");
	fcntl(ptm,F_SETFD,FD_CLOEXEC);
	if(
		grantpt(ptm)||
		unlockpt(ptm)||
		((devname=(char*)ptsname(ptm))==0)
	){
		telog_warn("trouble with %s",_PATH_DEV"/ptmx");
		close(ptm);
		return -1;
	}
	*pid=fork();
	if(*pid<0){
		telog_warn("fork failed");
		close(ptm);
		return -1;
	}
	if(*pid==0){
		int pts;
		setsid();
		if((pts=adb_open(devname,O_RDWR))<0){
			telog_error("child failed to open pseudo-term slave %s",devname);
			exit(-1);
		}
		dup2(pts,0);
		dup2(pts,1);
		dup2(pts,2);
		close(pts);
		close(ptm);
		char text[64];
		snprintf(text,sizeof text,"/proc/%d/oom_score_adj",getpid());
		int fd;
		if((fd=adb_open(text,O_WRONLY))>=0){
			adb_write(fd,"0",1);
			close(fd);
		}else telog_warn("cannot open %s",text);
		char*g;
		if(!getenv("TERM"))setenv("TERM","xterm-256color",0);
		if((g=getenv("HOME"))&&chdir(g)<0)telog_warn("chdir to %s failed",g);
		char*exe=(char*)cmd,*name=(char*)cmd;
		#ifdef ENABLE_READLINE
		if(!cmd||strlen(cmd)==0)exe=_PATH_PROC_SELF"/exe",name="initshell";
		#else
		exe="/bin/sh",name="sh";
		#endif
		execl(exe,name,arg0,arg1,NULL);
		if(errno>0)telog_error("exec '%s' failed",name);
		exit(-1);
	}else return ptm;
}
static void subproc_waiter_service(int fd,void *cookie){
	pid_t pid=*(pid_t*)cookie;
	if(pid<=0){
		tlog_warn("subproc_waiter_service call with zero pid");
		return;
	}
	for(;;){
		int st;
		if((waitpid(pid,&st,0))==pid){
			if(WIFSIGNALED(st))telog_warn("subproc killed by signal %d",WTERMSIG(st));
			else if(!WIFEXITED(st))telog_warn("subproc did not exit! status %d",st);
			else if(WEXITSTATUS(st)>=0)telog_warn("subproc exit code %d",WEXITSTATUS(st));
			else continue;
			break;
		}
	}
	telog_debug("shell exited pid %d",pid);
	if(shell_exit_notify_fd>=0){
		writex(shell_exit_notify_fd,&fd,sizeof(fd));
		telog_debug("notified shell exit pid %d",pid);
	}
}
static int create_subproc_thread(const char*name){
	stinfo *sti;
	pthread_t t;
	int ret_fd;
	pid_t pid=0;
	char*shell=adbd_get_shell();
	ret_fd=name?create_subprocess(shell,"-c",name,&pid):create_subprocess(shell,"-",0,&pid);
	if(ret_fd<0||pid<=0)return ret_fd;
	if(!(sti=malloc(sizeof(stinfo)))){
		telog_error("cannot allocate stinfo");
		exit(-1);
	}
	sti->func=subproc_waiter_service;
	if(!(sti->cookie=malloc(sizeof(pid_t)))){
		free(sti);
		close(ret_fd);
		tlog_warn("cannot allocate pid");
		return -1;
	}
	*(pid_t*)sti->cookie=pid;
	sti->fd=ret_fd;
	if(adb_thread_create(&t,service_bootstrap_func,sti)){
		free(sti->cookie);
		free(sti);
		close(ret_fd);
		tlog_warn("cannot create service thread");
		return -1;
	}
	return ret_fd;
}
static int tcp_svc(char*arg){
	int ret;
	int port=parse_int(arg,0);
	if(strchr(arg,':')!=0)ret=-1;
	else if((ret=socket_loopback_client(port,SOCK_STREAM))>=0)disable_tcp_nagle(ret);
	return ret;
}
static int tcpip_svc(char*arg){
	int port;
	if((port=parse_int(arg,0))==0)port=0;
	return create_service_thread(restart_tcp_service,&port);
}
static int local_svc(char*arg){return socket_local_client(arg,ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM);}
static int local_reserved_svc(char*arg){return socket_local_client(arg,ANDROID_SOCKET_NAMESPACE_RESERVED,SOCK_STREAM);}
static int local_abstract_svc(char*arg){return socket_local_client(arg,ANDROID_SOCKET_NAMESPACE_ABSTRACT,SOCK_STREAM);}
static int local_filesystem_svc(char*arg){return socket_local_client(arg,ANDROID_SOCKET_NAMESPACE_FILESYSTEM,SOCK_STREAM);}
static int sync_svc(char*arg __attribute__((unused))){return create_service_thread(file_sync_service,NULL);}
static int usb_svc(char*arg __attribute__((unused))){return create_service_thread(restart_usb_service,NULL);}
static int reboot_svc(char*arg){return create_service_thread(reboot_service,arg);}
static int shell_svc(char*arg){return create_subproc_thread((arg&&*arg)?arg:NULL);}
static int dev_svc(char*arg){return adb_open(arg,O_RDWR);}
static struct adb_service{
	char name[128];
	int(*handle)(char*);
}services[]={
	{"tcp",             &tcp_svc              },
	{"tcpip",           &tcpip_svc            },
	{"local",           &local_svc            },
	{"localreserved",   &local_reserved_svc   },
	{"localabstract",   &local_abstract_svc   },
	{"localfilesystem", &local_filesystem_svc },
	{"dev",             &dev_svc              },
	{"shell",           &shell_svc            },
	{"sync",            &sync_svc             },
	{"reboot",          &reboot_svc           },
	{"usb",             &usb_svc              },
	{}
};
int service_to_fd(const char *value){
	char*val,*arg,*name;
	tlog_debug("request service %s",value);
	if(!(val=strdup(value)))
		return terlog_warn(-1,"cannot allocate name");
	if((arg=strchr(val,':')))*arg++=0;
	name=val;
	for(int i=0;services[i].handle;i++){
		if(strcmp(services[i].name,name)!=0)continue;
		int ret=services[i].handle(arg);
		if(ret>=0)fcntl(ret,F_SETFD,FD_CLOEXEC);
		free(val);
		return ret;
	}
	return terlog_warn(-1,"service %s not found",name);
}

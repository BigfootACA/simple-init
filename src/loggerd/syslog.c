/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<ctype.h>
#include<errno.h>
#include<stdlib.h>
#include<signal.h>
#include<string.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#define TAG "syslog"
#include"str.h"
#include"list.h"
#include"init.h"
#include"logger.h"
#include"system.h"
#include"kloglevel.h"
#include"pathnames.h"
#include"proctitle.h"
#include"logger_internal.h"

#define SOCKET_SYSLOG _PATH_DEV"/log"

static bool run=true;
static int sfd=-1;

static int listen_syslog_socket(){
	static struct sockaddr_un un={
		.sun_family=AF_UNIX,
		.sun_path=SOCKET_SYSLOG
	};
	int er,o=1;

	// create socket
	if((sfd=socket(AF_UNIX,SOCK_DGRAM|SOCK_NONBLOCK,0))<0)
		return terlog_error(-errno,"cannot create socket");

	// check socket exists
	if(access(un.sun_path,F_OK)==0){
		if(
			connect(sfd,(struct sockaddr*)&un,sizeof(un))!=0||
			errno!=ECONNREFUSED
		){
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

	// change permission
	chmod(un.sun_path,0666);
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

static void syslog_thread_exit(int p __attribute__((unused))){
	close_logfd();
	close(sfd);
	unlink(SOCKET_SYSLOG);
	run=false;
}

static void signal_handler(int s,siginfo_t *info,void*c __attribute__((unused))){
	if(info->si_pid<=1)return;
	syslog_thread_exit(s);
}

int parse_syslog_item(struct ucred*cred,char*line){
	int e=-1;
	char*p,*c,*l=NULL;
	struct log_item*b=NULL;
	if(cred->pid<0)goto ex;
	if(!(p=l=strdup(line)))goto ex;
	if(!(b=malloc(sizeof(struct log_item))))goto ex;

	// strip space
	for(;*p!=0;p++)if(!isspace(*p))break;
	for(char*z=p+strlen(p)-1;z>p&&isspace(*z);z++)*z=0;

	// fill log_item
	memset(b,0,sizeof(struct log_item));
	b->time=time(NULL),b->pid=cred->pid;
	if(read_file(
		b->tag,sizeof(b->tag),false,
		_PATH_PROC"/%d/comm",b->pid
	)<0)strcpy(b->tag,"unknown");

	// parse log level
	int level=KERN_INFO;
	if(*p=='<'&&(c=strchr(p,'>'))){
		*(c++)=0,p++;
		level=parse_int(p,level);
		p=c;
	}

	// fix log level
	if(level<0)level=KERN_INFO;
	while(level>=8)level-=8;
	b->level=logger_klevel2level(level);

	// skip time
	poss**po=XPOS_RFC3164S;
	size_t len=possible_length(po);
	if(strlen(p)>len&&possible_match(p,po)==len)p+=len;

	// strip space
	for(;*p!=0;p++)if(!isspace(*p))break;

	// parse tag
	char*t,*x1,*x2;
	if((t=strchr(p,':'))&&strlen(t)>=2&&isspace(*(t+1))){
		bool v=true;
		for(char*z=p;z<t;z++)if(isspace(*z))v=false;
		if(v){
			*t=0;
			if(
				(x1=strchr(p,'['))&&
				(x2=strchr(p,']'))&&
				x2>x1&&x2==t-1
			)*x1=0;
			memset(b->tag,0,sizeof(b->tag));
			strncpy(b->tag,p,sizeof(b->tag)-1);
			p=t+2;
		}
	}

	// copy content
	strncpy(b->content,p,sizeof(b->content)-1);

	// send log;
	logger_write(b);

	e=0;
	ex:
	if(l)free(l);
	if(b)free(b);
	return e;
}

static bool process_data(int fd){
        union{
                struct cmsghdr cm;
                char buf[CMSG_SPACE(sizeof(struct ucred))];
        }ctl;
	char data[4096];
	struct cmsghdr*c;
	struct msghdr m={
		.msg_iov=&IOVEC(&data,sizeof(data)-1),
		.msg_iovlen=1,
		.msg_control=&ctl,
		.msg_controllen=sizeof(ctl)
	};
	ctl.cm.cmsg_len=CMSG_LEN(sizeof(struct ucred));
	ctl.cm.cmsg_level=SOL_SOCKET;
	ctl.cm.cmsg_type=SCM_CREDENTIALS;
	memset(data,0,sizeof(data));
	int r=recvmsg(fd,&m,MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
	if(r<0)return errno==EINTR||errno==EAGAIN;
	if(
		r>0&&
		(c=CMSG_FIRSTHDR(&m))&&
		c->cmsg_level==SOL_SOCKET&&
		c->cmsg_type==SCM_CREDENTIALS&&
		c->cmsg_len==CMSG_LEN(sizeof(struct ucred))&&
		data[sizeof(data)-1]==0
	){
		struct ucred*u=(struct ucred*)CMSG_DATA(c);
		if(u&&u->pid>0)parse_syslog_item(u,data);
	}
	return true;
}

static int read_kmsg_thread(void*data __attribute__((unused))){
	int e=0,r;

	clean_log_buffers();
	logger_internal_clean();
	close_all_fd(NULL,0);
	if(open_socket_logfd_default()<0)return -1;

	tlog_info("syslog forwarder start with pid %d",getpid());
	setproctitle("syslog");
	prctl(PR_SET_NAME,"Syslog Forward",0,0,0);
	action_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM,SIGUSR1,SIGUSR2},
		6,signal_handler
	);

	if(listen_syslog_socket()<0)
		return terlog_error(-errno,"listen %s failed",SOCKET_SYSLOG);

	fd_set fs;
	struct timeval timeout;
	while(run){
		FD_ZERO(&fs);
		FD_SET(sfd,&fs);
		FD_SET(logfd,&fs);
		timeout.tv_sec=1,timeout.tv_usec=0;
		r=select(MAX(sfd,logfd)+1,&fs,NULL,NULL,&timeout);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("select failed");
			e=-errno;
			goto ex;
		}else if(r==0)continue;
		else if(FD_ISSET(logfd,&fs)){
			struct log_msg l;
			int x=logger_internal_read_msg(logfd,&l);
			if(x<0){
				close_logfd();
				break;
			}
		}else if(FD_ISSET(sfd,&fs)&&!process_data(sfd))break;
	}
	ex:
	syslog_thread_exit(0);
	return e;
}

int init_syslog(){
	fork_run("syslog",false,NULL,NULL,read_kmsg_thread);
	return 0;
}

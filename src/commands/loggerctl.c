/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"defines.h"
#include"output.h"
#include"logger.h"
#include"getopt.h"
#include"str.h"

enum ctl_oper{
	OPER_NONE,
	OPER_OPEN,
	OPER_LISTEN,
	OPER_ADD,
	OPER_QUIT
};

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: loggerctl [OPTION]...\n"
		"Control init logger daemon.\n\n"
		"Options:\n"
		"\t-t, --tag <TAG>          set log tag\n"
		"\t-p, --pid <PID>          set log pid\n"
		"\t-n, --level <LEVEL>      set log level\n"
  		"\t-s, --socket <SOCKET>    control socket (default is %s)\n"
		"\t-l, --listen <SOCKET>    listen to new socket\n"
		"\t-o, --output <OUTPUT>    write log to new file\n"
		"\t-a, --add                send log to loggerd\n"
		"\t-q, --quit               terminate loggerd\n"
		"\t-h, --help               display this help and exit\n",
		DEFAULT_LOGGER
	);
}

static bool stdin2logger(char*tag,enum log_level level,pid_t pid){
	struct log_item l;
	memset(&l,0,sizeof(struct log_item));
	strncpy(l.tag,tag,sizeof(l.tag)-1);
	l.time=time(NULL);
	l.level=level;
	l.pid=pid;
	size_t s=sizeof(l.content),c=0;
	char*buff=malloc(s);
	if(!buff)return false;
	for(;;){
		ssize_t r=read(STDIN_FILENO,buff,s);
		if(r<0)return false;
		if(r==0)return true;
		memset(&l.content,0,s);
		for(size_t x=0;x<(size_t)r;x++)if(buff[x]=='\n'){
			if(l.content[0]==0)continue;
			if(logger_write(&l)<=0)return false;
			memset(&l.content,0,s);
			c=0;
		}else l.content[c++]=buff[x];
		if(l.content[0]!=0&&logger_write(&l)<=0)return false;
	}
}

static bool args2logger(char*tag,enum log_level level,pid_t pid,char**argv){
	struct log_item l;
	memset(&l,0,sizeof(struct log_item));
	strncpy(l.tag,tag,sizeof(l.tag)-1);
	l.time=time(NULL);
	l.level=level;
	l.pid=pid;
	size_t s=sizeof(l.content),e,i,t=0;
	for(i=0;argv[i]&&t<s;i++){
		for(e=0;argv[i][e]!=0&&t<s-1;e++)l.content[t++]=argv[i][e];
		l.content[t++]=' ';
	}
	return l.content[0]==0||logger_write(&l)>0;
}

int loggerctl_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"quit",    no_argument,       NULL,'q'},
		{"add",     no_argument,       NULL,'a'},
		{"pid",     required_argument, NULL,'p'},
		{"tag",     required_argument, NULL,'t'},
		{"level",   required_argument, NULL,'n'},
		{"listen",  required_argument, NULL,'l'},
		{"output",  required_argument, NULL,'o'},
		{"socket",  required_argument, NULL,'s'},
		{NULL,0,NULL,0}
	};
	char*socket=NULL,*data=NULL,*tag=NULL;
	enum ctl_oper op=OPER_NONE;
	enum log_level level=0;
	pid_t pid=-1;
	int o;
	while((o=b_getlopt(argc,argv,"hqap:t:n:l:o:s:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'q':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_QUIT;
		break;
		case 'a':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_ADD;
		break;
		case 'p':
			if(op!=OPER_NONE&&op!=OPER_ADD)goto conflict;
			if(pid>=0)goto conflict;
			if((pid=parse_int(b_optarg,-1))<0)
				return re_printf(2,"invalid PID number: %s\n",b_optarg);
		break;
		case 't':
			if(op!=OPER_NONE&&op!=OPER_ADD)goto conflict;
			if(tag)goto conflict;
			tag=b_optarg;
		break;
		case 'n':
			if(op!=OPER_NONE&&op!=OPER_ADD)goto conflict;
			if(level!=0)goto conflict;
			if((level=logger_parse_level(b_optarg))==0)
				return re_printf(2,"invalid log level: %s\n",b_optarg);
		break;
		case 'l':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_LISTEN;
			data=b_optarg;
		break;
		case 'o':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_OPEN;
			data=b_optarg;
		break;
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
		break;
		default:return 1;
	}
	if(!socket)socket=DEFAULT_LOGGER;
	if(open_socket_logfd(socket)<0)return 2;
	int r;
	if(op!=OPER_ADD&&(pid>=0||level!=0||tag||argc!=b_optind))goto conflict;
	switch(op){
		case OPER_ADD:
			if(!tag)tag="logger";
			if(pid<0)pid=getpid();
			if(level==0)level=LEVEL_INFO;
			r=(argc==b_optind+1&&strcmp("-",argv[b_optind])==0)||argc==b_optind?
				!stdin2logger(tag,level,pid):
				!args2logger(tag,level,pid,argv+b_optind);
		break;
		case OPER_QUIT:
			r=logger_exit();
			if(errno>0)perror(_("terminate loggerd"));
			break;
		case OPER_OPEN:
			r=logger_open(data);
			if(errno>0)stderr_perror("open file %s",data);
		break;
		case OPER_LISTEN:
			r=logger_listen(data);
			if(errno>0)stderr_perror("listen new socket %s",data);
		break;
		default:r=re_printf(2,"no action specified\n");break;
	}
	return r;
	conflict:return re_printf(2,"too many arguments\n");
}

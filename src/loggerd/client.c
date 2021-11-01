/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<ctype.h>
#include<errno.h>
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#ifdef ENABLE_UEFI
#include<Library/DebugLib.h>
#else
#include<sys/un.h>
#include<sys/socket.h>
#endif
#include"defines.h"
#include"output.h"
#include"system.h"
#include"logger_internal.h"

#ifndef ENABLE_UEFI
int logfd=-1;

int set_logfd(int fd){
	if(fd<0)return logfd;
	logfd=fd;
	return fd;
}

void close_logfd(){
	close(logfd);
	logfd=-1;
}

int open_file_logfd(char*path){
	if(!path)ERET(EINVAL);
	int fd=open(path,O_WRONLY|O_SYNC);
	if(fd<0)stderr_perror("cannot open log pipe %s",path);
	return fd<0?fd:set_logfd(fd);
}

int open_socket_logfd(char*path){
	if(!path)ERET(EINVAL);
	struct sockaddr_un addr;
	int sock;
	close_logfd();
	if((sock=socket(AF_UNIX,SOCK_STREAM,0))<0){
		stderr_perror("cannot create socket");
		return -1;
	}
	memset(&addr,0,sizeof(addr));
	addr.sun_family=AF_UNIX;
	strncpy(addr.sun_path,path,sizeof(addr.sun_path)-1);
	if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0){
		stderr_perror("cannot connect socket %s",path);
		close(sock);
		return -1;
	}
	return set_logfd(sock);
}

int logger_send_string(enum log_oper oper,char*string){
	int r;
	struct log_msg msg;
	if((r=logger_internal_send_string(logfd,oper,string))<0)return r;
	do{if(logger_internal_read_msg(logfd,&msg)<0)return -1;}
	while(msg.oper!=LOG_OK&&msg.oper!=LOG_FAIL);
	return msg.data.code;
}

int logger_listen(char*file){
	return file?logger_send_string(LOG_LISTEN,file):-EINVAL;
}

int logger_open(char*file){
	return file?logger_send_string(LOG_OPEN,file):-EINVAL;
}

int logger_exit(){
	logger_send_string(LOG_QUIT,NULL);
	close_logfd();
	return 0;
}

int logger_klog(){
	return logger_send_string(LOG_KLOG,NULL);
}

int logger_syslog(){
	return logger_send_string(LOG_SYSLOG,NULL);
}

int logger_open_console(){
	return logger_send_string(LOG_CONSOLE,NULL);
}

int start_loggerd(pid_t*p){
	int fds[2],r;
	if(logfd>=0)ERET(EEXIST);
	if(socketpair(AF_UNIX,SOCK_STREAM,0,fds)<0)return -errno;
	pid_t pid=fork();
	switch(pid){
		case -1:return -errno;
		case 0:
			close_all_fd((int[]){fds[0]},1);
			r=loggerd_thread(fds[0]);
			exit(r);
	}
	close(fds[0]);
	struct log_msg msg;
	do{if(logger_internal_read_msg(fds[1],&msg)<0)ERET(EIO);}
	while(msg.oper!=LOG_OK);
	if(p)*p=pid;
	return set_logfd(fds[1]);
}
#else
bool console_output=true;

void logger_set_console(bool enabled){
	console_output=enabled;
}
#endif

int logger_write(struct log_item*log){
	if(!log)ERET(EINVAL);
	ssize_t s=strlen(log->content)-1;
	while(s>=0)
		if(!isspace(log->content[s]))break;
		else log->content[s]=0,s--;
	#ifndef ENABLE_UEFI
	static size_t xs=sizeof(struct log_msg);
	if(logfd<0)return fprintf(stderr,"%s: %s\n",log->tag,log->content);
	struct log_msg msg;
	logger_internal_init_msg(&msg,LOG_ADD);
	memcpy(&msg.data.log,log,sizeof(struct log_item));
	if(((size_t)write(logfd,&msg,xs))!=xs)return -1;
	memset(&msg,0,sizeof(msg));
	do{if(logger_internal_read_msg(logfd,&msg)<0)return -1;}
	while(msg.oper!=LOG_OK&&msg.oper!=LOG_FAIL);
	errno=msg.data.code;
	return xs;
	#else
	wchar_t*mt,*mc;
	size_t st,sc;
	st=(strlen(log->tag)+1)*sizeof(wchar_t);
	sc=(strlen(log->content)+1)*sizeof(wchar_t);
	if(!(mt=malloc(st))||!(mc=malloc(sc))){
		if(mt)free(mt);
		if(mc)free(mc);
		return -1;
	}
	mbstowcs(mt,log->tag,st);
	mbstowcs(mc,log->content,sc);
	DebugPrint(EFI_D_INFO,"%s: %s\n",mt,mc);
	free(mt);
	free(mc);
	if(console_output)printf("%s: %s\n",log->tag,log->content);
	return 0;
	#endif
}

int logger_print(enum log_level level,char*tag,char*content){
	if(!tag||!content)ERET(EINVAL);
	struct log_item log;
	memset(&log,0,sizeof(struct log_item));
	log.level=level;
	strncpy(log.tag,tag,sizeof(log.tag)-1);
	strncpy(log.content,content,sizeof(log.content)-1);
	time(&log.time);
	#ifndef ENABLE_UEFI
	log.pid=getpid();
	#endif
	return logger_write(&log);
}

static int logger_printf_x(enum log_level level,char*tag,const char*fmt,va_list ap){
	if(!tag||!fmt)ERET(EINVAL);
	char content[4095];
	memset(content,0,4095);
	if(!vsnprintf(content,4094,fmt,ap))return -errno;
	return logger_print(level,tag,content);
}

static int logger_perror_x(enum log_level level,char*tag,const char*fmt,va_list ap){
	if(!fmt)ERET(EINVAL);
	char*buff=NULL;
	if(errno>0){
		char*er=strerror(errno);
		size_t s=strlen(fmt)+strlen(er)+4;
		if(!(buff=malloc(s)))ERET(ENOMEM);
		memset(buff,0,s);
		snprintf(buff,s,"%s: %s",fmt,er);
	};
	int e=logger_printf_x(level,tag,buff?buff:fmt,ap);
	if(buff)free(buff);
	return e;
}

int logger_printf(enum log_level level,char*tag,const char*fmt,...){
	if(!tag||!fmt)ERET(EINVAL);
	int err=errno;
	va_list ap;
	va_start(ap,fmt);
	errno=err;
	int r=logger_printf_x(level,tag,fmt,ap);
	va_end(ap);
	errno=err;
	return r;
}

int logger_perror(enum log_level level,char*tag,const char*fmt,...){
	if(!tag||!fmt)ERET(EINVAL);
	int err=errno;
	va_list ap;
	va_start(ap,fmt);
	errno=err;
	int r=logger_perror_x(level,tag,fmt,ap);
	va_end(ap);
	errno=err;
	return r;
}

int return_logger_printf(enum log_level level,int e,char*tag,const char*fmt,...){
	if(!tag||!fmt)ERET(EINVAL);
	int err=errno;
	va_list ap;
	va_start(ap,fmt);
	errno=err;
	logger_printf_x(level,tag,fmt,ap);
	va_end(ap);
	errno=err;
	return e;
}

int return_logger_perror(enum log_level level,int e,char*tag,const char*fmt,...){
	if(!tag||!fmt)ERET(EINVAL);
	int ee=errno;
	va_list ap;
	va_start(ap,fmt);
	errno=ee;
	logger_perror_x(level,tag,fmt,ap);
	va_end(ap);
	return e;
}

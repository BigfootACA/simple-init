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
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#ifdef ENABLE_UEFI
#include<Library/PcdLib.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/DebugLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Guid/FileInfo.h>
#include"str.h"
#include"locate.h"
#include"compatible.h"
#else
#include"recovery.h"
#include<sys/un.h>
#include<sys/socket.h>
#endif
#include"confd.h"
#include"output.h"
#include"system.h"
#include"defines.h"
#include"filesystem.h"
#include"logger_internal.h"
#define TAG "logger"

#ifdef ENABLE_UEFI
static char print_buff[16384];
static enum log_level logger_level=FixedPcdGet32(PcdLoggerdMinLevel);
#else
static enum log_level logger_level=LEVEL_DEBUG;
#endif

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
fsh*logger_output=NULL;

void logger_set_console(bool enabled){
	console_output=enabled;
}

static int logger_init_out(){
	bool ex=false;
	fsh*f=NULL;
	int old=0,i=0;
	EFI_TIME tm;
	size_t len,s;
	char*log=NULL,*nb,*k;
	fs_file_flag flag=FILE_FLAG_READ|FILE_FLAG_WRITE;
	// 0: truncate
	// 1: append
	// 2: rename
	switch((int)confd_get_type("logger.old_file")){
		case -1:break;
		case TYPE_INTEGER:old=confd_get_integer("logger.old_file",0);break;
		case TYPE_STRING:
			if(!(k=confd_get_string("logger.old_file",NULL)))break;
			s=MIN(strlen(k),1);
			if(strncasecmp(k,"truncate",s)==0)old=0;
			else if(strncasecmp(k,"append",s)==0)old=1;
			else if(strncasecmp(k,"rename",s)==0)old=2;
			else tlog_warn("unknown old file mode %s",k);
			free(k);
		break;
		default:tlog_warn("unknown type for old file mode");
	}
	if(!(log=confd_get_string("logger.file_output",NULL)))return 0;
	switch(old){
		case 0:flag|=FILE_FLAG_CREATE|FILE_FLAG_TRUNCATE;break;
		case 1:flag|=FILE_FLAG_CREATE|FILE_FLAG_APPEND;break;
		case 2:
			if(fs_open(NULL,&f,log,flag)==0){
				len=strlen(log);
				if((nb=malloc(len+32))){
					strncpy(nb,log,len);
					do{snprintf(nb+len,32,".%d",i++);}
					while(fs_exists(NULL,nb,&ex)==0&&ex);
					errno=0,fs_rename(f,nb);
					telog_debug("rename old log file %s to %s",log,nb);
					free(nb);
				}
				fs_close(&f);
			}else flag|=FILE_FLAG_TRUNCATE;
			flag|=FILE_FLAG_CREATE;
		break;
	}
	if(fs_open(NULL,&f,log,flag)!=0)
		EDONE(telog_warn("open logger file output failed"));
	logger_output=f;
	ZeroMem(&tm,sizeof(tm));
	gRT->GetTime(&tm,NULL);
	CHAR8*buff=AllocateZeroPool(PATH_MAX);
	if(buff){
		AsciiSPrint(
			buff,PATH_MAX,
			"-------- file %a opened at %t --------",
			log,&tm
		);
		logger_out_write(buff);
		FreePool(buff);
	}
	flush_buffer();
	tlog_debug("opened logger file output %s",log);
	free(log);
	return 0;
	done:
	if(f)fs_close(&f);
	if(log)free(log);
	return -1;
}

void logger_init(){
	char*k;
	enum log_level level;
	// PcdLoggerdUseConsole is in uefimain.c

	logger_init_out();
	console_output=confd_get_boolean(
		"logger.use_console",
		console_output
	);
	switch((int)confd_get_type("logger.min_level")){
		case -1:break;
		case TYPE_INTEGER:logger_level=confd_get_integer(
			"logger.min_level",
			logger_level
		);break;
		case TYPE_STRING:
			if(!(k=confd_get_string("logger.min_level",NULL)))break;
			level=logger_parse_level((char*)k);
			if(level!=0)logger_level=level;
			else tlog_warn("unknown log level %s",k);
			free(k);
		break;
		default:tlog_warn("unknown type for min level");
	}
}

void logger_out_write(char*buff){
	if(!logger_output||!*buff)return;
	fs_println(logger_output,buff);
	fs_flush(logger_output);
}
#endif

void logger_set_level(enum log_level level){
	logger_level=level;
}

int logger_write(struct log_item*log){
	if(!log)ERET(EINVAL);
	if(log->level<logger_level)return 0;
	ssize_t s=strlen(log->content)-1;
	while(s>=0)
		if(!isspace(log->content[s]))break;
		else log->content[s]=0,s--;
	#ifndef ENABLE_UEFI
	static size_t xs=sizeof(struct log_msg);
	if(logfd<0){
		if(recovery_out_fd>=0)recovery_ui_printf("%s: %s",log->tag,log->content);
		return fprintf(stderr,"%s: %s\n",log->tag,log->content);
	}
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
	AsciiSPrint(print_buff,sizeof(print_buff),"%a: %a",log->tag,log->content);
	DebugPrint(EFI_D_INFO,"%a",print_buff);
	DebugPrint(EFI_D_INFO,"\n");
	if(console_output){
		AsciiPrint("%a",print_buff);
		AsciiPrint("\n");
	}
	logger_out_write(print_buff);
	return 0;
	#endif
}

int logger_print(enum log_level level,char*tag,char*content){
	if(!tag||!content)ERET(EINVAL);
	if(level<logger_level)return 0;
	struct log_item*log=malloc(sizeof(struct log_item));
	if(!log)ERET(ENOMEM);
	memset(log,0,sizeof(struct log_item));
	log->level=level;
	strncpy(log->tag,tag,sizeof(log->tag)-1);
	strncpy(log->content,content,sizeof(log->content)-1);
	time(&log->time);
	#ifndef ENABLE_UEFI
	log->pid=getpid();
	#endif
	logger_internal_buffer_push(log);
	int r=logger_write(log);
	free(log);
	return r;
}

static int logger_printf_x(enum log_level level,char*tag,const char*fmt,va_list ap){
	int r;
	char*content=NULL;
	if(!tag||!fmt)ERET(EINVAL);
	if(level<logger_level)return 0;
	r=vasprintf(&content,fmt,ap);
	if(r<0||!content)return -errno;
	r=logger_print(level,tag,content);
	free(content);
	return r;
}

static int logger_perror_x(enum log_level level,char*tag,const char*fmt,va_list ap){
	if(!fmt)ERET(EINVAL);
	if(level<logger_level)return 0;
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
	if(level<logger_level)return 0;
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
	if(level<logger_level)return 0;
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
	if(level<logger_level)return 0;
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
	if(level<logger_level)return 0;
	int ee=errno;
	va_list ap;
	va_start(ap,fmt);
	errno=ee;
	logger_perror_x(level,tag,fmt,ap);
	va_end(ap);
	return e;
}

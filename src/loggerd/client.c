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
#include<Library/PcdLib.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/DebugLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include"str.h"
#include"locate.h"
#include"compatible.h"
#else
#include<sys/un.h>
#include<sys/socket.h>
#endif
#include"confd.h"
#include"output.h"
#include"system.h"
#include"defines.h"
#include"logger_internal.h"
#define TAG "logger"

#ifdef ENABLE_UEFI
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
EFI_FILE_PROTOCOL*logger_output=NULL;

void logger_set_console(bool enabled){
	console_output=enabled;
}

static int logger_init_out(){
	UINTN s=0;
	EFI_TIME tm;
	EFI_STATUS st;
	char*log=NULL;
	EFI_FILE_INFO*info=NULL;
	locate_ret*l=AllocateZeroPool(sizeof(locate_ret));
	if(!l)EDONE(tlog_warn("allocate pool failed"));
	// 0: truncate
	// 1: append
	// 2: rename
	int old=0;
	switch((int)confd_get_type("logger.old_file")){
		case -1:break;
		case TYPE_INTEGER:old=confd_get_integer("logger.old_file",0);break;
		case TYPE_STRING:{
			char*k=confd_get_string("logger.old_file",NULL);
			if(k){
				size_t s=MIN(strlen(k),1);
				if(strncasecmp(k,"truncate",s)==0)old=0;
				else if(strncasecmp(k,"append",s)==0)old=1;
				else if(strncasecmp(k,"rename",s)==0)old=2;
				else tlog_warn("unknown old_file mode %s",k);
				free(k);
			}
		}break;
		default:tlog_warn("unknown type for old_file mode");
	}
	if(!(log=confd_get_string("logger.file_output",NULL)))return 0;
	if(!boot_locate_create_file(l,log))
		EDONE(tlog_warn("resolve logger file output locate failed"));
	if(l->type!=LOCATE_FILE)
		EDONE(tlog_warn("unsupported locate type for logger file output"));
	if(old!=1)l->file->SetPosition(l->file,0);
	st=l->file->GetInfo(l->file,&gEfiFileInfoGuid,&s,info);
	if(st==EFI_BUFFER_TOO_SMALL){
		s+=64;
		if(!(info=AllocateZeroPool(s)))
			EDONE(tlog_warn("allocate file info failed"));
		st=l->file->GetInfo(l->file,&gEfiFileInfoGuid,&s,info);
	}
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get file info failed: %s",
		efi_status_to_string(st)
	));
	if(info->FileSize==0)
		tlog_verbose("old log file size is zero");
	else if(old==0){
		tlog_verbose(
			"truncated old log file %llu bytes",
			(unsigned long long)info->FileSize
		);
		info->FileSize=0;
		st=l->file->SetInfo(l->file,&gEfiFileInfoGuid,s,info);
		if(EFI_ERROR(st)){
			tlog_warn(
				"truncate file failed (%s), try to delete",
				efi_status_to_string(st)
			);
			st=l->file->Delete(l->file);
			if(EFI_ERROR(st))EDONE(tlog_warn(
				"delete %s failed: %s",
				log,efi_status_to_string(st)
			));
			l->file=NULL;
		}
	}else if(old==2){
		int i=0;
		EFI_FILE_PROTOCOL*fp=NULL;
		UINTN bs=PATH_MAX*sizeof(CHAR16);
		CHAR16*buff16=AllocatePool(bs);
		if(!buff16)EDONE(tlog_warn("allocate pool failed"));
		do{
			ZeroMem(buff16,bs);
			UnicodeSPrint(buff16,bs,L"%s.%d",l->path16,i);
			st=l->root->Open(l->root,&fp,buff16,EFI_FILE_MODE_READ,0);
			i++;
		}while(!EFI_ERROR(st));
		if(fp)fp->Close(fp);
		ZeroMem(buff16,bs);
		UnicodeSPrint(buff16,bs,L"%s.%d",info->FileName,i);
		StrCpyS(info->FileName,(s-80)/sizeof(CHAR16),buff16);
		tlog_verbose("rename old log file %s to %s.%d",l->path,l->path,i);
		st=l->file->SetInfo(l->file,&gEfiFileInfoGuid,s,info);
		FreePool(buff16);
		if(EFI_ERROR(st))EDONE(tlog_warn(
			"set info %s failed: %s",
			log,efi_status_to_string(st)
		));
	}
	if(!l->file){
		st=l->root->Open(
			l->root,&l->file,l->path16,
			EFI_FILE_MODE_READ|
			EFI_FILE_MODE_WRITE|
			EFI_FILE_MODE_CREATE,0
		);
		if(EFI_ERROR(st))EDONE(tlog_warn(
			"open %s failed: %s",
			log,efi_status_to_string(st)
		));
	}
	if(old==1){
		tlog_verbose(
			"old log file size: %llu bytes",
			(unsigned long long)info->FileSize
		);
		l->file->SetPosition(l->file,info->FileSize);
	}
	logger_output=l->file;
	ZeroMem(&tm,sizeof(tm));
	gRT->GetTime(&tm,NULL);
	CHAR8*buff=AllocateZeroPool(PATH_MAX);
	if(buff){
		AsciiSPrint(
			buff,PATH_MAX,
			"-------- file %a opened at %t --------",
			l->path,&tm
		);
		logger_out_write(buff);
		FreePool(buff);
	}
	flush_buffer();
	tlog_debug("opened logger file output %s",log);
	free(log);
	FreePool(info);
	FreePool(l);
	return 0;
	done:
	if(l)FreePool(l);
	if(log)free(log);
	if(info)FreePool(info);
	return -1;
}

void logger_init(){
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
		case TYPE_STRING:{
			char*k=confd_get_string("logger.min_level",NULL);
			if(k){
				enum log_level level;
				level=logger_parse_level((char*)k);
				if(level!=0)logger_level=level;
				else tlog_warn("unknown log level %s",k);
				free(k);
			}
		}break;
		default:tlog_warn("unknown type for min_level");
	}
}

void logger_out_write(char*buff){
	UINTN cnt,x,pos=0;
	if(!logger_output)return;
	cnt=AsciiStrLen(buff),x=cnt;
	do{
		logger_output->Write(logger_output,&x,buff+pos);
		pos+=x,x=cnt-pos;
	}while(pos<cnt);
	x=1;
	logger_output->Write(logger_output,&x,"\n");
	logger_output->Flush(logger_output);
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
	char*buff=AllocateZeroPool(16384);
	if(!buff)ERET(ENOMEM);
	AsciiSPrint(buff,16384,"%a: %a",log->tag,log->content);
	DebugPrint(EFI_D_INFO,"%a",buff);
	DebugPrint(EFI_D_INFO,"\n");
	if(console_output){
		AsciiPrint("%a",buff);
		AsciiPrint("\n");
	}
	logger_out_write(buff);
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
	if(!tag||!fmt)ERET(EINVAL);
	if(level<logger_level)return 0;
	size_t s=16384;
	char*content=malloc(s);
	if(!content)ERET(ENOMEM);
	memset(content,0,s);
	if(!vsnprintf(content,s-1,fmt,ap))return -errno;
	int r=logger_print(level,tag,content);
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

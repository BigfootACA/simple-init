/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"logger_internal.h"
#include"defines.h"
#include"output.h"
#include"str.h"

struct open_file{
	char file[PATH_MAX];
	int fd;
};

static struct open_file*files[128];

int open_log_file(char*path){
	int i;
	struct open_file*f=NULL;
	char tc[24]={0};
	time_t t=time(NULL);
	errno=0;
	for(i=0;(f=files[i]);i++)
		if(strcmp(path,f->file)==0)
			return f->fd;
	for(i=0;i<128;i++){
		f=files[i];
		if(f&&f->fd>0)continue;
		if(!(f=malloc(sizeof(struct open_file))))return -1;
		memset(f,0,sizeof(struct open_file));
		strncpy(f->file,path,sizeof(f->file)-1);
		if((f->fd=open(
			path,
			O_WRONLY|O_SYNC|O_APPEND|O_CREAT,
			0644
		))<0){
			free(f);
			return -1;
		}
		files[i]=f;
		if(!isatty(f->fd))dprintf(
			f->fd,
			"-------- file %s opened at %s --------\n",
			path,time2ndefstr(&t,tc,23)
		);
		errno=0;
		return f->fd;
	}
	fprintf(stderr,"open log files too many\n");
	ERET(ENOMEM);
}

static void close_log(struct open_file*f){
	if(f->fd<=0)return;
	char tc[24]={0};
	time_t t=time(NULL);
	if(!isatty(f->fd))dprintf(
		f->fd,
		"-------- file %s closed at %s --------\n",
		f->file,time2ndefstr(&t,tc,23)
	);
	close(f->fd);
	f->fd=-1;
}
void close_log_file(char*path){
	int i;
	struct open_file*f=NULL;
	for(i=0;(f=files[i]);i++)
		if(strcmp(path,f->file)==0)
			close_log(f);
}

void close_all_file(){
	int i;
	struct open_file*f=NULL;
	for(i=0;(f=files[i]);i++){
		close_log(f);
		free(f);
		files[i]=NULL;
	}
}

static char*level2color(enum log_level level){
	switch(level){
		case LEVEL_VERBOSE: return "\033[37;1m";
		case LEVEL_DEBUG:   return "\033[37;2m";
		case LEVEL_INFO:    return "\033[32;1m";
		case LEVEL_NOTICE:  return "\033[34;1m";
		case LEVEL_WARNING: return "\033[33;1m";
		case LEVEL_ERROR:   return "\033[31;1m";
		case LEVEL_CRIT:    return "\033[31;1;5m";
		case LEVEL_ALERT:   return "\033[31;2;7m";
		case LEVEL_EMERG:   return "\033[31;1;7m\007";
		default:            return "";
	}
}

int file_logger(char*name,struct log_item*log){
	char buff[24]={0},p[16]={0};
	int fd=-1;
	if(strncasecmp(name,"stderr",6)==0)fd=STDERR_FILENO;
	else if(strncasecmp(name,"stdout",6)==0)fd=STDOUT_FILENO;
	else fd=open_log_file(name);
	if(fd<0)return -errno;
	if(!log->time)ERET(EFAULT);
	if(log->pid>0)snprintf(p,15,"[%d]",log->pid);
	bool tty=isatty(fd);
	char*time,*level,level_pad[16]={0},*end;
	time=time2ndefstr(&log->time,buff,sizeof(buff));
	level=logger_level2string(log->level);
	for(size_t i=0;i<(6-strlen(level));i++)level_pad[i]=' ';
	end=tty?"\033[0m":"";
	int r=dprintf(fd,
		"%s[%s]%s %s<%s>%s%s %s%s%s%s: %s%s%s\n",
		tty?"\r\033[36m":"",time,end,
		tty?"\033[37;1;4m":"",level,end,level_pad,
		tty?"\033[33m":"",log->tag,p,end,
		tty?level2color(log->level):"",log->content,end
	);
	return r;
}

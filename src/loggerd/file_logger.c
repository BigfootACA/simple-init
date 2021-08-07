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
		if((f->fd=open(path,O_WRONLY|O_SYNC|O_APPEND|O_CREAT))<0){
			free(f);
			return -1;
		}
		files[i]=f;
		dprintf(
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
	dprintf(
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

int file_logger(char*name,struct log_item*log){
	char buff[24]={0},p[16]={0};
	int fd=-1;
	if(strncasecmp(name,"stderr",6)==0)fd=STDERR_FILENO;
	else if(strncasecmp(name,"stdout",6)==0)fd=STDOUT_FILENO;
	else fd=open_log_file(name);
	if(fd<0)return -errno;
	if(!log->time)ERET(EFAULT);
	if(log->pid>0)snprintf(p,15,"[%d]",log->pid);
	int r=dprintf(fd,
		"%s %-6s %s%s: %s\n",
		time2ndefstr(&log->time,buff,sizeof(buff)),
		level2string(log->level),
		log->tag,
		p,
		log->content
	);
	return r;
}

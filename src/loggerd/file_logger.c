#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"logger.h"
#include"defines.h"
#include"output.h"
#include"str.h"

struct open_file{
	char file[PATH_MAX];
	FILE*pointer;
};

static struct open_file*files[128];

FILE*open_log_file(char*path){
	int i;
	struct open_file*f=NULL;
	char tc[24]={0};
	time_t t=time(NULL);
	errno=0;
	for(i=0;(f=files[i]);i++)
		if(strcmp(path,f->file)==0&&f->pointer)
			return f->pointer;
	for(i=0;i<128;i++){
		f=files[i];
		if(f&&f->pointer)continue;
		if(!(f=malloc(sizeof(struct open_file))))return NULL;
		memset(f->file,0,PATH_MAX);
		strncpy(f->file,path,PATH_MAX-1);
		if(!(f->pointer=fopen(path,"a+"))){
			fprintf(stderr,"failed to open %s: %m\n",path);
			free(f);
			return NULL;
		}
		files[i]=f;
		fprintf(
			f->pointer,
			"-------- file %s opened at %s --------\n",
			path,time2ndefstr(&t,tc,23)
		);
		fflush(f->pointer);
		errno=0;
		return f->pointer;
	}
	fprintf(stderr,"open log files too many\n");
	errno=ENOMEM;
	return NULL;
}

void close_log_file(char*path){
	int i;
	struct open_file*f=NULL;
	for(i=0;(f=files[i]);i++){
		if(strcmp(path,f->file)==0&&f->pointer){
			fclose(f->pointer);
			f->pointer=NULL;
		}
	}
}

void close_all_file(){
	int i;
	struct open_file*f=NULL;
	for(i=0;(f=files[i]);i++)if(f->pointer){
		fclose(f->pointer);
		f->pointer=NULL;
		free(f);
	}
}

int file_logger(char*name,struct log_item*log){
	char buff[24]={0},p[16]={0};
	FILE*o=NULL;
	if(strncasecmp(name,"stderr",6)==0)o=stderr;
	else if(strncasecmp(name,"stdout",6)==0)o=stdout;
	else o=open_log_file(name);
	if(!o)return -errno;
	if(!log->time)ERET(EFAULT);
	if(log->pid>0)snprintf(p,15,"[%d]",log->pid);
	int r=fprintf(o,
		"%s %-6s %s%s: %s\n",
		time2ndefstr(&log->time,buff,sizeof(buff)),
		level2string(log->level),
		log->tag,
		p,
		log->content
	);
	fflush(o);
	return r;
}

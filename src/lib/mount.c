/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<libmount/libmount.h>
#include"str.h"
#include"logger.h"
#include"system.h"
#include"array.h"
#include"defines.h"
#define TAG "mount"

int xmount(bool ex,const char*dev,const char*dir,const char*type,const char*data,bool warning){
	int r,er;
	char buf[BUFSIZ]={0},xbuf[BUFSIZ]={0};
	struct libmnt_context *cxt=mnt_new_context();
	if(!cxt)return terlog_error(-errno,"failed to init libmount context");
	errno=0;
	if(mnt_context_set_options(cxt,data)!=0){
		telog_error("failed to set mount options");
		mnt_free_context(cxt);
		return -1;
	}
	errno=0;
	if(mnt_context_set_fstype(cxt,type)!=0){
		telog_error("failed to set mount filesystem type");
		mnt_free_context(cxt);
		return -1;
	}
	errno=0;
	if(mnt_context_set_source(cxt,dev)!=0){
		telog_error("failed to set mount source");
		mnt_free_context(cxt);
		return -1;
	}
	errno=0;
	if(mnt_context_set_target(cxt,dir)!=0){
		telog_error("failed to set mount target");
		mnt_free_context(cxt);
		return -1;
	}
	int ec=mnt_context_mount(cxt);
	er=errno;
	r=mnt_context_get_excode(cxt,ec,buf,sizeof(buf));
	mnt_free_context(cxt);
	snprintf(
		xbuf,BUFSIZ-1,
		"mount %s%s%s%s to %s with '%s'",
		dev,
		type?"(":"",type?type:"",type?")":"",
		dir,
		data
	);
	if(r==0)tlog_debug("%s success.",xbuf);
	else{
		logger_printf(warning?LEVEL_ERROR:LEVEL_DEBUG,TAG,"%s failed: %s",xbuf,buf);
		if(ex)exit(r);
	}
	errno=er;
	return r;
}

static struct mount_item**_array_add_entry(struct mount_item**array,struct mount_item*entry){
	size_t idx=0,s;
	if(array)while(array[idx++]);
	else idx++;
	s=sizeof(struct mount_item*)*(idx+1);
	if(!(array=array?realloc(array,s):malloc(s)))return NULL;
	array[idx-1]=entry;
	array[idx]=NULL;
	return array;
}

struct mount_item**read_proc_mounts(){
	FILE*f=fopen(_PATH_PROC_SELF"/mounts","r");
	if(!f)return NULL;
	size_t bs=sizeof(struct mount_item);
	struct mount_item**array=NULL,**arr,*buff;
	char line[BUFFER_SIZE];
	while(!feof(f)){
		memset(line,0,BUFSIZ);
		fgets(line,BUFFER_SIZE,f);
		if(!line[0])continue;
		char**c=args2array(line,0);
		if(!c||char_array_len(c)!=6)continue;
		if(!(buff=malloc(bs))){
			if(array)free_mounts(array);
			return NULL;
		}
		memset(buff,0,bs);
		buff->source=c[0];
		buff->target=c[1];
		buff->type=c[2];
		buff->options=args2array(c[3],',');
		buff->freq=parse_int(c[4],0);
		buff->passno=parse_int(c[5],0);
		free(c);
		if(!(arr=_array_add_entry(array,buff))){
			free_mounts(array);
			array=NULL;
			free(buff);
			return NULL;
		}else array=arr;
	}
	return array;
}

void free_mount_item(struct mount_item*m){
	if(!m)return;
	if(m->source)free(m->source);
	if(m->options){
		if(m->options[0])free(m->options[0]);
		free(m->options);
	}
	memset(m,0,sizeof(struct mount_item));
	free(m);
}

void free_mounts(struct mount_item**c){
	if(!c)return;
	struct mount_item*s;
	size_t l=0;
	while((s=c[l++]))free_mount_item(s);
	free(c);
}

bool is_mountpoint(char*path){
	struct mount_item**c=read_proc_mounts(),*s;
	if(!c)return false;
	bool ret=false;
	size_t l=0;
	while((s=c[l++]))if(strcmp(s->source,path)==0)ret=true;
	free_mounts(c);
	return ret;
}

char*auto_mountpoint(char*path,size_t len){
	static char*base=_PATH_RUN"/mounts";
	if(!path)EPRET(EINVAL);
	mkdir(base,0755);
	for(int i=0;i<256;i++){
		memset(path,0,len);
		snprintf(path,len-1,"%s/%d",base,i);
		if(access(path,F_OK)==0)continue;
		return mkdir(path,0755)<0?NULL:path;
	}
	EPRET(EMFILE);
}

void mountpoint_locker(bool lock){
	static char*base=_PATH_RUN"/mounts/LOCK";
	if(lock){
		mkdir(_PATH_RUN"/mounts",0755);
		while(access(base,F_OK)==0)usleep(500000);
		close(open(base,O_WRONLY|O_CREAT,0644));
	}else unlink(base);
}

int auto_mount(const char*source,const char*type,char*target,size_t len){
	bool mounted=false;
	mountpoint_locker(true);
	memset(target,0,len);
	struct mount_item**ms=read_proc_mounts();
	if(ms){
		for(size_t i=0;ms[i];i++){
			if(strcmp(type,ms[i]->type)!=0)continue;
			if(strcmp(source,ms[i]->source)!=0)continue;
			mounted=true;
			strncpy(target,ms[i]->target,len-1);
			tlog_debug("%s already mounted on %s",source,target);
			break;
		}
		free_mounts(ms);
	}
	if(!mounted){
		int e;
		if(!auto_mountpoint(target,len)){
			e=-(errno);
			telog_error("cannot get new mountpoint");
			mountpoint_locker(false);
			return e;
		}
		if(xmount(false,source,target,type,"rw,noatime",true)!=0){
			e=-(errno);
			mountpoint_locker(false);
			return e;
		}
	}
	mountpoint_locker(false);
	return 0;
}

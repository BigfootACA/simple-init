/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<unistd.h>
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#define TAG "devtmpfs"
#include"str.h"
#include"devd.h"
#include"system.h"
#include"logger.h"
#include"defines.h"

static int do_mknod(char*path,int dir,int type){
	int major=-1,minor=-1,uevent;
	char*name=NULL,kb[PATH_MAX]={0},vb[PATH_MAX]={0},buff[PATH_MAX]={0};
	size_t idx,xi=0;
	ssize_t r;
	bool expr=false;
	memset(&kb,0,PATH_MAX);
	memset(&vb,0,PATH_MAX);
	memset(&buff,0,PATH_MAX);
	if((uevent=openat(dir,"uevent",O_RDONLY))<0)return -errno;
	while((r=read(uevent,&buff,PATH_MAX))>0){
		for(idx=0;idx<=(size_t)r;idx++){
			char c=buff[idx];
			if(c=='='){
				kb[xi]=0;
				expr=true;
				xi=0;
			}else if(c=='\n'||c=='\r'){
				vb[xi]=0;
				if(xi>0&&expr){
					if(strcmp(kb,"DEVNAME")==0&&!name)name=strdup(vb);
					if(strcmp(kb,"MAJOR")==0&&major<0)major=parse_int(vb,-1);
					if(strcmp(kb,"MINOR")==0&&minor<0)minor=parse_int(vb,-1);
				}
				xi=0;
				kb[xi]=0;
				vb[xi]=0;
				expr=false;
			}else if(expr)vb[xi++]=c;
			else kb[xi++]=c;
		}
		memset(&buff,0,PATH_MAX);
	}
	close(uevent);
	if(!name||major<0||minor<0){
		if(name)free(name);
		ERET(EINVAL);
	}
	char dev[PATH_MAX]={0};
	snprintf(dev,PATH_MAX-1,"%s/%s",path,name);
	mkdir_res(dev);
	if(access(dev,F_OK)!=0){
		if(errno!=ENOENT){
			free(name);
			return -errno;
		}
		mknod(dev,type|0600,makedev(major,minor));
		char*t;
		switch(type){
			case S_IFCHR:t="char";break;
			case S_IFBLK:t="block";break;
			default:t="unknown";break;
		}
		tlog_debug("create %s device(%d:%d) %s",t,major,minor,dev);
	}
	free(name);
	return 0;
}

static int scan_devices(char*path,int dir,int type){
	DIR*d=fdopendir(dir);
	if(!d)goto er;
	struct dirent*r=NULL;
	while((r=readdir(d))){
		if(is_virt_dir(r)||r->d_type!=DT_LNK)continue;
		int f=openat(dir,r->d_name,O_RDONLY|O_DIRECTORY);
		if(f<0)continue;
		do_mknod(path,f,type);
		close(f);
	}
	closedir(d);
	return 0;
	er:
	if(dir>=0)close(dir);
	return -1;
}

int init_devtmpfs(char*path){
	int fd=open(_PATH_SYS_DEV,O_RDONLY|O_DIRECTORY),e=0;
	if(fd<=0)return terlog_error(-errno,"open %s failed",_PATH_SYS_DEV);
	e=MIN(e,scan_devices(path,openat(fd,"char",O_RDONLY|O_DIRECTORY),S_IFCHR));
	e=MIN(e,scan_devices(path,openat(fd,"block",O_RDONLY|O_DIRECTORY),S_IFBLK));
	close(fd);
	add_link(path,"stdin",_PATH_PROC_SELF"/fd/0");
	add_link(path,"stdout",_PATH_PROC_SELF"/fd/1");
	add_link(path,"stderr",_PATH_PROC_SELF"/fd/2");
	add_link(path,"fd",_PATH_PROC_SELF"/fd");
	return e;
}

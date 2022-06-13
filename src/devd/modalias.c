/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<dirent.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include"pathnames.h"
#include"defines.h"
#include"system.h"
#include"logger.h"
#include"str.h"
#define TAG "modalias"

static int scan_modalias(int dir);

static int scan_modalias_dir(int dir,char*name){
	if(!name||dir<0)return -1;
	int f,e;
	if((f=openat(dir,name,O_DIR))>=0){
		e=scan_modalias(f);
		close(f);
	}else e=-errno;
	return e;
}

static int scan_modalias_file(int dir,char*name){
	if(!name||dir<0)return -1;
	int f,e;
	char buff[PATH_MAX]={0};
	if(strcmp(name,"modalias")!=0)return 0;
	if((f=openat(dir,name,O_RDONLY))>=0){
		ssize_t r=read(f,buff,PATH_MAX-1);
		if(r>0){
			trim(buff);
			e=insmod(buff,false);
		}else e=-errno;
		close(f);
	}else e=-errno;
	return e;
}

static int scan_modalias_dirent(int dir,struct dirent*r){
	if(!r||dir<0)return -1;
	if(is_virt_dir(r))return 0;
	int e=0;
	switch(r->d_type){
		case DT_DIR:e+=scan_modalias_dir(dir,r->d_name);break;
		case DT_REG:e+=scan_modalias_file(dir,r->d_name);break;
	}
	return e;
}

static int scan_modalias(int dir){
	DIR*d=fdopendir(dir);
	if(!d)goto er;
	int e=0;
	struct dirent*r=NULL;
	while((r=readdir(d)))e+=scan_modalias_dirent(dir,r);
	closedir(d);
	return e;
	er:
	if(dir>=0)close(dir);
	ERET(ENOMEM);
}

int load_modalias(){
	int dfd=open(_PATH_SYS_DEVICES,O_DIR);
	if(dfd<0)return -errno;
	int r=scan_modalias(dfd);
	close(dfd);
	return r;
}

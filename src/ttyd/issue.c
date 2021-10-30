/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<time.h>
#include<stdlib.h>
#include<string.h>
#include<sys/utsname.h>
#include"str.h"
#include"confd.h"
#include"keyval.h"
#include"system.h"
#include"version.h"
#include"ttyd_internal.h"
#define TIME_STR_DAY "%A, %d %B %Y"
#define TIME_STR_TIME "%H:%M:%S"

char*tty_issue_replace(char*src,char*dest,size_t dest_len,struct tty_data*data){
	time_t t;
	struct utsname u;
	char tm_buf1[64]={0},tm_buf2[64]={0};
	if(!src||!dest||!data||uname(&u)!=0)return NULL;
	time(&t);
	keyval*v[]={
		&KV("s",u.sysname),
		&KV("n",u.nodename),
		&KV("h",u.nodename),
		&KV("r",u.release),
		&KV("v",u.version),
		&KV("m",u.machine),
		&KV("D",u.domainname),
		&KV("o",u.domainname),
		&KV("d",time2nstr(&t,TIME_STR_DAY,tm_buf1,63)),
		&KV("t",time2nstr(&t,TIME_STR_TIME,tm_buf2,63)),
		&KV("l",data->name),
		&KV("S",NAME),
		&KV("V",VERSION),
		NULL
	};
	return replace(v,'\\',dest,src,dest_len);
}

char*tty_issue_read(char*path,char*dest,size_t dest_len,struct tty_data*data){
	if(!dest||!data||dest_len==0)return NULL;
	char*buff=malloc(dest_len*2);
	if(!buff)return NULL;
	memset(buff,0,dest_len*2);
	if(!path||read_file(buff,dest_len*2-1,true,path)<0){
		free(buff);
		buff=confd_get_string("ttyd.issue",NULL);
		if(!buff)return NULL;
	}
	char*rest=tty_issue_replace(buff,dest,dest_len,data);
	free(buff);
	return rest;
}

char*tty_issue_get(char*dest,size_t dest_len,struct tty_data*data){
	if(!dest||!data||dest_len==0)return NULL;
	char*path=confd_get_string("ttyd.issue_file",NULL);
	char*rest=tty_issue_read(path,dest,dest_len,data);
	if(path)free(path);
	return rest;
}

ssize_t tty_issue_write(int fd,struct tty_data*data){
	if(fd<0||!data)return -1;
	char buff[BUFSIZ]={0};
	if(!tty_issue_get(buff,BUFSIZ,data))return -1;
	return write(fd,buff,strlen(buff));
}

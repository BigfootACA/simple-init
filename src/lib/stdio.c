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
#include<stddef.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/ioctl.h>
#include<sys/resource.h>
#include<linux/vt.h>
#include"pathnames.h"
#include"system.h"
#include"defines.h"
#include"output.h"
#include"str.h"

void fd_vperror(int fd,const char*format,va_list a){
	char buff[BUFFER_SIZE];
	memset(&buff,0,BUFFER_SIZE);
	if(format)vsnprintf(buff,BUFFER_SIZE-1,format,a);
	dprintf(fd,errno!=0?"%s: %m\n":"%s.\n",buff);
}

void fd_perror(int fd,const char*format,...){
	va_list a;
	if(format)va_start(a,format);
	fd_vperror(fd,format,a);
	if(format)va_end(a);
}

int get_term_width(int fd,int def){
	int c=def;
	#if defined(TIOCGWINSZ)
	struct winsize w_win;
        if(ioctl(fd,TIOCGWINSZ,&w_win)==0)return w_win.ws_col;
	#elif defined(TIOCGSIZE)
	struct ttysize t_win;
        if(ioctl(fd,TIOCGSIZE,&t_win)==0)return t_win.ts_cols;
	#else
	(void)fd;
	#endif
        return c;
}

char skips(int fd,char stop[]){
	if(fd<0)return -EINVAL;
	size_t a;
	char bit[2]={0,0};
	while(read(fd,&bit,1)==1){
		a=0;
		while(a<strlen(stop))if(stop[a++]==bit[0])goto re;
	}
	re:
	errno=0;
	return bit[0];
}

int get_max_fd(){
	struct rlimit rl;
	rlim_t m;
	if(getrlimit(RLIMIT_NOFILE,&rl)<0)return -errno;
	m=MAX(rl.rlim_cur,rl.rlim_max);
	if(m<FD_SETSIZE)return FD_SETSIZE-1;
	if(m==RLIM_INFINITY||m>INT_MAX)return INT_MAX;
	return (int)(m-1);
}

static bool int_in_array(int val,const int*array,int count){
	if(!array)return false;
	for(int i=0;i<count;i++)if(array[i]==val)return true;
	return false;
}

int close_all_fd(const int*exclude,int count){
	DIR*d=NULL;
	int r=0,fd,max_fd;
	if(!(d=opendir(_PATH_PROC_SELF"/fd"))){
		if((max_fd=get_max_fd())<0)return max_fd;
		for(fd=3;fd>=0;fd=fd<max_fd?fd+1:-1)
			if(!int_in_array(fd,exclude,count))close(fd);
		return r;
	}
	struct dirent*de;
	while((de=readdir(d))){
		if((fd=parse_int(de->d_name,-1))<0)continue;
		if(fd<3||fd==dirfd(d))continue;
		if(!int_in_array(fd,exclude,count))close(fd);
	}
	closedir(d);
	return r;
}

int set_active_console(int vt){
	int fd=-1;
	if((fd=open(_PATH_DEV"/tty0",O_RDWR))<0)goto f;
	if(ioctl(fd,VT_ACTIVATE,(void*)(ptrdiff_t)vt)<0)goto f;
	if(ioctl(fd,VT_WAITACTIVE,(void*)(ptrdiff_t)vt)<0)goto f;
	f:
	if(fd>0)close(fd);
	return errno==0?0:-1;
}

char**get_active_consoles(){
	void*v=NULL;
	char**arr,*tty,*p,**r=NULL;
	size_t arr_len=16,arr_size=sizeof(char*)*arr_len;
	size_t tty_len=16,tty_size=sizeof(char)*tty_len*arr_len;
	size_t arr_off=0,tty_off=0,buf_size=tty_size+arr_size;
	if(!(v=malloc(buf_size)))goto fail;
	memset(v,0,buf_size);
	arr=(char**)v,tty=(char*)v+arr_size;
	if(read_file(
		tty,tty_size,false,
		_PATH_SYS_CLASS"/tty/console/active"
	)<0)goto fail;
	if(*tty){
		arr[arr_off++]=tty;
		while(
			tty_off<tty_size-1&&
			arr_off<arr_len-1&&
			*(p=tty+tty_off)
		){
			if(isspace(*p)){
				*p=0;
				if(!*(p-1))arr_off--;
				arr[arr_off++]=p+1;
			}
			tty_off++;
		}
	}
	r=arr;
	fail:
	if(v&&!r)free(v);
	return r;
}

ssize_t full_write(int fd,void*data,size_t len){
	ssize_t sent;
	size_t rxl=len;
	void*rxd=data;
	do{
		errno=0;
		sent=write(fd,rxd,rxl);
		if(sent<0&&errno!=EAGAIN&&errno!=EINTR)return sent;
		rxl-=sent,rxd+=sent;
	}while(rxl>0);
	return (ssize_t)len;
}

ssize_t full_read(int fd,void*data,size_t len){
	ssize_t sent;
	size_t rxl=len;
	void*rxd=data;
	do{
		errno=0;
		sent=read(fd,rxd,rxl);
		if(sent<0&&errno!=EAGAIN&&errno!=EINTR)return sent;
		rxl-=sent,rxd+=sent;
	}while(rxl>0);
	return (ssize_t)len;
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<libintl.h>
#include"defines.h"
#include"output.h"

int ret_perror(int err,bool quit,const char*format,...){
	va_list a;
	if(format)va_start(a,format);
	fd_vperror(STDERR_FILENO,_(format),a);
	if(format)va_end(a);
	if(quit)exit(err);
	return err;
}

int ret_stdout_perror(int err,bool quit,const char*format,...){
	stdout_perror("%s",_(format));
	if(quit)exit(err);
	return err;
}

int ret_printf(int err,bool quit,int fd,const char*format,...){
	va_list a;
	char buff[BUFFER_SIZE];
	memset(&buff,0,BUFFER_SIZE);
	va_start(a,format);
	vsnprintf(buff,BUFFER_SIZE-1,_(format),a);
	va_end(a);
	dprintf(fd,"%s",buff);
	if(quit)exit(err);
	return err;
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include"getopt.h"
#include"defines.h"
#include"output.h"
static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: cat [OPTIONS]... [FILE]...\n"
		"Concatenate FILE(s) to standard output.\n"
		"Options:\n"
		"\t-h, --help           show this help\n"
	);
}
int cat_fd(int fd){
	ssize_t r;
	char buf[BUFSIZ];
	while(1){
		memset(buf,0,BUFSIZ);
		if((r=read(fd,buf,BUFSIZ-1))<=0)break;
		if(write(STDOUT_FILENO,buf,(size_t)r)!=r)break;
	}
	return 0;
}
int cat_file(char*file){
	if(!file)return -1;
	if(file[0]=='-'&&file[1]==0)return cat_fd(STDIN_FILENO);
	int fd=open(file,O_RDONLY);
	if(fd<0)return ret_stderr_perror(1,false,"cat: %s",file);
	cat_fd(fd);
	close(fd);
	return 0;
}
static int max(int a,int b){return MAX(a,b);}
int cat_main(int argc,char**argv){
	static const struct option lo[]={
		{"help", no_argument, NULL,'h'},
		{NULL,0,NULL,0}
	};
	int o=b_getlopt(argc,argv,"h",lo,NULL);
	if(o>=0)switch(o){
		case 'h':return usage(0);
		default:return 1;
	}
	int e=0;
	if(b_optind==argc)e=max(0,cat_fd(STDIN_FILENO));
	else for(int i=b_optind;i<argc;i++)e=max(0,cat_file(argv[i]));
	return e;
}
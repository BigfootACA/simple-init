/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include"shell_internal.h"
#include"defines.h"
#include"pathnames.h"
#include"output.h"
#include"init.h"
#include"array.h"
#include"str.h"

static int ext_errno(){
	switch(errno){
		case 0:return 0;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOENT:
		case ENOTDIR:return 127;
		default:return 126;
	}
}

static int exec_cmd(char*path,char**argv){
	int r;
	errno=0;
	execv(path,argv);
	if((r=ext_errno())!=0)perror(argv[0]);
	_exit(r);
}

int run_external_cmd(char**argv,bool background){
	if(!argv||!argv[0])ERET(EINVAL);
	char cp[PATH_MAX],*paths;
	memset(cp,0,PATH_MAX);
	strncpy(cp,argv[0],PATH_MAX-1);
	if(!contains_of(cp,PATH_MAX,'/')){
		bool exists=false;
		if(!(paths=getenv("PATH")))paths=_PATH_DEFAULT_PATH;
		memset(cp,0,PATH_MAX);
		char**ps=args2array(paths,':');
		if(!ps){
			snprintf(cp,PATH_MAX-1,"%s/%s",paths,argv[0]);
			exists=access(cp,F_OK)==0;
		}else{
			char*p;
			for(int i=0;(p=ps[i]);i++){
				snprintf(cp,PATH_MAX-1,"%s/%s",p,argv[0]);
				if((exists=access(cp,F_OK)==0))break;
				memset(cp,0,PATH_MAX);
			}
			free_args_array(ps);
		}
		if(!exists)return re_printf(127,"%s: not found\n",argv[0]);
	}
	struct stat st;
	if(stat(cp,&st)!=0){
		perror(argv[0]);
		return ext_errno();
	}
	errno=0;
	if(S_ISDIR(st.st_mode))return re_printf(127,"%s: is a directory\n",argv[0]);
	if(!S_ISREG(st.st_mode)||access(cp,X_OK)!=0){
		if(errno==0)errno=EACCES;
		return re_err(ext_errno(),"%s",argv[0]);
	}
	pid_t p=fork();
	switch(p){
		case -1:return re_err(errno,"%s: fork",TAG);
		case 0:return exec_cmd(cp,argv);
		default:return background?0:wait_cmd(p);
	}
}

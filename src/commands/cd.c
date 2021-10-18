/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"output.h"
#include"pathnames.h"
#include"defines.h"

int xchdir(const char*path){
	if(!path)ERET(EINVAL);
	char old[PATH_MAX]={0};
	setenv("OLDPWD",getcwd(old,PATH_MAX),1);
	int r=chdir(path);
	if(r<0)return re_err(1,"cd: %s",path);
	return 0;
}

int cd_main(int argc,char**argv){
	if(argc<1)ERET(EINVAL);
	if(argc>2)return re_printf(1,"cd: too many arguments\n");
	char*path=argv[1];
	if(argc==1||!path){
		path=getenv("HOME");
		if(!path)path=_PATH_ROOT;
	}else if(strcmp(argv[1],"-")==0){
		path=getenv("OLDPWD");
		if(!path)return re_printf(1,"cd: OLDPWD not set\n");
	}
	return xchdir(path);
}

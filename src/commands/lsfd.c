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
#include<string.h>
#include"output.h"
#include"pathnames.h"
#include"defines.h"

int lsfd_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: lsfd\n");
	int dfd=open(_PATH_PROC_SELF"/fd",O_DIR);
	if(dfd<0)return re_err(1,"open folder failed");
	DIR*d=fdopendir(dfd);
	if(!d)return re_err(1,"open folder failed");
	int r=0;
	char buf[BUFSIZ];
	struct dirent*e;
	while((e=readdir(d))){
		if(e->d_type!=DT_LNK)continue;
		memset(buf,0,BUFSIZ);
		if(readlinkat(dfd,e->d_name,buf,BUFSIZ-1)<0){
			stderr_perror("readlink %s failed",e->d_name);
			r=1;
			continue;
		}
		printf("%s: %s\n",e->d_name,buf);
	}
	closedir(d);
	return r;
}

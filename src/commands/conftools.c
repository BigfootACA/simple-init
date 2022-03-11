/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include"confd.h"
#include"output.h"
#include"defines.h"

extern int confctl_do_get(char*key);
int confget_main(int argc,char**argv){
	if(argc!=2)return re_printf(1,"Usage: confget <KEY>\n");
	return confctl_do_get(argv[1]);
}

extern int confctl_do_set(char*key,char*value);
int confset_main(int argc,char**argv){
	if(argc!=3)return re_printf(1,"Usage: confset <KEY> <VALUE>\n");
	return confctl_do_set(argv[1],argv[2]);
}

int confdel_main(int argc,char**argv){
	if(argc!=2)return re_printf(1,"Usage: confdel <KEY>\n");
	errno=0;
	confd_delete(argv[1]);
	if(errno!=0)perror(_("config daemon delete failed"));
	return errno;
}

int confdump_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: confdump\n");
	errno=0;
	confd_dump(LEVEL_DEBUG);
	if(errno!=0)perror(_("config daemon dump failed"));
	return errno;
}

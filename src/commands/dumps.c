/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdio.h>
#include"logger.h"
#include"output.h"
#include"init.h"

int dumpenv_main(int argc,char **argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: dumpenv\n");
	dump_environ(STDOUT_FILENO);
	return 0;
}

int logdumpenv_main(int argc,char **argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: logdumpenv\n");
	open_socket_logfd_default();
	log_environ(LEVEL_INFO,"dumpenv");
	return 0;
}

int dumpargv_main(int argc,char**argv){
	for(int i=0;i<argc;i++)
		printf("argv[%d]=%s\n",i,argv[i]);
	return 0;
}

int logdumpargv_main(int argc,char**argv){
	open_socket_logfd_default();
	for(int i=0;i<argc;i++)
		log_info("logdumpargv","argv[%d]=%s",i,argv[i]);
	close_logfd();
	return 0;
}

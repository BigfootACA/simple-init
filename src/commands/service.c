/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<string.h>
#include"output.h"
#include"logger.h"
#include"getopt.h"
#include"init_internal.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: service [SERVICE] [OPERATION]\n"
		"Control service operation (SysV Compatible).\n\n"
		"Operation:\n"
		"\tstart                  Start service\n"
		"\tstop                   Stop service\n"
		"\trestart                Re-Start service\n"
		"\treload                 Re-Load service\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Use custom initd socket\n"
		"\t-h, --help             Display this help and exit\n"
	);
}

int service_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"socket",  required_argument, NULL,'s'},
		{NULL,0,NULL,0}
	};
	char*socket=NULL;
	int o;
	while((o=b_getlopt(argc,argv,"hs:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
		break;
		default:return 1;
	}
	if(!socket)socket=DEFAULT_INITD;
	if(open_socket_initfd(socket,false)<0)return 2;
	if(b_optind>=argc)return usage(1);
	int ac=argc-b_optind;
	char**av=argv+b_optind;
	if(ac>2)return re_printf(2,"too many arguments\n");
	if(ac<2)return re_printf(2,"missing arguments\n");
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_NONE);
	size_t len=strlen(av[0]);
	if(!av[0]||len<=0)return re_printf(2,"invalid service name\n");
	if(len>=sizeof(msg.data.data))return re_printf(2,"arguments too long\n");
	if(strcmp(av[1],"start")==0)msg.action=ACTION_SVC_START;
	else if(strcmp(av[1],"stop")==0)msg.action=ACTION_SVC_STOP;
	else if(strcmp(av[1],"reload")==0)msg.action=ACTION_SVC_RELOAD;
	else if(strcmp(av[1],"restart")==0)msg.action=ACTION_SVC_RESTART;
	else return re_printf(2,"invalid operation %s\n",av[1]);
	strncpy(msg.data.data,av[0],sizeof(msg.data.data)-1);
	init_send(&msg,&response);
	if(errno!=0)perror(_("send command"));
	if(response.data.status.ret!=0)fprintf(
		stderr,_("execute %s: %s\n"),av[1],
		strerror(response.data.status.ret)
	);
	return response.data.status.ret;
	conflict:return re_printf(2,"too many arguments\n");
}

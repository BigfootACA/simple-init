/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include"output.h"
#include"getopt.h"
#include"system.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: chvt N\n"
		"Change foreground virtual terminal.\n\n"
		"Arguments:\n"
		"\tN:           /dev/ttyN\n"
		"Options:\n"
		"\t-h, --help   display this help and exit\n"
	);
}

int chvt_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	int o;
	while((o=b_getlopt(argc,argv,"h",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		default:return 1;
	}
	int v=argc-b_optind;
	if(v>1)return re_printf(2,"too many arguments\n");
	if(v<1)return re_printf(2,"missing arguments\n");
	char*value=argv[v],*end;
	int l=(int)strtol(value,&end,10);
	if(*end||value==end||errno!=0)
		return re_printf(2,"invalid virtual terminal number\n");
	return set_active_console(l)==0?
		0:re_err(2,"change virtual terminal failed");
}

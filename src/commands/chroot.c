/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<unistd.h>
#include"output.h"
#include"getopt.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: chroot [OPTIONS] <ROOT> [COMMAND [ARGS]...]\n"
		"Change to new root and execute command.\n\n"
		"Options:\n"
		"\t-s, --skip-chdir  do not change working directory to '/'\n"
		"\t-d, --chdir=DIR   change working directory to DIR\n"
		"\t-h, --help        display this help and exit\n"
		"If no command is given, run \"$SHELL -i\"."
	);
}

int chroot_main(int argc,char**argv){
	static const struct option lo[]={
		{"chdir",      required_argument, NULL,'d'},
		{"skip-chdir", no_argument,       NULL,'s'},
		{"help",       no_argument,       NULL,'h'},
		{NULL,0,NULL,0}
	};
	char*dir="/";
	int o;
	while((o=b_getlopt(argc,argv,"hsd:u:g:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'd':dir=b_optarg;break;
		case 's':dir=NULL;break;
		default:return 1;
	}
	int ac=argc-b_optind;
	char**av=argv+b_optind;
	char**rv=av+1;
	if(ac<1)return re_printf(2,"missing arguments\n");
	if(ac==1)rv=(char*[]){getenv("SHELL"),"-i",NULL};
	if(!rv[0])return re_printf(2,"no command specified\n");
	if(chroot(av[0])!=0)return re_err(2,"chroot failed");
	if(dir&&chdir(dir)!=0)return re_err(2,"chdir failed");
	int r=execvp(rv[0],rv);
	if(r!=0)return re_err(2,"execvp failed");
	return r;
}

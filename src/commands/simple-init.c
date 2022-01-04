/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<strings.h>
#include"shell.h"
#include"output.h"
#include"defines.h"
#include"version.h"

static int usage(int e){
	return r_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: simple-init <CMD> [OPTIONS]...\n"
		"A linux system in a single binary.\n\n"
		"%s\n",VERSION_INFO
	);
}

static int version(int e){
	puts(VERSION_INFO);
	return e;
}

int simple_init_main(int argc,char**argv){
	if(argc<=1)return getpid()==1&&
		getuid()==0&&geteuid()==0&&
		getgid()==0&&getegid()==0?
		invoke_internal_cmd_nofork_by_name("init",argv):
		usage(1);
	if(strcasecmp(argv[1],"--help")==0)return usage(0);
	else if(strcasecmp(argv[1],"-h")==0)return usage(0);
	else if(strcasecmp(argv[1],"--version")==0)return version(0);
	else if(strcasecmp(argv[1],"-v")==0)return version(0);
	else if(argv[1][0]=='-')return re_printf(
		1,"%s: unrecognized option: %s\n",
		program_invocation_name,argv[1]
	);
	int r=invoke_internal_cmd_nofork_by_name(argv[1],argv+1);
	if(errno!=0)fprintf(stderr,_("%s: command not found\n"),argv[1]);
	return r;
}

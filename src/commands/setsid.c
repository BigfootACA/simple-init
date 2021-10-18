/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/wait.h>
#include"getopt.h"
#include"output.h"

static int usage(int e){
	return r_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: setsid [OPTIONS] MODULENAME ...\n"
		"Run a program in a new session.\n\n"
		"Options:\n"
		"\t-c, --ctty     set the controlling terminal to the current one\n"
		"\t-f, --fork     always fork\n"
		"\t-w, --wait     wait program to exit, and use the same return\n"
	);
}

int setsid_main(int argc, char **argv){
	int ch,forcefork=0,ctty=0,status=0;
	pid_t pid;
	static const struct option lo[]={
		{"ctty", no_argument, NULL, 'c'},
		{"fork", no_argument, NULL, 'f'},
		{"wait", no_argument, NULL, 'w'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	while((ch=b_getlopt(argc,argv,"hcfw",lo,NULL))!=-1)switch(ch){
		case 'c':ctty=1;break;
		case 'f':forcefork=1;break;
		case 'w':status=1;break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(argc-b_optind<1)return re_printf(1,"no command specified\n");
	if(forcefork||getpgrp()==getpid())switch((pid=fork())){
		case -1:return re_err(1,"fork");
		case 0:break;
		default:
			if(!status)return 0;
			if(wait(&status)!=pid)return re_err(1,"wait");
			if(WIFEXITED(status))return WEXITSTATUS(status);
		return re_err(1,"child %d did not exit normally", pid);
	}
	if(setsid()<0)return re_err(1,"setsid failed");
	if(ctty&&ioctl(STDIN_FILENO,TIOCSCTTY,1))return re_err(1,"failed to set the ctty");
	execvp(argv[b_optind],argv+b_optind);
	return re_err(1,"exec %s",argv[b_optind]);
}

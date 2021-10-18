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
#include<string.h>
#include<unistd.h>
#include<libintl.h>
#include<sys/prctl.h>
#include"shell_internal.h"
#include"output.h"
#include"init.h"
#include"array.h"
#include"str.h"

struct shell_command*find_internal_cmd(char*name){
	if(!name||strlen(name)<=0)EPRET(EINVAL);
	struct shell_command*cmd;
	for(int i=0;(cmd=(struct shell_command*)shell_cmds[i]);i++){
		if(!cmd->enabled)continue;
		if(strncmp(cmd->name,name,sizeof(cmd->name))!=0)continue;
		errno=0;
		return cmd;
	}
	EPRET(ENOENT);
}

int install_cmds(int dfd){
	char*exe=_PATH_PROC_SELF"/exe",buff[PATH_MAX]={0};
	if(access(exe,R_OK)!=0)return terlog_error(-errno,"cannot read %s",exe);
	if(readlink(exe,buff,sizeof(buff))<0)return terlog_error(-errno,"readlink failed");
	if(strlen(buff)<=0)return -1;

	struct shell_command*cmd;
	for(int i=0;(cmd=(struct shell_command*)shell_cmds[i]);i++){
		if(!cmd->enabled||!cmd->fork)continue;
		symlinkat(buff,dfd,cmd->name);
	}
	return 0;
}

int invoke_internal_cmd_nofork(struct shell_command*cmd,char**args){
	if(!cmd||!cmd->main)ERET(EINVAL);
	char*s1=program_invocation_name,*s2=program_invocation_short_name;
	program_invocation_name=cmd->name;
	program_invocation_short_name=basename(cmd->name);
	prctl(PR_SET_NAME,program_invocation_short_name,0,0,0);
	char**a=args?args:(char*[]){cmd->name,NULL};
	int argc=0;
	while(a[++argc]);
	int r=cmd->main(argc,a);
	program_invocation_name=s1;
	program_invocation_short_name=s2;
	return r;
}

int invoke_internal_cmd(struct shell_command*cmd,bool background,char**args){
	if(!cmd)ERET(EINVAL);
	if(cmd->fork){
		pid_t p=fork();
		switch(p){
			case 0:break;
			case -1:return ret_perror(errno,false,"%s: fork",TAG);
			default:return background?0:wait_cmd(p);
		}
		unsetenv("INIT_MAIN");
		execv(_PATH_PROC_SELF"/exe",args);
		return -1;
	}else return invoke_internal_cmd_nofork(cmd,args);
}

int invoke_internal_cmd_by_name(char*name,bool background,char**args){
	struct shell_command*cmd;
	if(!name)ERET(EINVAL);
	if(!(cmd=find_internal_cmd(name)))ERET(ENOENT);
	int r=invoke_internal_cmd(cmd,background,args);
	errno=0;
	return r;
}

int invoke_internal_cmd_nofork_by_name(char*name,char**args){
	struct shell_command*cmd;
	if(!name)ERET(EINVAL);
	if(!(cmd=find_internal_cmd(name)))ERET(ENOENT);
	int r=invoke_internal_cmd_nofork(cmd,args);
	errno=0;
	return r;
}

int init_commands_locale(){
	struct shell_command*cmd;
	for(int i=0;(cmd=(struct shell_command*)shell_cmds[i]);i++){
		if(!cmd->help[0])continue;
		char*text=gettext(cmd->help);
		if(!text)continue;
		cmd->help=text;
	}
	return 0;
}

int run_cmd(char**args,bool background){
	if(!args)ERET(EINVAL);
	int r;
	struct shell_command*cmd=NULL;
	char**argv=array_dup(args);
	if(!argv)ERET(ENOMEM);
	if(!contains_of(argv[0],strlen(argv[0]),'/'))cmd=find_internal_cmd(argv[0]);
	r=cmd?invoke_internal_cmd(cmd,background,argv):run_external_cmd(argv,background);
	array_free(argv);
	return r;
}

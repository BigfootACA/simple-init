#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
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

int invoke_internal_cmd_nofork(struct shell_command*cmd,char**args){
	if(!cmd||!cmd->main)ERET(EINVAL);
	char*s1=program_invocation_name,*s2=program_invocation_short_name;
	program_invocation_name=cmd->name;
	program_invocation_short_name=basename(cmd->name);
	int argc=0;
	while(args[++argc]);
	int r=cmd->main(argc,args);
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
	}
	int r=invoke_internal_cmd_nofork(cmd,args);
	if(cmd->fork)_exit(r);
	return r;
}

int run_cmd(char**args,bool background){
	if(!args)ERET(EINVAL);
	char**argv=array_dup(args);
	if(!argv)ERET(ENOMEM);
	if(!contains_of(argv[0],strlen(argv[0]),'/')){
		struct shell_command*cmd=find_internal_cmd(argv[0]);
		if(cmd)return invoke_internal_cmd(cmd,background,argv);
	}
	int r=run_external_cmd(argv,background);
	array_free(argv);
	return r;
}

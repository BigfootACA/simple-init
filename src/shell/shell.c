/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_READLINE
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/prctl.h>
#include<sys/select.h>
#include<readline/readline.h>
#include<readline/history.h>
#include"shell_internal.h"
#include"proctitle.h"
#include"service.h"
#include"system.h"
#include"output.h"
#include"getopt.h"
#include"array.h"
#include"confd.h"

#define DEF_PS1 "\\$ "

static bool sigwinch=false;
bool shell_running=false,shell_executing=false;
unsigned char exit_code=0;
static char prompt[1024],*last=NULL;

static void update_prompt(){
	char*ps1=getenv("PS1");
	if(!ps1)ps1=DEF_PS1;
	shell_replace(prompt,ps1,sizeof(prompt)-1);
	rl_set_prompt(prompt);
}

static void shell_exit(int x){
	if(last)free(last);
	last=NULL;
	if(shell_running){
		printf("\nexit\n");
		rl_callback_handler_remove();
	}
	shell_running=false;
	exit(x);
}

static void sighand(int s __attribute__((unused))){
	switch(s){
		case SIGINT:
			if(shell_executing||!shell_running)return;
			printf("^C\r\n");
			exit_code=128|s;
			update_prompt();
			rl_replace_line("",0);
			rl_forced_update_display();
		return;
		case SIGWINCH:sigwinch=true;return;
		case SIGHUP:case SIGQUIT:case SIGTERM:shell_exit(s);
		default:;
	}
}

static void linehandler(char*line){
	bool removed=false;
	shell_executing=true;
	if(!line){
		shell_exit(exit_code);
		return;
	}
	char**args;
	if((args=args2array(line,0))){
		if(args[0]){
			if(shell_running){
				switch(line[0]){
					case ' ':case '\t':case '\n':break;
					default:if(
						!last||
						strlen(last)!=strlen(line)||
						strcmp(line,last)!=0
					)add_history(line);
				}
				rl_callback_handler_remove();
			}
			exit_code=run_cmd(args,false);
			removed=true;
		}
		free_args_array(args);
	}
	if(last)free(last);
	last=line;
	if(shell_running){
		update_prompt();
		if(removed)rl_callback_handler_install(prompt,linehandler);
	}
	shell_executing=false;
}

static char*cmd_generator(const char*text,int state){
	static DIR*d=NULL;
	static char**x=NULL;
	static size_t i,s,len;
	if(!state){
		if(d)closedir(d);
		if(x)free_args_array(x);
		len=strlen(text);
		s=0,i=0;
		x=args2array(getenv("PATH"),':');
		if(x&&x[i])d=opendir(x[i]);
	}
	const struct shell_command*cmd;
	while((cmd=shell_cmds[s])){
		s++;
		if(!cmd->enabled||!cmd->name[0])continue;
		if(strncmp(cmd->name,text,len)==0)return strdup(cmd->name);
	}
	if(x)for(;;){
		if(d){
			struct dirent*e=NULL;
			while((e=readdir(d))){
				switch(e->d_type){
					case DT_REG:case DT_LNK:break;
					default:continue;
				}
				if(strncmp(e->d_name,text,len)==0)return strdup(e->d_name);
			}
			closedir(d);
			d=NULL;
		}
		i++;
		if(!x[i])break;
		d=opendir(x[i]);
	}
	return NULL;
}

static char**initshell_completion(const char*text,int start,int end __attribute__((unused))){
	return start==0?rl_completion_matches(text,cmd_generator):NULL;
}

void run_shell(){
	int r;
	fd_set fds;
	handle_signals((int[]){SIGINT,SIGTERM,SIGQUIT,SIGWINCH,SIGHUP},5,sighand);
	setproctitle("initshell");
	prctl(PR_SET_NAME,"Init Shell",0,0,0);
	if(!getenv("PS1"))setenv("PS1","\\VINIT(\\u@\\h):\\w \\$ ",1);
	update_prompt();
	rl_callback_handler_install(prompt,linehandler);
	rl_attempted_completion_function=initshell_completion;
	shell_running=true;
	while(shell_running){
		FD_ZERO(&fds);
		FD_SET(fileno(rl_instream),&fds);
		r=select(FD_SETSIZE,&fds,NULL,NULL,NULL);
		if(r<0){
			if(errno!=EINTR){
				perror("select");
				rl_callback_handler_remove();
				break;
			}else continue;
		}
		if(sigwinch)rl_resize_terminal();
		sigwinch=false;
		if(FD_ISSET(fileno(rl_instream),&fds))
		rl_callback_read_char();
	}
	shell_exit(exit_code);
}

int initshell_main(int argc,char**argv){
	static const char*so="c:";
	static const struct option lo[]={
		{"command", required_argument, NULL,'c'},
		{NULL,0,NULL,0}
	};
	int o;
	if(argc>1){
		if((o=b_getlopt(argc,argv,so,lo,NULL))!=-1)switch(o){
			case 'c':
				linehandler(b_optarg);
			return exit_code;
			default:return 2;
		}
		if(argc>b_optind&&strcmp(argv[b_optind],"-")==0)b_optind++;
		if(argc!=b_optind)return re_printf(3,"unsupport script file\n");
	}
	run_shell();
	return exit_code;
}

static int console_shell_service(struct service*svc __attribute__((unused))){
	run_shell();
	return 0;
}

int register_console_shell(){
	struct service*shell=svc_create_service("console-shell",WORK_FOREGROUND);
	if(shell){
		svc_set_desc(shell,"Init Shell on Console");
		svc_set_start_function(shell,console_shell_service);
		shell->auto_restart=true;
		if(confd_get_boolean("runtime.cmdline.console_shell",false))
			svc_add_depend(svc_system,shell);
	}
	return 0;
}

#endif

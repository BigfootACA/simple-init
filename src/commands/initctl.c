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
		"Usage: initctl [OPTION]...\n"
		"Control init daemon.\n\n"
		"Commands:\n"
		"\tpoweroff                  Power-off system\n"
		"\thalt                      Halt system\n"
		"\treboot [STRING]           Reboot system\n"
		"\tswitchroot <ROOT> [INIT]  Switch to new root\n"
		"\tsetenv <KEY> <VALUE>      Add environment variable\n"
		"\taddenv <KEY>=<VALUE>      Add environment variable\n"
		"\tlanguage <LANGUAGE>       Set system current language\n"
		"\tstart <SERVICE>           Start service\n"
		"\tstop <SERVICE>            Stop service\n"
		"\trestart <SERVICE>         Re-Start service\n"
		"\treload <SERVICE>          Re-Load service\n"
		"\tdump                      Dump all service to loggerd\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>     Use custom initd socket\n"
		"\t-h, --help                Display this help and exit\n"
	);
}

static int cmd_wrapper(struct init_msg*msg,char*name){
	struct init_msg response;
	init_send(msg,&response);
	if(errno!=0)perror(_("send command"));
	if(response.data.status.ret!=0)fprintf(
		stderr,
		_("execute %s: %s\n"),
		name,
		strerror(response.data.status.ret)
	);
	return response.data.status.ret;
}

static int cmd_help(int argc,char**argv __attribute__((unused))){
	return argc>1?re_printf(2,"too many arguments\n"):usage(0);
}

static int cmd_poweroff(int argc,char**argv){
	if(argc>1)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_POWEROFF);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_halt(int argc,char**argv){
	if(argc>1)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_HALT);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_reboot(int argc,char**argv){
	if(argc>2)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	size_t ss=sizeof(msg.data.data);
	init_initialize_msg(&msg,ACTION_REBOOT);
	if(argc==2){
		if(strlen(argv[1])>=ss)return re_printf(2,"arguments too long\n");
		strncpy(msg.data.data,argv[1],ss-1);
	}
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_switchroot(int argc,char**argv){
	if(argc>3)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	#define root msg.data.newroot.root
	#define init msg.data.newroot.init
	struct init_msg msg;
	size_t s1=sizeof(root),s2=sizeof(init);
	if(
		strlen(argv[1])>=s1||
		(argc==3&&strlen(argv[2])>=s2)
	)return re_printf(2,"arguments too long\n");
	init_initialize_msg(&msg,ACTION_SWITCHROOT);
	strncpy(root,argv[1],s1-1);
	if(argc==3)strncpy(init,argv[2],s2-1);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_setenv(int argc,char**argv){
	if(argc>3)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	#define xkey msg.data.env.key
	#define xvalue msg.data.env.value
	char*key,*value;
	struct init_msg msg;
	size_t klen=0,vlen=0;
	size_t s1=sizeof(xkey),s2=sizeof(xvalue);
	switch(argc){
		case 2:
			if(!(value=strchr(argv[1],'=')))
				return re_printf(2,"missing arguments\n");
			klen=value-argv[1],vlen=strlen(++value),key=argv[1];
		break;
		case 3:
			key=argv[1],value=argv[2];
			klen=strlen(key),vlen=strlen(value);
		break;
	}
	if(klen<=0)return re_printf(2,"invalid environ name\n");
	if(klen>=s1||vlen>=s2)return re_printf(2,"arguments too long\n");
	init_initialize_msg(&msg,ACTION_ADDENV);
	strncpy(xkey,key,klen);
	strncpy(xvalue,value,vlen);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_language(int argc,char**argv){
	if(argc>2)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strncpy(msg.data.data,argv[1],sizeof(msg.data.data)-1);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_service(int argc,char**argv){
	if(argc>2)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_NONE);
	if(!argv[1]||strlen(argv[1])<=0)return re_printf(2,"invalid service name\n");
	if(strlen(argv[1])>=sizeof(msg.data.data))return re_printf(2,"arguments too long\n");
	if(strcmp(argv[0],"start")==0)msg.action=ACTION_SVC_START;
	else if(strcmp(argv[0],"stop")==0)msg.action=ACTION_SVC_STOP;
	else if(strcmp(argv[0],"reload")==0)msg.action=ACTION_SVC_RELOAD;
	else if(strcmp(argv[0],"restart")==0)msg.action=ACTION_SVC_RESTART;
	else return re_printf(2,"invalid action %s\n",argv[1]);
	strncpy(msg.data.data,argv[1],sizeof(msg.data.data)-1);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_service_dump(int argc,char**argv){
	if(argc>1)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_SVC_DUMP);
	return cmd_wrapper(&msg,argv[0]);
}

struct{
	char*name;
	int(*cmd_handle)(int,char**);
}cmds[]={
	{"help",          cmd_help},
	{"usage",         cmd_help},
	{"?",             cmd_help},
	{"poweroff",      cmd_poweroff},
	{"halt",          cmd_halt},
	{"reboot",        cmd_reboot},
	{"switchroot",    cmd_switchroot},
	{"setenv",        cmd_setenv},
	{"addenv",        cmd_setenv},
	{"language",      cmd_language},
	{"lang",          cmd_language},
	{"start",         cmd_service},
	{"stop",          cmd_service},
	{"restart",       cmd_service},
	{"reload",        cmd_service},
	{"dump",          cmd_service_dump},
	{NULL,NULL}
};

int initctl_main(int argc,char**argv){
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
	if(b_optind>=argc)return re_printf(1,"no command specified\n");
	int ac=argc-b_optind;
	char**av=argv+b_optind,*vn=argv[b_optind];
	for(int i=0;cmds[i].name;i++){
		if(strcmp(vn,cmds[i].name)!=0)continue;
		return cmds[i].cmd_handle(ac,av);
	}
	return re_printf(1,"unknown command: %s\n",vn);
	conflict:return re_printf(2,"too many arguments\n");
}

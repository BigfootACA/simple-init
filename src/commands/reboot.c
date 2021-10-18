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
#include<string.h>
#include<syscall.h>
#include<sys/reboot.h>
#include<linux/reboot.h>
#include"str.h"
#include"output.h"
#include"logger.h"
#include"getopt.h"
#include"init_internal.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: reboot [OPTIONS] [ARG]\n"
		"Reboot the system.\n\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Use custom initd socket\n"
		"\t-d, --delay <DELAY>    Delay interval seconds\n"
		"\t-n, --no-sync          Do not sync when force reboot\n"
		"\t-f, --force            Force reboot (direct call reboot syscall)\n"
		"\t-h, --help             Display this help and exit\n"
	);
}

int reboot_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"force",   no_argument,       NULL,'f'},
		{"no-sync", no_argument,       NULL,'n'},
		{"socket",  required_argument, NULL,'s'},
		{"delay",   required_argument, NULL,'d'},
		{NULL,0,NULL,0}
	};
	int delay=0,o;
	bool no_sync=false,force=false;
	char*socket=NULL;
	while((o=b_getlopt(argc,argv,"hfns:d:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
			case 'f':force=true;break;
			case 'n':no_sync=true;break;
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
			break;
		case 'd':
			if(delay>0)goto conflict;
			delay=parse_int(b_optarg,-1);
			if(delay<0)return re_printf(2,"invalid delay seconds\n");
		break;
		default:return 1;
	}
	if(b_optind>argc)return usage(1);
	int ac=argc-b_optind;
	char**av=argv+b_optind;
	if(ac>1)return re_printf(2,"too many arguments\n");
	if(ac<0)return re_printf(2,"missing arguments\n");
	if(delay>0)sleep(delay);
	if(force){
		if(!no_sync)sync();
		return (ac==1)?(int)syscall(
			SYS_reboot,
			LINUX_REBOOT_MAGIC1,
			LINUX_REBOOT_MAGIC2,
			LINUX_REBOOT_CMD_RESTART2,
			av[0]
		):reboot(LINUX_REBOOT_CMD_RESTART);
	}else{
		struct init_msg msg,response;
		if(!socket)socket=DEFAULT_INITD;
		if(open_socket_initfd(socket,false)<0)return 2;
		init_initialize_msg(&msg,ACTION_REBOOT);
		if(ac==1){
			size_t len=strlen(av[0]);
			if(!av[0]||len<=0)return re_printf(2,"invalid reboot argument\n");
			if(len>=sizeof(msg.data.data))return re_printf(2,"arguments too long\n");
			strncpy(msg.data.data,av[0],sizeof(msg.data.data)-1);
		}
		init_send(&msg,&response);
		if(errno!=0)perror(_("send command"));
		if(response.data.status.ret!=0)fprintf(
			stderr,_("call reboot failed: %s\n"),
			strerror(response.data.status.ret)
		);
		return response.data.status.ret;
	}
	conflict:return re_printf(2,"too many arguments\n");
}

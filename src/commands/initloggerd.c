/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stddef.h>
#include<string.h>
#include<stdlib.h>
#include"output.h"
#include"getopt.h"
#include"logger.h"
#include"init.h"

static struct{
	bool daemon;
	char*output[64];
	char*socket[64];
}opts;

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: initloggerd [OPTION]...\n"
		"Start init logger daemon.\n\n"
		"Options:\n"
		"\t-s, --socket      add listen socket\n"
		"\t-r, --output      add logger daemon output file\n"
		"\t-d, --daemon      running in background\n"
		"\t-f, --foreground  running in foreground\n"
		"\t-h, --help        display this help and exit\n"
	);
}

#define ADD_LIST(tag)\
	if(b_optarg){\
                size_t c=sizeof(opts.tag)/sizeof(char*);\
		for(x=0;opts.tag[x];x++); \
		if(x>=c-1)return re_printf(\
			1,"initloggerd: too many "#tag" specified\n" \
		);\
		if(!(opts.tag[x]=strdup(b_optarg)))return re_printf(\
			1,"initloggerd: add new "#tag" to list failed\n"\
		);\
		break;\
	}

int initloggerd_main(int argc,char**argv){
	int o;
	pid_t pid;
	size_t x;
	static const struct option lo[]={
		{"help",       no_argument,      NULL,'h'},
		{"daemon",     no_argument,      NULL,'d'},
		{"foreground", no_argument,      NULL,'f'},
		{"socket",     required_argument,NULL,'s'},
		{"output",     required_argument,NULL,'o'},
		{NULL,0,NULL,0}
	};
	memset(&opts,0,sizeof(opts));
	while((o=b_getlopt(argc,argv,"hdfs:o:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'd':opts.daemon=true;break;
		case 'f':opts.daemon=false;break;
		case 's':ADD_LIST(socket)
		//fallthrough
		case 'o':ADD_LIST(output)
		//fallthrough
		default:return 1;
	}
	if(start_loggerd(&pid)<0)return re_err(
		2,"initloggerd: start init logger daemon failed"
	);
	char*v;
	for(x=0;(v=opts.output[x]);x++){
		logger_open(v);
		free(v);
	}
	for(x=0;(v=opts.socket[x]);x++){
		logger_listen(v);
		free(v);
	}
	close_logfd();
	return wait_cmd(pid);
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdlib.h>
#include<stdio.h>
#include<sys/klog.h>
#include"str.h"
#include"output.h"
#include"getopt.h"
#include"kloglevel.h"

static struct{
	bool raw,clear;
	int size;
}opts;

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: dmesg [OPTIONS]\n"
		"Print or control the kernel ring buffer\n"
		"Options:\n"
		"\t-s, --size <SIZE>    buffer size\n"
		"\t-n, --level <LEVEL>  set console logging level\n"
		"\t-c, --clear          clear ring buffer after printing\n"
		"\t-r, --raw            print raw message buffer\n"
		"\t-h, --help           show this help\n"
	);
}

int dmesg_main(int argc,char **argv){
	int o;
	char*buf;
	static const struct option lo[]={
		{"size",  required_argument,NULL,'s'},
		{"level", required_argument,NULL,'n'},
		{"clear", no_argument,      NULL,'c'},
		{"raw",   no_argument,      NULL,'r'},
		{"help",  no_argument,      NULL,'h'},
		{NULL,0,NULL,0}
	};
	while((o=b_getlopt(argc,argv,"cs:n:r",lo,NULL))>0)switch(o){
		case 'n':return klogctl(
			SYSLOG_ACTION_CONSOLE_LEVEL,
			NULL,
			parse_int(b_optarg,DEFAULT_KERN_LEVEL)
		)?re_err(1,"klogctl"):0;
		case 'c':opts.clear=true;break;
		case 'r':opts.raw=true;break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(opts.size==0)opts.size=klogctl(SYSLOG_ACTION_SIZE_BUFFER,NULL,0);
	if(opts.size<16*1024)opts.size=16*1024;
	if(opts.size>16*1024*1024)opts.size=16*1024*1024;
	if(!(buf=malloc(opts.size)))return re_printf(1,"malloc");
	opts.size=klogctl(
		opts.clear?
			SYSLOG_ACTION_READ_CLEAR:
			SYSLOG_ACTION_READ_ALL,
		buf,
		opts.size
	);
	if(opts.size<=0){
		free(buf);
		return opts.size==0?0:re_err(1,"klogctl");
	}
	if (!opts.raw){
		int last='\n',in=0;
		while(1){
			if(last=='\n'&&buf[in]=='<') {
				while(buf[in++]!='>'&&in<opts.size);
			}else{
				last=buf[in++];
				putchar(last);
			}
			if(in>=opts.size)break;
		}
		if(last!='\n')putchar('\n');
	}else{
		printf("%s",buf);
		if(buf[opts.size-1]!='\n')putchar('\n');
	}
	free(buf);
	return 0;
}
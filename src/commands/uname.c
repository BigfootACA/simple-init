/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdbool.h>
#include<unistd.h>
#include<sys/utsname.h>
#include"getopt.h"
#include"output.h"

#define progname program_invocation_short_name

static struct{
	bool name;
	bool node;
	bool release;
	bool version;
	bool machine;
	bool processor;
	bool platform;
	bool system;
}opts;

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: uname [OPTION]...\n"
		"Print system information, default -s.\n\n"
		"Options:\n"
		"\t-a, --all                print all information expect -p, -i\n"
		"\t-s, --kernel-name        print the kernel name\n"
		"\t-n, --nodename           print the network node hostname\n"
		"\t-r, --kernel-release     print the kernel release\n"
		"\t-v, --kernel-version     print the kernel version\n"
		"\t-m, --machine            print the machine hardware name\n"
		"\t-p, --processor          print the processor type\n"
		"\t-i, --hardware-platform  print the hardware platform\n"
		"\t-o, --operating-system   print the operating system\n"
		"\t-h, --help               display this help and exit\n"
	);
}

int uname_main(int argc,char**argv){
	struct utsname uts;
	static const struct option lo[]={
		{"help",              no_argument,NULL,'h'},
		{"all",               no_argument,NULL,'a'},
		{"kernel-name",       no_argument,NULL,'s'},
		{"nodename",          no_argument,NULL,'n'},
		{"kernel-release",    no_argument,NULL,'r'},
		{"release",           no_argument,NULL,'r'},
		{"kernel-version",    no_argument,NULL,'v'},
		{"machine",           no_argument,NULL,'m'},
		{"processor",         no_argument,NULL,'p'},
		{"hardware-platform", no_argument,NULL,'i'},
		{"operating-system",  no_argument,NULL,'o'},
		{NULL,0,NULL,0}
	};
	int o;
	while((o=b_getlopt(argc,argv,"hsnrvmpioa",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 's':opts.name=true;break;
		case 'n':opts.node=true;break;
		case 'r':opts.release=true;break;
		case 'v':opts.version=true;break;
		case 'm':opts.machine=true;break;
		case 'p':opts.processor=true;break;
		case 'i':opts.platform=true;break;
		case 'o':opts.system=true;break;
		case 'a':
			opts.name=true;
			opts.node=true;
			opts.release=true;
			opts.version=true;
			opts.machine=true;
			opts.system=true;
		break;
		default:return 1;
	}
	if(b_optind<=1)opts.name=true;
	if(argc>b_optind)return re_printf(
		1,"uname: extra operand: %s\n",
		argv[b_optind]
	);
	uname(&uts);
	char*buff[8]={0};
	int i=0;
	if(opts.name)buff[i++]=uts.sysname;
	if(opts.node)buff[i++]=uts.nodename;
	if(opts.release)buff[i++]=uts.release;
	if(opts.version)buff[i++]=uts.version;
	if(opts.machine)buff[i++]=uts.machine;
	if(opts.system)buff[i++]="GNU/Linux";
	if(opts.platform)buff[i++]="unknown";
	if(opts.processor)buff[i++]="unknown";
	for(int z=0;z<i;z++){
		printf("%s",buff[z]);
		if(z+1<i)putchar(' ');
	}
	putchar('\n');
	return 0;
}
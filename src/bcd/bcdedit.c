/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#include<strings.h>
#include"output.h"
#include"confd.h"
#include"bcdstore.h"
#include"getopt.h"

enum ctl_oper{
	OPER_NONE,
	OPER_SHOW,
	OPER_DUMP,
};

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: bcdedit [OPTIONS]...\n"
		"Windows Boot Configuration Data Editor.\n\n"
		"Options:\n"
		"\t-f, --store <STORE>    BCD store path\n"
		"\t-S, --show             Show BCD\n"
		"\t-D, --dump             Dump BCD\n"
		"\t-h, --help             Display this help and exit\n"
	);
}

int bcdedit_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"store",   required_argument, NULL,'f'},
		{"show",    no_argument,       NULL,'S'},
		{"dump",    no_argument,       NULL,'D'},
		{NULL,0,NULL,0}
	};
	int o;
	char*store=NULL;
	enum ctl_oper op=OPER_NONE;
	while((o=b_getlopt(argc,argv,"hf:SD",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'f':
			if(store)goto conflict;
			store=b_optarg;
		break;
		case 'S':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_SHOW;
		break;
		case 'D':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_DUMP;
		break;
		default:return 1;
	}
	if(!store)return re_printf(2,"no bcd store specified\n");
	struct bcd_store*bcd=bcd_store_open(store,0);
	if(!bcd)return re_err(1,"open bcd store failed");
	switch(op){
		case OPER_SHOW:bcd_dump(bcd);break;
		case OPER_DUMP:bcd_dump_all(bcd);break;
		case OPER_NONE:return re_printf(2,"no operation specified\n");
	}
	bcd_store_free(bcd);
	return 0;
	conflict:return re_printf(2,"too many arguments\n");
}
#endif

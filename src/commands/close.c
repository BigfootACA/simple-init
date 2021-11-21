/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<unistd.h>
#include"output.h"
#include"defines.h"

int close_main(int argc,char**argv __attribute__((unused))){
	if(argc!=2)return re_printf(1,"Usage: close <FD>\n");
	char*value=argv[1],*end;
	int l=(int)strtol(value,&end,10);
	if(*end||value==end||errno!=0)
		return re_printf(2,"invalid file descriptor\n");
	if(close(l)!=0)perror(_("close failed"));
	return errno;
}

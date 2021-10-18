/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<unistd.h>
#include"output.h"

int unlink_main(int argc,char**argv){
	if(argc<2)return re_printf(1,"unlink: missing operand\n");
	if(argc>2)return re_printf(1,"unlink: too many arguments\n");
	int r=unlink(argv[1]);
	return r==0?0:re_err(2,"unlink %s",argv[1]);
}
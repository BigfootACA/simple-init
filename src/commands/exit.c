/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_READLINE
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include"defines.h"
#include"str.h"
#include"output.h"
#include"../shell/shell_internal.h"

int exit_main(int argc,char**argv){
	if(argc<1)ERET(EINVAL);
	if(argc>2)return re_printf(1,"exit: too many arguments\n");
	if(argc==2&&argv[1])exit_code=parse_int(argv[1],2);
	if(shell_running)shell_running=false;
	else exit(exit_code);
	return exit_code;
}
#endif

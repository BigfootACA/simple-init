/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>

int echo_main(int argc,char**argv){
	for(int i=1;i<argc;i++){
		printf("%s",argv[i]);
		if(i!=argc-1)putchar(' ');
	}
	putchar('\n');
	return 0;
}

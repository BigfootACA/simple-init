/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdio.h>
#include"defines.h"
#include"version.h"

int version_main(int argc,char**argv __attribute__((unused))){
	puts(argc==1?VERSION_INFO:_("Usage: version"));
	return argc-1;
}
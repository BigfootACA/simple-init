/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include"confd.h"
#include"cmdline.h"
#define TAG "cmdline"

extern boot_config boot_charger;
int cmdline_androidboot(char*k,char*v){
	char*base="runtime.cmdline",*conf;
	if(strncmp(k,"androidboot.",12)!=0)return 0;
	k+=12;
	if(strcmp(k,"mode")==0){
		if(strcmp(v,"charger")==0){
			confd_set_string("boot.current","charger");
			confd_set_save("boot.current",false);
		}
		conf=k;
	}else if(strcmp(k,"usbcontroller")==0)conf="udc";
	else if(strcmp(k,"serialno")==0)conf="serial";
	else if(strcmp(k,"bootdevice")==0)conf="disk";
	else if(strlen(k)>0)conf=k;
	else return 0;
	confd_set_string_base(base,conf,v);
	return 0;
}

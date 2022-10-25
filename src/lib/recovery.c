/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include"recovery.h"
int recovery_out_fd=-1;

void recovery_ui_print(const char*str){
	if(recovery_out_fd<0||!str)return;
	dprintf(recovery_out_fd,"ui_print %s\n",str);
}

void recovery_progress(const float frac,const int sec){
	if(recovery_out_fd<0)return;
	dprintf(recovery_out_fd,"progress %f %d\n",frac,sec);
}

void recovery_set_progress(const float frac){
	if(recovery_out_fd<0)return;
	dprintf(recovery_out_fd,"set_progress %f\n",frac);
}

void recovery_log(const char*str){
	if(recovery_out_fd<0||!str)return;
	dprintf(recovery_out_fd,"log %s\n",str);
}

void recovery_clear_display(){
	if(recovery_out_fd<0)return;
	dprintf(recovery_out_fd,"clear_display\n");
}

void recovery_ui_printf(const char*fmt,...){
	char*str=NULL;
	va_list va;
	va_start(va,fmt);
	vasprintf(&str,fmt,va);
	va_end(va);
	if(!str)return;
	recovery_ui_print(str);
	free(str);
}

void recovery_logf(const char*fmt,...){
	char*str=NULL;
	va_list va;
	va_start(va,fmt);
	vasprintf(&str,fmt,va);
	va_end(va);
	if(!str)return;
	recovery_log(str);
	free(str);
}

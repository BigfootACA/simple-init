/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"defines.h"
#include"str.h"

const char*make_readable_str_buf(char*buf,size_t len,unsigned long long val,unsigned long block_size,unsigned long display){
	int unit=0;
	memset(buf,0,len);
	if(val==0)return strncpy(buf,"0",len-1);
	if(block_size>1)val*=block_size;
	if(display)val+=display/2,val/=display;
	else while((val>=1024))val/=1024,unit++;
	snprintf(buf,len-1,"%llu %s",val,size_units[unit]);
	return buf;
}

const char*make_readable_str(unsigned long long val,unsigned long block_size,unsigned long display){
	static size_t s=1024;
	char*c=malloc(s);
	if(!c)EPRET(ENOMEM);
	return make_readable_str_buf(c,s,val,block_size,display);
}

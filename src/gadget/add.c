/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include"system.h"
#include"logger.h"
#include"gadget.h"
#define TAG "gadget"

int gadget_add_config(gadget*gadget,gadget_cfg*cfg){
	int d=-1;
	if(!(gadget&&cfg&&(cfg->id)>0&&(cfg->id)<127))goto er;
	char cfgs[32]={0};
	snprintf(cfgs,31,"configs/%s.%d",cfg->type,cfg->id);
	mkdirat(gadget->dir_fd,cfgs,0755);
	if((d=openat(gadget->dir_fd,cfgs,O_RDONLY|O_DIRECTORY|O_CLOEXEC))<0)goto er;
	if(cfg->attributes){
		char val[8]={0};
		snprintf(val,7,"0x%02X\n",cfg->attributes);
		if((wr_file(d,"bmAttributes",val))<0)goto er;
	}
	if(cfg->max_power){
		char val[8]={0};
		snprintf(val,7,"%d\n",cfg->max_power);
		if((wr_file(d,"MaxPower",val))<0)goto er;
	}
	if(cfg->strings)for(int idx=0;cfg->strings[idx];idx++){
		gadget_cfgstr*str=cfg->strings[idx];
		if(!(str&&(str->id)>0x000&&(str->id)<0xFFF))goto er;
		int dx;
		char locale[16]={0};
		snprintf(locale,15,"strings/0x%03X",str->id);
		mkdirat(d,locale,0755);
		if((dx=openat(d,locale,O_RDONLY|O_DIRECTORY|O_CLOEXEC))<0)goto er;
		if(str->configuration&&wr_file(dx,"configuration",str->configuration)<0)goto er;
		close(dx);
	}
	close(d);
	tlog_info("add config '%s.%d' to gadget '%s'",cfg->type,cfg->id,gadget->name);
	return 0;
	er:
	if(cfg&&gadget)telog_error("add config '%s.%d' to gadget '%s' failed",cfg->type,cfg->id,gadget->name);
	if(d>=0)close(d);
	return -1;
}

int gadget_add_string(gadget*gadget,gadget_str*str){
	int d;
	if(!str||!gadget||str->id<0x000||str->id>0xFFF)return -1;
	char locale[16]={0};
	snprintf(locale,15,"strings/0x%03X",str->id);
	mkdirat(gadget->dir_fd,locale,0755);
	if((d=openat(gadget->dir_fd,locale,O_RDONLY|O_DIRECTORY|O_CLOEXEC))<0)goto er;
	if(str->product&&wr_file(d,"product",str->product)<0)goto er;
	if(str->manufacturer&&wr_file(d,"manufacturer",str->manufacturer)<0)goto er;
	if(str->serialnumber&&wr_file(d,"serialnumber",str->serialnumber)<0)goto er;
	close(d);
	return 0;
	er:
	close(d);
	return -1;
}
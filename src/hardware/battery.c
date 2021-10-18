/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include"str.h"
#include"array.h"
#include"logger.h"
#include"system.h"
#include"hardware.h"
#include"pathnames.h"
#define TAG "battery"
#define PS _PATH_SYS_CLASS"/power_supply"

static const char*status_text[]={
	[STATUS_UNKNOWN]      = "Unknown",
	[STATUS_CHARGING]     = "Charging",
	[STATUS_DISCHARGING]  = "Discharging",
	[STATUS_NOT_CHARGING] = "Not charging",
	[STATUS_FULL]         = "Full",
};

static const char*type_text[]={
	[TYPE_UNKNOWN]        = "Unknown",
	[TYPE_BATTERY]        = "Battery",
	[TYPE_UPS]            = "UPS",
	[TYPE_MAINS]          = "Mains",
	[TYPE_USB]            = "USB",
	[TYPE_USB_DCP]        = "USB_DCP",
	[TYPE_USB_CDP]        = "USB_CDP",
	[TYPE_USB_ACA]        = "USB_ACA",
	[TYPE_USB_HVDCP]      = "USB_HVDCP",
	[TYPE_USB_HVDCP_3]    = "USB_HVDCP_3",
	[TYPE_USB_TYPE_C]     = "USB_C",
	[TYPE_USB_PD]         = "USB_PD",
	[TYPE_USB_PD_DRP]     = "USB_PD_DRP",
	[TYPE_APPLE_BRICK_ID] = "BrickID",
	[TYPE_WIRELESS]       = "Wireless",
	[TYPE_USB_FLOAT]      = "USB_FLOAT",
	[TYPE_BMS]            = "BMS",
	[TYPE_PARALLEL]       = "Parallel",
	[TYPE_MAIN]           = "Main",
	[TYPE_WIPOWER]        = "Wipower",
	[TYPE_TYPEC]          = "TYPEC",
	[TYPE_UFP]            = "TYPEC_UFP",
	[TYPE_DFP]            = "TYPEC_DFP",
};

enum power_supply_status pwr_chars2status(const char*status){
	if(status)for(size_t s=0;s<ARRLEN(status_text);s++)
		if(strcasecmp(status_text[s],status)==0)return s;
	return STATUS_UNKNOWN;
}

const char*pwr_status2chars(enum power_supply_status status){
	return status_text[status<0||status>=ARRLEN(status_text)?STATUS_UNKNOWN:status];
}

enum power_supply_type pwr_chars2type(const char*type){
	if(type)for(size_t s=0;s<ARRLEN(type_text);s++)
		if(strcasecmp(type_text[s],type)==0)return s;
	return TYPE_UNKNOWN;
}

const char*pwr_type2chars(enum power_supply_type type){
	return type_text[type<0||type>=ARRLEN(type_text)?TYPE_UNKNOWN:type];
}

static int _open_ps(){
	errno=0;
	static int ps=-1;
	if(ps>=0)return ps;
	if((ps=open(PS,O_DIR|O_CLOEXEC))<0){
		if(errno==ENOENT)ERET(ENOSYS);
		telog_error("open sysfs power supply class failed");
	}
	return ps;
}

enum power_supply_type pwr_get_type(int fd){
	char type[64]={0};
	if(fd_read_file(fd,type,63,false,"type")<0)return -1;
	return pwr_chars2type(type);
}

enum power_supply_status pwr_get_status(int fd){
	char status[64]={0};
	if(fd_read_file(fd,status,63,false,"status")<0)return -1;
	return pwr_chars2status(status);
}

int pwr_get_capacity(int fd){
	return fd_read_int(fd,"capacity");
}

bool pwr_is_battery(int fd){
	return pwr_get_type(fd)==TYPE_BATTERY;
}

bool pwr_is_charging(int fd){
	switch(pwr_get_status(fd)){
		case STATUS_CHARGING:
		case STATUS_NOT_CHARGING:
		case STATUS_FULL:return true;
		default:return false;
	}
}

bool pwr_multi_is_charging(int*fds){
	if(!fds||!fds[0])return false;
	bool charge=false;
	for(int i=0;fds[i];i++)
		charge=charge||pwr_is_charging(fds[i]);
	return charge;
}

int pwr_multi_get_capacity(int*fds){
	if(!fds||!fds[0])ERET(EINVAL);
	int count=0,size=0,i;
	for(i=0;fds[i];i++){
		int r=pwr_get_capacity(fds[i]);
		if(r<0)continue;
		count+=r,size++;
	}
	if(size<=0)ERET(ENODATA);
	return count/size;
}

int pwr_scan_device(int fds[],int max,bool battery){
	if(max<=0||!fds)ERET(EINVAL);
	memset(fds,0,max);
	int ps,cur=0;
	if((ps=_open_ps())<0)return -1;
	DIR*d=fdopendir(ps);
	if(!d)return -1;
	seekdir(d,0);
	struct dirent*e;
	while((e=readdir(d))){
		if(e->d_type!=DT_LNK)continue;
		fds[cur]=openat(ps,e->d_name,O_DIR|O_CLOEXEC);
		if(fds[cur]<0){
			terlog_warn(-1,"open power supply device failed");
			fds[cur]=0;
			continue;
		}
		if(battery&&!pwr_is_battery(fds[cur])){
			close(fds[cur]);
			fds[cur]=0;
			continue;
		}
		if(cur++>=max)break;
	}
	free(d);
	return cur;
}

int pwr_open_device(const char*name){
	if(!name)ERET(EINVAL);
	for(int c=0;name[c]!=0;c++)
		if(name[c]=='/'||strncmp(name+c,"..",2)==0)
			ERET(EINVAL);
	int ps=_open_ps();
	if(ps<0)return -1;
	if(!fd_is_link(ps,name))
		return terlog_warn(-1,"%s is not a power supply device",name);
	int r=openat(ps,name,O_DIR|O_CLOEXEC);
	if(r<0)return terlog_warn(-1,"open power supply device failed");
	if(!fd_is_file(r,"uevent")){
		telog_warn("%s is not a valid power supply device",name);
		close(r);
		r=-1;
	}
	return r;
}

void pwr_close_device(int*fds){
	if(!fds)return;
	for(int i=0;(fds[i]>0);i++){
		close(fds[i]);
		fds[i]=-1;
	}
}

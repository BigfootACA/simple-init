/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#define TAG "led"
#include"str.h"
#include"system.h"
#include"hardware.h"
#include"logger.h"
#include"pathnames.h"

bool led_is_led(int fd){
	return fd_is_file(fd,"brightness");
}

int led_get_max_brightness(int fd){
	return fd_read_int(fd,"max_brightness");
}

int led_get_brightness(int fd){
	return fd_read_int(fd,"brightness");
}

int led_set_brightness(int fd,int value){
	return fd_write_int(fd,"brightness",value,true);
}

int led_get_brightness_percent(int fd){
	if(fd<0)ERET(EBADF);
	int max,cur;
	if((max=led_get_max_brightness(fd))<=0)return -1;
	if((cur=led_get_brightness(fd))<0)return -1;
	return (int)(((float)cur)/((float)max)*100);
}

int led_set_brightness_percent(int fd,int percent){
	if(percent<0||percent>100)ERET(EINVAL);
	if(fd<0)ERET(EBADF);
	int max=led_get_max_brightness(fd);
	return max>0?led_set_brightness(fd,max*percent/100):-1;
}

bool led_check_name(const char*name){
	if(name[0]=='.')return false;
	for(int c=0;name[c]!=0;c++)
		if(name[c]=='/'||strncmp(name+c,"..",2)==0)
			return false;
	return true;
}

int led_open_sysfs_class(){
	static int fd=-1;
	if(fd<0)fd=open(_PATH_SYS_CLASS"/leds",O_DIR|O_CLOEXEC);
	if(fd<0)telog_warn("open leds sysfs class failed");
	return fd;
}

int led_find_class(int sysfs,const char*name){
	if(name&&!led_check_name(name))ERET(EINVAL);
	int bn;
	struct dirent*e;
	DIR*d=fdopendir(sysfs);
	if(!d)return -1;
	seekdir(d,0);
	while((e=readdir(d))){
		if(e->d_type!=DT_DIR&&e->d_type!=DT_LNK)continue;
		if(e->d_name[0]=='.')continue;
		if(name&&strcmp(name,e->d_name)!=0)continue;
		if((bn=openat(sysfs,e->d_name,O_DIR|O_CLOEXEC))<0){
			telog_warn("open led %s folder failed",e->d_name);
			continue;
		}
		if(led_is_led(bn))return bn;
		close(bn);

	}
	free(d);
	ERET(ENOENT);
}

int led_find(const char*name){
	int c;
	if((c=led_open_sysfs_class())<0)return -1;
	return led_find_class(c,name);
}

int led_set_brightness_percent_by_name(char*name,int percent){
	if(!name)return -1;
	int dir,r;
	if(!led_check_name(name))return -1;
	if((dir=led_find(name))<0)return -1;
	r=led_set_brightness_percent(dir,percent);
	close(dir);
	return r;
}

int led_parse_arg(const char*arg,char*def){
	if(!arg)return -1;
	char*d=def,*buf=strdup(arg),*p;
	if(!buf)return -1;
	if(!d)d="leds";
	if((p=strchr(buf,':'))){
		*(p++)=0;
		d=buf;
	}else p=buf;
	int sysfs;
	if(strcasecmp(d,"backlight")==0)sysfs=backlight_open_sysfs_class();
	else if(strcasecmp(d,"leds")==0)sysfs=led_open_sysfs_class();
	else{
		tlog_warn("unsupport led class: %s",d);
		free(buf);
		return -1;
	}
	if(sysfs<0){
		tlog_warn("open sysfs class %s failed",d);
		free(buf);
		return -1;
	}
	int r=led_find_class(sysfs,p);
	free(buf);
	return r;
}

int backlight_open_sysfs_class(){
	static int fd=-1;
	if(fd<0)fd=open(_PATH_SYS_CLASS"/backlight",O_DIR|O_CLOEXEC);
	if(fd<0)telog_warn("open backlight sysfs class failed");
	return fd;
}

int backlight_find(const char*name){
	int c;
	if((c=backlight_open_sysfs_class())<0)return -1;
	return led_find_class(c,name);
}

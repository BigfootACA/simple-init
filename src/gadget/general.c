/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<stdio.h>
#include<dirent.h>
#include<string.h>
#include<stdbool.h>
#include<sys/mount.h>
#include"system.h"
#include"logger.h"
#include"gadget.h"
#include"pathnames.h"
#define TAG "gadget"

int gadget_write_info(gadget*gadget){
	if(!gadget)return -1;
	#define GADGET_WRITE(tag,file,len) \
		if(gadget->tag){\
			char val[6+(len)]={0};\
			snprintf(val,5+(len),"0x%0" #len "X\n",gadget->tag);\
			if(wr_file(gadget->dir_fd,file,val)<0)return -1;\
		}
	GADGET_WRITE(vendor,"idVendor",4)
	GADGET_WRITE(product,"idProduct",4)
	GADGET_WRITE(device,"bcdDevice",4)
	GADGET_WRITE(USB,"bcdUSB",4)
	GADGET_WRITE(devClass,"bDeviceClass",2)
	GADGET_WRITE(devSubClass,"bDeviceSubClass",2)
	GADGET_WRITE(devProtocol,"bDeviceProtocol",2)

	if(gadget->strings)for(int i=0;gadget->strings[i];i++)
		if(gadget_add_string(gadget,gadget->strings[i])<0)return -1;

	if(gadget->configs)for(int i=0;gadget->configs[i];i++)
		if(gadget_add_config(gadget,gadget->configs[i])<0)return -1;

	return 0;
}

static ssize_t find_configfs(char*buff){
	struct mount_item**c=read_proc_mounts(),*s;
	if(!c)return -1;
	size_t l=0;
	ssize_t r=-1;
	while((s=c[l++]))if(strcmp(s->type,"configfs")==0){
		r=strlen(strcpy(buff,s->target));
		break;
	}
	free_mounts(c);
	return r;
}

int open_usb_gadget(){
	int o;
	char c[PATH_MAX-32]={0},p[PATH_MAX]={0};
	if(find_configfs(c)<0){
		strcpy(c,_PATH_SYS_KERNEL"/config");
		if(mount("configfs",c,"configfs",0,NULL)<0)
			return terlog_error(-1,"failed to mount configfs");
	}
	insmod("libcomposite",false);
	snprintf(p,PATH_MAX,"%s/usb_gadget",c);
	if((o=open(p,O_DIR|O_CLOEXEC))<0)
		return terlog_error(-1,"failed to open gadget configfs");
	return o;
}

char*gadget_find_udc(){
	insmod("udc-core",false);
	DIR*d=opendir(_PATH_SYS_CLASS"/udc");
	if(!d){
		telog_error("cannot open UDC class");
		return NULL;
	}
	char*udc=NULL;
	struct dirent*r=NULL;
	while((r=readdir(d)))
		if(!is_virt_dir(r)&&r->d_type==DT_LNK)udc=r->d_name;
	if(!udc){
		tlog_error("cannot find usable UDC.");
		return NULL;
	}
	return udc;
}

int gadget_write_udc(int fd,const char*val){
	return val?wr_file(fd,"UDC",val):-1;
}

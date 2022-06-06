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

int gadget_add_function_cfg(gadget*gadget,gadget_cfg*config,gadget_func*func){
	int d;
	if(!gadget||!config||!func||!func->function||!func->name)return -1;

	// try to load usb function modules
	char module[32]={0};
	snprintf(module,31,"usbfunc:%s",func->function);
	insmod(module,false);

	char cfg_name[32]={0};
	snprintf(cfg_name,31,"%s.%d",config->type,config->id);

	char func_name[64]={0};
	snprintf(func_name,63,"%s.%s",func->function,func->name);

	char function[256]={0};
	snprintf(function,255,"functions/%s",func_name);
	mkdirat(gadget->dir_fd,function,0755);

	char cfg[512]={0};
	snprintf(cfg,511,"configs/%s/%s",cfg_name,func_name);

	if((d=openat(gadget->dir_fd,function,O_DIR|O_CLOEXEC))<0)goto er;

	char path[64]={0};
	get_fd_path(gadget->dir_fd,path,63);

	char mod[512]={0};
	snprintf(mod,511,"%s/%s",path,function);
	if((symlinkat(mod,gadget->dir_fd,cfg))<0)goto er;

	keyval*k;
	if(func->values)for(int i=0;(k=func->values[i]);i++)
		if(k->key&&k->value&&wr_file(d,k->key,k->value)<0)goto er;
	close(d);
	tlog_info("add function '%s' to config '%s'",func_name,cfg_name);
	return 0;
	er:
	telog_error("add function '%s' to config '%s' failed",func_name,cfg_name);
	if(d>=0)close(d);
	return -1;
}

int gadget_add_function(gadget*gadget,gadget_func*func){
	if(!gadget||!func||!gadget->configs)return -1;
	gadget_cfg*cfg=gadget->configs[0];
	if(!cfg)return terlog_error(-1,"no available config found in gadget '%s'",gadget->name);
	return gadget_add_function_cfg(gadget,cfg,func);
}

int gadget_add_function_var(gadget*gadget,char*function,char*name,keyval**values){
	return gadget_add_function(
		gadget,
		&(gadget_func){
			function,
			name,
			values
		}
	);
}

int gadget_add_function_var_cfg(gadget*gadget,char*function,char*name,gadget_cfg*config,keyval**values){
	return gadget_add_function_cfg(
		gadget,
		config,
		&(gadget_func){
			function,
			name,
			values
		}
	);
}

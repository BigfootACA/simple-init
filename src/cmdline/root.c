/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include"defines.h"
#include"logger.h"
#include"keyval.h"
#include"cmdline.h"
#include"boot.h"
#define TAG "cmdline"

extern boot_config boot_switchroot;

static int _add_item(char*key,char*value){
	if(!boot_options.config)boot_options.config=&boot_switchroot;
	keyval**kv=boot_switchroot.data;
	for(int i=0;i<64;i++){
		if(!kv[i])ERET((kv[i]=kv_new_set_dup(key,value))?0:ENOMEM);
		else if(strcmp(key,kv[i]->key)==0){
			if(kv[i]->value)free(kv[i]->value);
			ERET((kv[i]->value=strdup(value))?0:ENOMEM);
		}
	}
	ERET(ENOSPC);
}

static int _xadd_item(char*k,char*key,char*value){
	return _add_item(key,value)<0?terlog_warn(-1,"set value %s failed",k):0;
}

int cmdline_root(char*k,char*v){
	if(v)tlog_debug("root block set to %s",v);
	return _xadd_item(k,"path",v);
}

int cmdline_rootwait(char*k,char*v){
	return _xadd_item(k,"wait",v?v:"0");
}

int cmdline_rootflags(char*k,char*v){
	tlog_debug("root block flags set to %s",v);
	return _xadd_item(k,"flags",v);
}

int cmdline_rootfstype(char*k,char*v){
	tlog_debug("root block type set to %s",v);
	return _xadd_item(k,"type",v);
}

int cmdline_loop(char*k,char*v){
	if(v)tlog_debug("root image file set to %s",v);
	return _xadd_item(k,"loop",v);
}

int cmdline_loopflags(char*k,char*v){
	tlog_debug("root image file flags set to %s",v);
	return _xadd_item(k,"loop_flags",v);
}

int cmdline_loopfstype(char*k,char*v){
	tlog_debug("root image file type set to %s",v);
	return _xadd_item(k,"loop_fstype",v);
}

int cmdline_loopsec(char*k,char*v){
	tlog_debug("root image file sector size set to %s",v);
	return _xadd_item(k,"loop_sector",v);
}

int cmdline_loopoff(char*k,char*v){
	tlog_debug("root image file offset set to %s",v);
	return _xadd_item(k,"loop_offset",v);
}

int cmdline_looppart(char*k,char*v){
	tlog_debug("root image file partition set to %s",v);
	return _xadd_item(k,"loop_partno",v);
}

int cmdline_rw(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-write");
	return _xadd_item(k,"rw","1");
}

int cmdline_ro(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-only");
	return _xadd_item(k,"rw","0");
}

int cmdline_init(char*k,char*v){
	if(!v[0]||v[0]!='/')return trlog_error(ENUM(EINVAL),"invalid init %s",v);
	tlog_debug("init set to %s",v);
	return _xadd_item(k,"init",v);
}

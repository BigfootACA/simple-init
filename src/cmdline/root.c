/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdbool.h>
#include"defines.h"
#include"logger.h"
#include"confd.h"
#include"boot.h"
#include"str.h"
#define TAG "cmdline"

extern boot_config boot_switchroot;

static int _add_item(char*key,char*value,bool num){
	static const char*k="boot.current";
	static const char*b="boot.configs.switchroot";
	if(confd_get_type(k)!=TYPE_STRING)confd_set_string(k,"switchroot");
	confd_set_save(k,false);
	confd_set_boolean_base(b,"enabled",true);
	if(!num)confd_set_string_dict(b,"extra",key,value);
	else confd_set_integer_dict(b,"extra",key,parse_int(value,1));
	return 0;
}

static int _xadd_item(char*k,char*key,char*value,bool num){
	return _add_item(key,value,num)<0?terlog_warn(-1,"set value %s failed",k):0;
}

int cmdline_root(char*k,char*v){
	if(v)tlog_debug("root block set to %s",v);
	return _xadd_item(k,"path",v,false);
}

int cmdline_rootwait(char*k,char*v){
	return _xadd_item(k,"wait",v?v:"0",true);
}

int cmdline_rootflags(char*k,char*v){
	tlog_debug("root block flags set to %s",v);
	return _xadd_item(k,"flags",v,false);
}

int cmdline_rootfstype(char*k,char*v){
	tlog_debug("root block type set to %s",v);
	return _xadd_item(k,"type",v,false);
}

int cmdline_loop(char*k,char*v){
	if(v)tlog_debug("root image file set to %s",v);
	return _xadd_item(k,"loop",v,false);
}

int cmdline_loopflags(char*k,char*v){
	tlog_debug("root image file flags set to %s",v);
	return _xadd_item(k,"loop_flags",v,false);
}

int cmdline_loopfstype(char*k,char*v){
	tlog_debug("root image file type set to %s",v);
	return _xadd_item(k,"loop_fstype",v,false);
}

int cmdline_loopsec(char*k,char*v){
	tlog_debug("root image file sector size set to %s",v);
	return _xadd_item(k,"loop_sector",v,true);
}

int cmdline_loopoff(char*k,char*v){
	tlog_debug("root image file offset set to %s",v);
	return _xadd_item(k,"loop_offset",v,true);
}

int cmdline_looppart(char*k,char*v){
	tlog_debug("root image file partition set to %s",v);
	return _xadd_item(k,"loop_partno",v,true);
}

int cmdline_datablk(char*k,char*v){
	tlog_debug("overlay data block set to %s",v);
	return _xadd_item(k,"data",v,false);
}

int cmdline_datasize(char*k,char*v){
	tlog_debug("overlay work tmpfs size set to %s",v);
	return _xadd_item(k,"data_size",v,false);
}

int cmdline_dataflags(char*k,char*v){
	tlog_debug("overlay block flags set to %s",v);
	return _xadd_item(k,"data_flags",v,false);
}

int cmdline_datafstype(char*k,char*v){
	tlog_debug("overlay block fstype set to %s",v);
	return _xadd_item(k,"data_fstype",v,false);
}

int cmdline_dataname(char*k,char*v){
	tlog_debug("overlay name set to %s",v);
	return _xadd_item(k,"overlay_name",v,false);
}

int cmdline_dataprefix(char*k,char*v){
	tlog_debug("overlay data prefix set to %s",v);
	return _xadd_item(k,"data_prefix",v,false);
}

int cmdline_rw(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-write");
	return _xadd_item(k,"rw","1",true);
}

int cmdline_ro(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-only");
	return _xadd_item(k,"rw","0",true);
}

int cmdline_init(char*k,char*v){
	if(!v[0]||v[0]!='/')return trlog_error(ENUM(EINVAL),"invalid init %s",v);
	tlog_debug("init set to %s",v);
	return _xadd_item(k,"init",v,false);
}

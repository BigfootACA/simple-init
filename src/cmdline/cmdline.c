/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include"logger.h"
#include"keyval.h"
#include"cmdline.h"
#include"param.h"
#define TAG "cmdline"

static struct cmdline_option*find_option(char*name){
	if(!name)return NULL;
	struct cmdline_option*co;
	for(int i=0;(co=cmdline_options[i]);i++){
		if(!co->name||!co->handler)continue;
		if(strcmp(name,co->name)==0)return co;
	}
	return NULL;
}

void parse_params(keyval**params){
	if(!params)return;
	size_t s;
	KVARR_FOREACH(params,t,i){
		struct cmdline_option*co=find_option(t->key);
		if(!co||!t->key)continue;
		s=t->value?strlen(t->value):0;
		if(co->type==NO_VALUE&&s>0){
			tlog_warn("param %s should not have an argument.",t->key);
			continue;
		}else if(co->type==REQUIRED_VALUE&&s<=0){
			tlog_warn("param %s need an argument.",t->key);
			continue;
		}
		if(boot_options.end&&!co->always)continue;
		co->handler(t->key,t->value);
	}
}

int parse_cmdline(int fd){
	keyval**kvs=read_params(fd);
	if(!kvs)return trlog_error(-1,"failed to parse kernel cmdline.");
	parse_params(kvs);
	tlog_info("load kernel cmdline done");
	return 0;
}

int load_cmdline(){
	tlog_debug("loading kernel cmdline");
	int fd,r;
	if((fd=open(_PATH_PROC_CMDLINE,O_RDONLY))<0)
		return telog_error("open "_PATH_PROC_CMDLINE);
	r=parse_cmdline(fd);
	close(fd);
	return r;
}

int cmdline_end(char*k __attribute__((unused)),char*v __attribute__((unused))){
	boot_options.end=true;
	return 0;
}

int cmdline_verbose(char*k __attribute__((unused)),char*v __attribute__((unused))){
	boot_options.verbose=true;
	return 0;
}

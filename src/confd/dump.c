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
#include"str.h"
#include"list.h"
#include"logger.h"
#include"system.h"
#include"confd_internal.h"
#define TAG "confd"

static const char*conf_summary(struct conf*key){
	static char buf[256];
	memset(buf,0,sizeof(buf));
	snprintf(
		buf,sizeof(buf)-1,
		"%c/%d:%d/%04o",
		key->save?'S':'.',
		key->user,key->group,
		key->mode
	);
	return buf;
}

static int dump(enum log_level l,struct conf*key,int depth){
	size_t cnt=16384,len;
	char*buf,x[256],*p;
	if(depth<0||!(buf=malloc(cnt)))return -1;
	memset(buf,0,cnt);
	snprintf(buf,cnt-1,"%-16s ",conf_summary(key));
	for(int i=0;i<depth;i++)strlcat(buf,"  ",cnt-1);
	if(key->name[0])strlcat(buf,key->name,cnt-1);
	else strlcat(buf,"[ROOT]",cnt-1);
	strlcat(buf," ",cnt-1);
	if(key->type==TYPE_KEY){
		strlcat(buf,"(key)",cnt-1);
		logger_print(l,TAG,buf);
		list*p=list_first(key->keys);
		if(p)do{dump(l,LIST_DATA(p,struct conf*),depth+1);}
		while((p=p->next));
		free(buf);
		return 0;
	}
	strlcat(buf," = ",cnt-1);
	len=strlen(buf);
	switch(key->type){
		case TYPE_STRING:
			if(!(p=VALUE_STRING(key))){
				strlcat(buf,"(null string)",cnt-1);
				break;
			}
			memset(x,0,sizeof(x));
			strncpy(x,p,sizeof(x)-1);
			snprintf(
				buf+len,cnt-len-1,
				"\"%s\"%s %zu bytes (string)",
				x,(strlen(p)>sizeof(x)-1?"...":""),
				strlen(p)
			);
		break;
		case TYPE_INTEGER:
			snprintf(
				buf+len,cnt-len-1,
				"%lld (integer)",
				(long long int)VALUE_INTEGER(key)
			);
		break;
		case TYPE_BOOLEAN:
			snprintf(
				buf+len,cnt-len-1,
				"%s (boolean)",
				BOOL2STR(VALUE_BOOLEAN(key))
			);
		break;
		default:strlcat(buf,"(unknown)",cnt-1);break;
	}
	logger_print(l,TAG,buf);
	free(buf);
	return 0;
}

int conf_dump_store(enum log_level level){
	int r=0;
	char buf[64];
	struct conf*c=conf_get_store();
	logger_print(level,TAG,"dump configuration store:");
	if(dump(level,c,0)!=0)r=-1;
	size_t size=conf_calc_size(c);
	logger_printf(
		level,TAG,
		"used memory size: %zu bytes (%s)",size,
		make_readable_str_buf(buf,sizeof(buf),size,1,0)
	);
	return r;
}

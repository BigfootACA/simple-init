/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include"str.h"
#include"list.h"
#include"logger.h"
#include"confd_internal.h"
#define TAG "config"

static char*get_buffer(size_t size){
	static size_t buf_size=0;
	static char*buf=NULL;
	if(size==0){
		free(buf);
		buf=NULL,buf_size=0;
		return NULL;
	}
	if(size>buf_size||!buf){
		if(buf)free(buf);
		buf_size=size,buf=malloc(size);
		if(buf)memset(buf,0,size);
	}
	return buf;
}

static int print_conf(struct conf_file_hand*hand,struct conf*key,const char*name){
	size_t size=0;
	char path[PATH_MAX],*buf,*str;
	if(!key->save)return 0;
	memset(path,0,sizeof(path));
	if(key->name[0]){
		if(!name[0])strcpy(path,key->name);
		else snprintf(path,PATH_MAX-1,"%s.%s",name,key->name);
	}
	if(key->type==TYPE_KEY){
		list*p=list_first(key->keys);
		if(!p)return 0;
		do{print_conf(hand,LIST_DATA(p,struct conf*),path);}while((p=p->next));
	}else if(key->include)return 0;
	else switch(key->type){
		case TYPE_STRING:
			if(!(str=str_escape(VALUE_STRING(key))))break;
			size=strlen(str)+strlen(path)+10;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = \"%s\"\n",path,str);
				hand->write(hand,buf,0);
			}
			free(str);
		break;
		case TYPE_INTEGER:
			size=strlen(path)+50;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = %lld\n",path,(long long int)VALUE_INTEGER(key));
				hand->write(hand,buf,0);
			}
		break;
		case TYPE_BOOLEAN:
			size=strlen(path)+15;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = %s\n",path,BOOL2STR(VALUE_BOOLEAN(key)));
				hand->write(hand,buf,0);
			}
		break;
		default:;
	}
	return 0;
}

static int conf_save(struct conf_file_hand*hand){
	struct conf*store=conf_get_store();
	if(!store)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	hand->write(hand,"# -*- coding: utf-8 -*-\n",0);
	hand->write(hand,"##\n## Simple Init Configuration Store For UEFI\n##\n\n",0);
	#else
	hand->write(hand,"#!/usr/bin/confctl -L\n",0);
	hand->write(hand,"# -*- coding: utf-8 -*-\n",0);
	hand->write(hand,"##\n## Simple Init Configuration Store for Linux\n##\n\n",0);
	#endif
	print_conf(hand,store,"");
	hand->write(hand,"\n# vim: ts=8 sw=8\n",0);
	get_buffer(0);
	return 0;
}

static void line_set_string(struct conf_file_hand*hand,char*key,char*value,size_t len){
	value[len-1]=0,value++;
	char*old=conf_get_string(key,NULL,0,0);
	char*val=str_unescape(value);
	if(!val)return;
	if(old)free(old);
	conf_set_string_inc(key,val,0,0,hand->include);
}

static void conf_parse_line(struct conf_file_hand*hand,int*err,const char*name,size_t n,char*data){
	list*f;
	size_t ks,vs,ds;
	static list*path=NULL;
	char*tk,*value,key[PATH_MAX],*p;
	if(!data){
		if(path){
			tlog_warn("%s: unexpected file end",name);
			list_free_all_def(path);
			path=NULL,(*err)++;
		}
		return;
	}
	if(data[0]=='!'){
		data++;
		if(strncmp(data,"include ",8)==0){
			data+=8;
			if(conf_include_file_depth(
				hand->file,data,hand->depth+1
			)<0)telog_warn("include \"%s\" failed",data);
		}else goto inv_key;
		return;
	}
	if(!(p=strchr(data,'='))){
		if((p=strchr(data,'{'))){
			if(*(p+1))goto inv_key;
			*p=0;
			trim(data);
			if(!*data||!check_valid(data,CONF_KEY_CHARS))goto inv_key;
			list_obj_add_new_strdup(&path,data);
		}else if(strcmp(data,"}")==0){
			if(!path)goto inv_key;
			list_obj_del(&path,list_last(path),list_default_free);
		}else{
			if(!check_valid(data,CONF_KEY_CHARS))goto inv_key;
			if((ds=strlen(data))==0)goto inv_key;
			if(strncmp(data,"runtime.",MIN(7,ds))==0)goto runtime;
			conf_del(data,0,0);
		}
		return;
	}
	*p=0,tk=data,value=p+1;
	trim(tk);
	trim(value);
	memset(key,0,sizeof(key));
	if((f=list_first(path)))do{
		LIST_DATA_DECLARE(k,f,char*);
		if(strlen(k)+strlen(key)+1>=sizeof(key))goto inv_key;
		if(key[0])strcat(key,".");
		strcat(key,k);
	}while((f=f->next));
	if(strlen(tk)+strlen(key)+1>=sizeof(key))goto inv_key;
	if(key[0])strcat(key,".");
	strcat(key,tk);
	if((ks=strlen(key))==0)goto inv_key;
	if((vs=strlen(value))==0)goto inv_val;
	if(!check_valid(key,CONF_KEY_CHARS))goto inv_key;
	if(strncmp(key,"runtime.",MIN(7,ks))==0)goto runtime;
	if(value[0]=='\''){
		if(vs<2||value[vs-1]!='\'')goto inv_val;
		line_set_string(hand,key,value,vs);
	}else if(value[0]=='"'){
		if(vs<2||value[vs-1]!='"')goto inv_val;
		line_set_string(hand,key,value,vs);
	}else{
		if(strcmp(value,"0")==0){
			conf_set_integer_inc(key,0,0,0,hand->include);
			return;
		}
		errno=0;
		int64_t i=strtol(value,&p,0);
		if(errno!=0||p==value){
			if(string_is_true(value))
				conf_set_boolean_inc(key,true,0,0,hand->include);
			else if(string_is_false(value))
				conf_set_boolean_inc(key,false,0,0,hand->include);
			else goto inv_val;
		}else conf_set_integer_inc(key,i,0,0,hand->include);
	}
	return;
	runtime:
	tlog_debug("%s: skip runtime config in line %zu",name,n);
	return;
	inv_key:
	tlog_warn("%s: invalid key in line %zu",name,n);
	(*err)++;
	return;
	inv_val:
	tlog_warn("%s: invalid value in line %zu",name,n);
	(*err)++;
}

static void conf_parse_bare_line(struct conf_file_hand*hand,int*err,const char*name,size_t n,char*data){
	if(!data)return;
	trim(data);

	// empty line
	if(strlen(data)<=0)return;

	// comment line
	if(*data=='#'||strncmp(data,"//",2)==0)return;

	conf_parse_line(hand,err,name,n,data);
}

static int conf_load(struct conf_file_hand*hand){
	char*buff=malloc(hand->len);
	if(!buff)ERET(ENOMEM);
	int err=0;
	char last=0,cur=0;
	size_t dx=0,bx=0,n=0;
	memset(buff,0,hand->len);
	const char*name=hand->path;
	for(size_t x=strlen(name)-1;x>0;x--)if(name[x]=='/'){
		name+=x+1;
		break;
	}
	while(dx<hand->len&&bx<hand->len){
		cur=hand->buff[dx];
		switch(cur){
			case '\n':case '\r':
				if(last!='\r'||cur!='\n')n++;
				if(bx==0)break;
				conf_parse_bare_line(hand,&err,name,n,buff);
				if(err>16){
					tlog_warn("too many errors, stop parse config");
					free(buff);
					ERET(EINVAL);
				}
				bx=0;
				memset(buff,0,hand->len);
			break;
			default:buff[bx++]=cur;
		}
		last=cur,dx++;
	}
	conf_parse_line(hand,&err,name,0,NULL);
	free(buff);
	return 0;
}

struct conf_file_hand conf_hand_conf={
	.ext=(char*[]){"conf","cfg","txt",NULL},
	.load=conf_load,
	.save=conf_save,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_JSONC
#define _GNU_SOURCE
#include<string.h>
#include"str.h"
#include"json.h"
#include"list.h"
#include"logger.h"
#include"confd_internal.h"
#define TAG "config"

static int load_json_object(struct conf_file_hand*hand,struct json_object*obj,const char*path){
	size_t cnt,buf;
	char rpath[PATH_MAX],*p,*str;
	if(!hand||!obj||!path)return -1;
	enum json_type type=json_object_get_type(obj);
	memset(rpath,0,sizeof(rpath));
	if(path[0]){
		strncpy(rpath,path,sizeof(rpath)-1);
		strlcat(rpath,".",sizeof(rpath)-1);
	}
	cnt=strlen(rpath);
	if(sizeof(rpath)-1<=cnt)return -1;
	p=rpath+cnt,buf=sizeof(rpath)-cnt;
	switch(type){
		case json_type_int:
			if(!path[0])goto inv_key;
			conf_set_integer_inc(
				path,
				json_object_get_int64(obj),
				0,0,hand->include
			);
		break;
		case json_type_boolean:
			if(!path[0])goto inv_key;
			conf_set_boolean_inc(
				path,
				json_object_get_boolean(obj),
				0,0,hand->include
			);
		break;
		case json_type_string:
			if(!path[0])goto inv_key;
			if(!(str=strdup(json_object_get_string(obj))))return -1;
			conf_set_string_inc(path,str,0,0,hand->include);
		break;
		case json_type_array:{
			for(size_t i=0;i<json_object_array_length(obj);i++){
				struct json_object*val=json_object_array_get_idx(obj,i);
				if(!val)continue;
				memset(p,0,buf);
				snprintf(p,buf-1,"%zu",i);
				if(path[0])conf_add_key(path,0,0);
				load_json_object(hand,val,rpath);
			}
		}break;
		case json_type_object:{
			json_object_object_foreach(obj,key,val){
				memset(p,0,cnt);
				strncpy(p,key,buf-1);
				if(path[0])conf_add_key(path,0,0);
				load_json_object(hand,val,rpath);
			}
		}break;
		case json_type_null:break;
		default:tlog_warn("unsupported json value type %d in %s",type,hand->path);
	}
	return 0;
	inv_key:return trlog_warn(-1,"invalid key in %s",hand->path);
}

static int conf_load(struct conf_file_hand*hand){
	int r=-1;
	struct json_object*obj=json_tokener_parse(hand->buff);
	if(!obj)EDONE(tlog_warn("parse json %s failed",hand->path));
	if(!json_object_is_type(obj,json_type_object))
		EDONE(tlog_warn("invalid json %s",hand->path));
	r=0;
	load_json_object(hand,obj,"");
	done:
	if(obj)json_object_put(obj);
	return r;
}

static int save_json_object(struct conf_file_hand*hand,struct json_object*obj,struct conf*c){
	list*p;
	struct json_object*val;
	if(!hand||!obj||!c)return -1;
	if(c->include||!c->save)return 0;
	switch(c->type){
		case TYPE_KEY:
			if(!c->name[0])val=obj;
			else if(!(val=json_object_new_object()))break;
			if((p=list_first(c->keys)))do{
				LIST_DATA_DECLARE(l,p,struct conf*);
				save_json_object(hand,val,l);
			}while((p=p->next));
		break;
		case TYPE_STRING:val=VALUE_STRING(c)?
			json_object_new_string(VALUE_STRING(c)):
			json_object_new_null();
		break;
		case TYPE_INTEGER:val=json_object_new_int64(VALUE_INTEGER(c));break;
		case TYPE_BOOLEAN:val=json_object_new_boolean(VALUE_BOOLEAN(c));break;
		default:return -1;
	}
	if(val&&val!=obj)json_object_object_add(obj,c->name,val);
	return 0;
}

static int conf_save(struct conf_file_hand*hand){
	int r=-1;
	char*buff;
	size_t len=0;
	struct json_object*obj=json_object_new_object();
	if(!obj)EDONE(tlog_warn("new json object failed"));
	save_json_object(hand,obj,conf_get_store());
	buff=(char*)json_object_to_json_string_length(
		obj,
		JSON_C_TO_STRING_PRETTY|
		JSON_C_TO_STRING_PRETTY_TAB,
		&len
	);
	if(!buff||len<=0)EDONE(tlog_warn("generate json failed"));
	hand->write(hand,buff,len);
	r=0;
	done:
	if(obj)json_object_put(obj);
	return r;
}

struct conf_file_hand conf_hand_json={
	.ext=(char*[]){"json","jsn",NULL},
	.load=conf_load,
	.save=conf_save,
};
#endif

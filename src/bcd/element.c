/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<string.h>
#include"keyval.h"
#include"bcdstore.h"

bcd_element bcd_get_element_by_node(bcd_object obj,hive_node_h node){
	if(!obj||!bcd_object_get_store(obj)||node<=0)EPRET(EINVAL);
	list*l=list_first(obj->elements);
	if(l)do{
		LIST_DATA_DECLARE(v,l,bcd_element);
		if(v&&v->node==node)return v;
	}while((l=l->next));
	char*end,*key;
	bcd_element ele=malloc(sizeof(struct bcd_element));
	if(!ele)EPRET(ENOMEM);
	memset(ele,0,sizeof(struct bcd_element));
	errno=ENOTSUP;
	ele->bcd=bcd_object_get_store(obj);
	ele->obj=obj;
	ele->node=node;

	if((ele->ele=hivex_node_get_value(
		bcd_element_get_hive(ele),node,"Element"
	))<=0)goto fail;

	if(hivex_value_type(
		bcd_element_get_hive(ele),ele->ele,
		&ele->val_type,&ele->val_len
	)<0)goto fail;

	if(!(key=hivex_node_name(
		bcd_element_get_hive(ele),node
	)))goto fail;
	strncpy(ele->key,key,sizeof(ele->key)-1);
	free(key);

	errno=0,end=NULL;
	ele->et.value=strtoul(ele->key,&end,16);
	if(*end||ele->key==end||errno!=0||ele->et.value==0)goto fail;

	bool found=false;
	int32_t p=bcd_object_get_type(obj);
	for(size_t s=0;BcdElementType[s].name;s++){
		ele->id=&BcdElementType[s];
		if(ele->id->type!=ele->et.value)continue;
		if(!ele->id->objs)found=true;
		else for(size_t x=0;BcdElementType[s].objs[x];x++)
			if(BcdElementType[s].objs[x]==p)
				found=true;
		if(found)break;
	}
	if(!found)ele->id=NULL;
	list_obj_add_new(&obj->elements,ele);
	errno=0;
	return ele;
	fail:
	bcd_element_free(ele);
	return NULL;
}

bcd_element bcd_get_element_by_key(bcd_object obj,const char*key){
	if(!obj||!bcd_object_get_store(obj)||!key)EPRET(EINVAL);
	return bcd_get_element_by_node(obj,hivex_node_get_child(
		bcd_object_get_hive(obj),obj->eles,key
	));
}

bcd_element bcd_get_element_by_id(bcd_object obj,int32_t id){
	if(!obj||!bcd_object_get_store(obj))EPRET(EINVAL);
	char key[16]={0};
	snprintf(key,15,"%x",id);
	return bcd_get_element_by_key(obj,key);
}

bcd_element bcd_get_element_by_name(bcd_object obj,const char*name){
	if(!obj||!name)EPRET(EINVAL);
	for(size_t s=0;BcdElementType[s].name;s++){
		if(strcasecmp(name,BcdElementType[s].name)!=0)continue;
		return bcd_get_element_by_id(obj,BcdElementType[s].type);
	}
	EPRET(ENOENT);
}

bcd_element*bcd_get_all_elements(bcd_object obj){
	size_t cnt,size;
	hive_node_h*cs;
	bcd_element*eles=NULL;
	if(!obj)return NULL;

	if(!(cs=hivex_node_children(
		bcd_object_get_hive(obj),obj->eles
	)))goto fail;

	for(cnt=0;cs[cnt];cnt++);
	size=sizeof(bcd_element)*(cnt+1);
	if(!(eles=malloc(size)))goto fail;
	memset(eles,0,size);

	for(size_t i=0;i<cnt;i++){
		eles[i]=bcd_get_element_by_node(obj,cs[i]);
		if(!eles[i])goto fail;
	}

	list_obj_add_new_notnull(&obj->to_free,eles);
	free(cs);
	return eles;
	fail:
	if(cs)free(cs);
	bcd_elements_free(eles);
	return NULL;
}

const char*bcd_element_get_type_name(bcd_element ele){
	return
		ele&&ele->id&&
		ele->id->name&&
		ele->id->name[0]?
		ele->id->name:NULL;
}

const char*bcd_element_get_display_name(bcd_element ele,char*buf){
	memset(buf,0,256);
	if(bcd_element_get_type_name(ele))
		strncpy(buf,bcd_element_get_type_name(ele),255);
	else if(bcd_element_get_type(ele)!=0)
		snprintf(buf,255,"0x%08x",bcd_element_get_type(ele));
	else if(bcd_element_get_key(ele))
		strncpy(buf,bcd_element_get_key(ele),255);
	else strcpy(buf,"(NULL)");
	return buf;
}

const char*bcd_element_get_key(bcd_element ele){
	return ele&&ele->key[0]?ele->key:NULL;
}

int32_t bcd_element_get_type(bcd_element ele){
	return ele?(ele->id?ele->id->type:ele->et.value):0;
}

bcd_value_type bcd_element_get_format(bcd_element ele){
	return ele?ele->et.format:0;
}

bool bcd_element_is_format(bcd_element ele,bcd_value_type type){
	return bcd_element_get_format(ele)==type;
}

bcd_value_class bcd_element_get_class(bcd_element ele){
	return ele?ele->et.class:0;
}

bool bcd_element_is_class(bcd_element ele,bcd_value_class class){
	return bcd_element_get_class(ele)==class;
}

bcd_object bcd_element_get_object(bcd_element ele){
	return ele?ele->obj:NULL;
}

bcd_store bcd_element_get_store(bcd_element ele){
	return ele?ele->bcd:NULL;
}

hive_h*bcd_element_get_hive(bcd_element ele){
	return ele?bcd_object_get_hive(bcd_element_get_object(ele)):NULL;
}

void bcd_element_free(bcd_element ele){
	if(!ele)return;
	if(bcd_element_get_object(ele)){
		list*l=list_first(bcd_element_get_object(ele)->elements),*n;
		if(l)do{
			n=l->next;
			LIST_DATA_DECLARE(v,l,bcd_element);
			if(!v||v->node!=ele->node)continue;
			list_obj_del(&bcd_element_get_object(ele)->elements,l,NULL);
		}while((l=n));
	}
	free(ele);
}

void bcd_elements_free(bcd_element*eles){
	if(!eles||!*eles||!bcd_element_get_object(*eles))return;
	list*l,*n;
	if((l=list_first(bcd_element_get_object(*eles)->to_free)))do{
		n=l->next;
		LIST_DATA_DECLARE(v,l,bcd_element*);
		if(eles!=v)continue;
		list_obj_del(&bcd_element_get_object(*eles)->to_free,l,NULL);
	}while((l=n));
	free(eles);
}

#endif

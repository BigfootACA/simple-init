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

bcd_object bcd_get_object_by_node(bcd_store bcd,hive_node_h node){
	if(!bcd||node<=0)EPRET(EINVAL);
	list*l=list_first(bcd->objects);
	if(l)do{
		LIST_DATA_DECLARE(v,l,bcd_object);
		if(v&&v->node==node)return v;
	}while((l=l->next));
	hive_type t;
	hive_value_h type;
	char*key=NULL,*alias;
	bcd_object obj=malloc(sizeof(struct bcd_object));
	if(!obj)EPRET(ENOMEM);
	memset(obj,0,sizeof(struct bcd_object));
	errno=ENOTSUP;
	obj->bcd=bcd;
	obj->node=node;

	if((obj->desc=hivex_node_get_child(
		bcd_object_get_hive(obj),node,"Description"
	))<=0)goto fail;

	if((obj->eles=hivex_node_get_child(
		bcd_object_get_hive(obj),node,"Elements"
	))<=0)goto fail;

	if((type=hivex_node_get_value(
		bcd_object_get_hive(obj),obj->desc,"Type"
	))<=0)goto fail;

	if(hivex_value_type(
		bcd_object_get_hive(obj),type,&t,NULL
	)<0)goto fail;

	if(t!=hive_t_REG_DWORD)goto fail;
	bool found=false;
	if((obj->type=hivex_value_dword(bcd_object_get_hive(obj),type))==0)goto fail;
	for(size_t s=0;BcdObjectType[s].name;s++){
		obj->id=&BcdObjectType[s];
		if(obj->id->type!=obj->type)continue;
		found=true;
		break;
	}
	if(!found)obj->id=NULL;

	if(!(key=hivex_node_name(bcd_object_get_hive(obj),node)))goto fail;
	if(key[0]!='{'||strlen(key)!=38||key[37]!='}')goto fail;
	key[37]=0;
	uuid_parse(key+1,obj->uuid);

	if((alias=(char*)bcd_get_name_by_guid(obj->uuid)))
		strncpy(obj->alias,alias,sizeof(obj->alias)-1);

	free(key);
	list_obj_add_new(&bcd->objects,obj);
	errno=0;
	return obj;
	fail:
	if(key)free(key);
	bcd_object_free(obj);
	return NULL;
}

bcd_object bcd_get_object_by_key(bcd_store bcd,const char*key){
	if(!bcd||!key)EPRET(EINVAL);
	return bcd_get_object_by_node(bcd,hivex_node_get_child(
		bcd_store_get_hive(bcd),bcd->objs,key
	));
}

bcd_object bcd_get_object_by_uuid(bcd_store bcd,uuid_t uuid){
	char uuid_str[40]={0},key[64];
	uuid_unparse(uuid,uuid_str);
	snprintf(key,63,"{%s}",uuid_str);
	return bcd_get_object_by_key(bcd,key);
}

bcd_object bcd_get_object_by_name(bcd_store bcd,const char*name){
	if(!bcd||!name)EPRET(EINVAL);
	uuid_t u;
	if(!bcd_get_guid_by_name(name,u))EPRET(ENOENT);
	return bcd_get_object_by_uuid(bcd,u);
}

bcd_object*bcd_get_all_objects(bcd_store bcd){
	size_t cnt,size;
	hive_node_h*cs;
	bcd_object*objs=NULL;
	if(!bcd)return NULL;

	if(!(cs=hivex_node_children(
		bcd_store_get_hive(bcd),bcd->objs
	)))goto fail;

	for(cnt=0;cs[cnt];cnt++);
	size=sizeof(bcd_object)*(cnt+1);
	if(!(objs=malloc(size)))goto fail;
	memset(objs,0,size);

	for(size_t i=0;i<cnt;i++){
		objs[i]=bcd_get_object_by_node(bcd,cs[i]);
		if(!objs[i])goto fail;
	}

	list_obj_add_new_notnull(&bcd->to_free,objs);
	free(cs);
	return objs;
	fail:
	if(cs)free(cs);
	bcd_objects_free(objs);
	return NULL;
}

bcd_object*bcd_get_boot_menu_objects(bcd_store bcd){
	size_t cnt,size;
	bcd_element menu;
	bcd_object*objs=NULL,*buf,mgr;
	uuid_t*us=NULL;
	if(!bcd)return NULL;

	if(
		(mgr=bcd_get_object_by_name(bcd,"BOOTMGR"))&&
		(menu=bcd_get_element_by_name(mgr,"DisplayOrder"))&&
		(us=bcd_element_get_value_uuid_list(menu,&cnt))
	){
		size=sizeof(bcd_object)*(cnt+1);
		if(!(objs=malloc(size)))goto fail;
		memset(objs,0,size);
		for(size_t i=0;i<cnt;i++)
			if(!(objs[i]=bcd_get_object_by_uuid(bcd,us[i])))goto fail;
	}else{
		if(!(buf=bcd_get_all_objects(bcd)))goto fail;
		for(size_t i=0;buf[i];i++)
			if(bcd_object_is_type_name(buf[i],"Boot-OSLoader"))cnt++;
		size=sizeof(bcd_object)*(cnt+1);
		if(!(objs=malloc(size)))goto fail;
		memset(objs,0,size);

		for(size_t i=0,k=0;i<cnt;i++){
			if(!buf[i])goto fail;
			objs[k++]=buf[i];
		}
		bcd_objects_free(buf);
	}

	if(us)free(us);
	list_obj_add_new_notnull(&bcd->to_free,objs);
	return objs;
	fail:
	if(us)free(us);
	bcd_objects_free(objs);
	return NULL;
}

char*bcd_object_get_key(bcd_object obj,char*buf){
	if(!obj)return NULL;
	memset(buf,0,40);
	uuid_unparse(obj->uuid,buf+1);
	buf[0]='{',buf[37]='}';
	return buf;
}

char*bcd_object_get_display_name(bcd_object obj,char*buf){
	if(!obj)return NULL;
	const char*n=bcd_object_get_alias(obj);
	if(!n)return bcd_object_get_key(obj,buf);
	memset(buf,0,40);
	snprintf(buf,39,"{%s}",n);
	return buf;
}

bool bcd_object_get_uuid(bcd_object obj,uuid_t uuid){
	if(!obj||!uuid)return false;
	uuid_copy(uuid,obj->uuid);
	return true;
}

int32_t bcd_object_get_type(bcd_object obj){
	return obj?(obj->id?obj->id->type:obj->type):0;
}

const char*bcd_object_get_type_name(bcd_object obj){
	return obj&&obj->id?obj->id->name:NULL;
}

char*bcd_object_get_description(bcd_object obj){
	bcd_element e=bcd_get_element_by_name(obj,"Description");
	return e?bcd_element_get_value_string(e):NULL;
}

bool bcd_object_is_type(bcd_object obj,int32_t type){
	return
		obj&&type!=0&&
		obj->id&&obj->id->type!=0&&
		type==obj->id->type;
}

bool bcd_object_is_type_name(bcd_object obj,char*type){
	return
		obj&&type&&
		obj->id&&obj->id->name&&
		strcasecmp(type,obj->id->name)==0;
}

const char*bcd_object_get_alias(bcd_object obj){
	return obj&&obj->alias[0]?obj->alias:NULL;
}

bcd_store bcd_object_get_store(bcd_object obj){
	return obj?obj->bcd:NULL;
}

hive_h*bcd_object_get_hive(bcd_object obj){
	return obj?bcd_store_get_hive(bcd_object_get_store(obj)):NULL;
}

void bcd_object_free(bcd_object obj){
	if(!obj)return;
	list*l,*n;
	if(
		bcd_object_get_store(obj)&&
		(l=list_first(bcd_object_get_store(obj)->objects))
	)do{
		n=l->next;
		LIST_DATA_DECLARE(v,l,bcd_object);
		if(!v||v->node!=obj->node)continue;
		list_obj_del(&obj->bcd->objects,l,NULL);
	}while((l=n));
	if((l=list_first(obj->elements)))do{
		n=l->next;
		LIST_DATA_DECLARE(v,l,bcd_element);
		bcd_element_free(v);
	}while((l=n));
	list_free_all_def(obj->to_free);
	free(obj);
}

void bcd_objects_free(bcd_object*objs){
	if(!objs||!*objs||!bcd_object_get_store(*objs))return;
	list*l,*n;
	if((l=list_first(bcd_object_get_store(*objs)->to_free)))do{
		n=l->next;
		LIST_DATA_DECLARE(v,l,bcd_object*);
		if(objs!=v)continue;
		list_obj_del(&bcd_object_get_store(*objs)->to_free,l,NULL);
	}while((l=n));
	free(objs);
}

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include<stdlib.h>
#include"str.h"
#include"assets.h"
#include"render_internal.h"

static bool info_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_event_info*);
	return x&&d&&strcmp(x->event_id,(char*)d)==0;
}

static xml_event_info*list_get_listener(
	list*lst,
	const char*evt_id
){
	list*l;
	if(!lst||!evt_id||!evt_id[0])return NULL;
	if(!(l=list_search_one(lst,info_cmp,(void*)evt_id)))return NULL;
	return LIST_DATA(l,xml_event_info*);
}

static xml_event_info*obj_get_listener(
	xml_render_obj*obj,
	const char*evt_id
){
	if(!obj||!evt_id||!evt_id[0])return NULL;
	return list_get_listener(obj->callbacks,evt_id);
}

static xml_event_info*get_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id
){
	list*l;
	xml_render_obj*obj=NULL;
	if(!render)return NULL;
	if(!evt_id||!evt_id[0])return NULL;
	if(!obj_id||!obj_id[0])return NULL;
	if(!render->initialized)
		return list_get_listener(render->callbacks,evt_id);
	if((l=list_search_one(
		render->objects,
		list_render_obj_cmp,
		(void*)obj_id
	)))obj=LIST_DATA(l,xml_render_obj*);
	return obj_get_listener(obj,evt_id);
}

static int trigger_event(
	xml_event_info*info,
	lv_event_t*event,
	void*data
){
	int r=0;
	xml_render_event e;
	if(!info->callback&&!info->code)return -1;
	if(info->callback&&info->code)return -1;
	if(!info||!info->obj)return -1;
	if(
		info->event!=_LV_EVENT_LAST&&
		info->event!=lv_event_get_code(event)
	)return 0;
	memset(&e,0,sizeof(e));
	e.info=info;
	e.event=event;
	e.data=data;
	#ifdef ENABLE_LUA
	if(info->code){
		lua_State*st=info->obj->render->lua;
		render_event_to_lua(st,&e);
		lua_setglobal(st,"event");
		r=render_code_exec_run(info->code);
		lua_pushnil(st);
		lua_setglobal(st,"event");
		return r;
	}
	#endif
	info->callback(&e);
	return r;
}

static xml_event_info*list_add_listener(
	list**lst,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_event_cb callback,
	xml_render_code*code,
	void*data
){
	xml_event_info*info;
	if(!lst||!evt_id||!evt_id[0])return NULL;
	if(!callback&&!code)return NULL;
	if(callback&&code)return NULL;
	if(list_get_listener(*lst,evt_id))return NULL;
	if(strlen(evt_id)>=sizeof(info->event_id)-1){
		tlog_error("event id too long");
		return NULL;
	}
	if(!(info=malloc(sizeof(xml_event_info))))return NULL;
	memset(info,0,sizeof(xml_event_info));
	strncpy(
		info->event_id,evt_id,
		sizeof(info->event_id)-1
	);
	info->callback=callback;
	info->event=event;
	info->data=data;
	info->code=code;
	return list_obj_add_new(lst,info)==0?info:NULL;
}

int render_obj_add_code_event_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_code*code,
	void*data
){
	if(!obj||!code||!obj->id[0])return -1;
	if(obj_get_listener(obj,evt_id))return -1;
	xml_event_info*info=list_add_listener(
		&obj->callbacks,evt_id,
		event,NULL,code,data
	);
	if(!info)return -1;
	if(obj->obj)lv_obj_add_event_cb(
		obj->obj,lv_render_event_cb,event,info
	);
	info->obj=obj;
	strncpy(
		info->obj_name,obj->id,
		sizeof(info->obj_name)-1
	);
	return 0;
}

int render_obj_add_event_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_event_cb callback,
	void*data
){
	if(!obj||!callback||!obj->id[0])return -1;
	if(obj_get_listener(obj,evt_id))return -1;
	xml_event_info*info=list_add_listener(
		&obj->callbacks,evt_id,
		event,callback,NULL,data
	);
	if(!info)return -1;
	if(obj->obj)lv_obj_add_event_cb(
		obj->obj,lv_render_event_cb,event,info
	);
	info->obj=obj;
	strncpy(
		info->obj_name,obj->id,
		sizeof(info->obj_name)-1
	);
	return 0;
}

int render_add_event_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_event_cb callback,
	void*data
){
	list*l;
	xml_event_info*info;
	if(!render||!callback)return -1;
	if(!obj_id||!obj_id[0])return -1;
	if(!render->initialized){
		if(get_listener(
			render,obj_id,evt_id
		))return -1;
		if((info=list_add_listener(
			&render->callbacks,evt_id,
			event,callback,NULL,data
		)))strncpy(
			info->obj_name,obj_id,
			sizeof(info->obj_name)-1
		);
		return info?0:-1;
	}else{
		if(!(l=list_search_one(
			render->objects,
			list_render_obj_cmp,
			(void*)obj_id
		)))return -1;
		return render_obj_add_event_listener(
			LIST_DATA(l,xml_render_obj*),
			evt_id,event,callback,data
		);
	}
}

int render_obj_del_event_listener(
	xml_render_obj*obj,
	const char*evt_id
){
	list*l;
	if(!obj||!evt_id||!evt_id[0])return -1;
	if(!(l=list_search_one(
		obj->callbacks,
		info_cmp,(char*)evt_id
	)))return -1;
	list_obj_del(
		&obj->callbacks,l,
		list_default_free
	);
	return 0;
}

int render_del_event_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id
){
	list*l;
	xml_render_obj*obj=NULL;
	if(!render)return -1;
	if(!obj_id||!obj_id[0])return -1;
	if(!evt_id||!evt_id[0])return -1;
	if(render->initialized){
		if((l=list_search_one(
			render->objects,
			list_render_obj_cmp,
			(void*)obj_id
		)))obj=LIST_DATA(l,xml_render_obj*);
		return render_obj_del_event_listener(obj,evt_id);
	}
	if(!(l=list_search_one(
		render->callbacks,
		info_cmp,(char*)evt_id
	)))return -1;
	list_obj_del(
		&render->callbacks,
		l,list_default_free
	);
	return 0;
}

int render_obj_trigger_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_t*event,
	void*data
){
	xml_event_info*info;
	if(!obj||!evt_id||!evt_id[0])return -1;
	if(lv_event_get_code(event)>=_LV_EVENT_LAST)return -1;
	if(!(info=obj_get_listener(obj,evt_id)))return -1;
	return trigger_event(info,event,data);
}

int render_trigger_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id,
	lv_event_t*event,
	void*data
){
	if(!render||!render->initialized)return -1;
	if(!obj_id||!obj_id[0])return -1;
	if(!evt_id||!evt_id[0])return -1;
	return render_obj_trigger_listener(
		render_lookup_object(render,obj_id),
		evt_id,event,data
	);
}

int render_obj_trigger_event(
	xml_render_obj*obj,
	lv_event_t*event,
	void*data
){
	list*l;
	if(!obj)return -1;
	if((l=list_first(obj->callbacks)))do{
		LIST_DATA_DECLARE(d,l,xml_event_info*);
		if(d)trigger_event(d,event,data);
	}while((l=l->next));
	return 0;
}

int render_trigger_event(
	xml_render*render,
	const char*obj_id,
	lv_event_t*event,
	void*data
){
	list*l;
	xml_render_obj*obj=NULL;
	if(!render||!render->initialized)return -1;
	if(!obj_id||!obj_id[0])return -1;
	if((l=list_search_one(
		render->objects,
		list_render_obj_cmp,
		(void*)obj_id
	)))obj=LIST_DATA(l,xml_render_obj*);
	return render_obj_trigger_event(obj,event,data);
}

bool render_move_callbacks(xml_render*render){
	list*l,*n,*x;
	bool result=true;
	if(!render||render->initialized)return false;
	if((l=list_first(render->callbacks)))do{
		n=l->next;
		LIST_DATA_DECLARE(d,l,xml_event_info*);
		if(!d||!d->obj_name[0])continue;
		if(!(x=list_search_one(
			render->objects,
			list_render_obj_cmp,
			(char*)d->obj_name
		))){
			tlog_error("cannot resolve id %s",d->obj_name);
			result=false;
			continue;
		}
		d->obj=LIST_DATA(x,xml_render_obj*);
		list_obj_add(&d->obj->callbacks,l);
		list_obj_strip(&render->callbacks,l);
	}while((l=n));
	return result;
}

void lv_render_event_cb(lv_event_t*event){
	xml_event_info*info;
	if(!(info=lv_event_get_user_data(event)))return;
	if(!info->obj||info->obj->obj!=lv_event_get_target(event))return;
	if(lv_event_get_code(event)==LV_EVENT_DELETE)info->obj->obj=NULL;
	else render_obj_trigger_event(info->obj,event,NULL);
}
#endif
#endif

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
#include"defines.h"
#include"gui/lua.h"
#include"render_internal.h"

void render_obj_attr_free(xml_render_obj_attr*o){
	if(!o)return;
	if(o->value)free(o->value);
	memset(o,0,sizeof(xml_render_obj_attr));
	free(o);
}

static int list_render_obj_attr_free(void*r){
	render_obj_attr_free(r);
	return 0;
}

void render_doc_free(xml_render_doc*o){
	if(!o)return;
	if(o->document)mxmlDelete(o->document);
	memset(o,0,sizeof(xml_render_doc));
	free(o);
}

static int list_render_style_free(void*r);
void render_obj_free(xml_render_obj*o){
	if(!o)return;
	if(o->attrs)list_free_all(
		o->attrs,
		list_render_obj_attr_free
	);
	if(o->callbacks)list_free_all(
		o->callbacks,
		list_default_free
	);
	list_free_item(o->styles,list_render_style_free);
	if(o->obj)lv_obj_set_user_data(o->obj,NULL);
	memset(o,0,sizeof(xml_render_obj));
	free(o);
}

void render_style_free(xml_render_style*o){
	if(!o)return;
	lv_style_reset(&o->style);
	memset(o,0,sizeof(xml_render_style));
	free(o);
}

void render_code_free(xml_render_code*o){
	if(!o)return;
	if(o->code)free(o->code);
	memset(o,0,sizeof(xml_render_code));
	free(o);
}

static int list_render_doc_free(void*r){
	render_doc_free(r);
	return 0;
}

static int list_render_obj_free(void*r){
	render_obj_free(r);
	return 0;
}

static int list_render_style_free(void*r){
	render_style_free(r);
	return 0;
}

static int list_render_code_free(void*r){
	render_code_free(r);
	return 0;
}

void render_free(xml_render*r){
	if(!r)return;
	MUTEX_LOCK(r->lock);
	if(r->styles)list_free_all(
		r->styles,
		list_render_style_free
	);
	if(r->objects)list_free_all(
		r->objects,
		list_render_obj_free
	);
	if(r->codes)list_free_all(
		r->codes,
		list_render_code_free
	);
	if(r->docs)list_free_all(
		r->docs,
		list_render_doc_free
	);
	#ifdef ENABLE_LUA
	if(r->lua)lua_close(r->lua);
	#endif
	if(r->content)free(r->content);
	MUTEX_UNLOCK(r->lock);
	MUTEX_DESTROY(r->lock);
	memset(r,0,sizeof(xml_render));
	free(r);
}

xml_render*render_create(void*user_data){
	xml_render*render=malloc(sizeof(xml_render));
	if(!render)goto done;
	memset(render,0,sizeof(xml_render));
	render->data=user_data;
	#ifdef ENABLE_LUA
	if(!(render->lua=xlua_init()))
		EDONE(tlog_error("initialize lua failed"));
	lua_gui_init(render->lua);
	#endif
	MUTEX_INIT(render->lock);
	return render;
	done:
	render_free(render);
	return NULL;
}

xml_render_obj*render_obj_new(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node
){
	xml_render_obj*obj;
	if(!render||!node)return NULL;
	if(!(obj=malloc(sizeof(xml_render_obj))))
		return NULL;
	memset(obj,0,sizeof(xml_render_obj));
	obj->node=node;
	obj->render=render;
	obj->parent=parent;
	obj->translate=true;
	list_obj_add_new(&render->objects,obj);
	return obj;
}

xml_render_style*render_style_new(
	xml_render*render,
	mxml_node_t*node
){
	xml_render_style*style;
	if(!render||!node)return NULL;
	if(!(style=malloc(sizeof(xml_render_style))))
		return NULL;
	memset(style,0,sizeof(xml_render_style));
	style->render=render;
	style->node=node;
	list_obj_add_new(&render->styles,style);
	return style;
}

bool list_render_obj_attr_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_render_obj_attr*);
	return x&&d&&strcmp(x->key,(char*)d)==0;
}

bool list_render_obj_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_render_obj*);
	return x&&d&&strcasecmp(x->id,(char*)d)==0;
}

bool list_render_style_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_render_style*);
	return x&&d&&strcasecmp(x->id,(char*)d)==0;
}

bool list_render_style_class_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_render_style*);
	return x&&d&&strcasecmp(x->class,(char*)d)==0;
}

bool list_render_code_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,xml_render_code*);
	return x&&d&&strcasecmp(x->id,(char*)d)==0;
}
#endif
#endif

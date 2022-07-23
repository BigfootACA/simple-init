/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"str.h"
#include"gui/tools.h"
#include"gui/string.h"
#include"render_internal.h"

static int style_set_type(
	xml_render_obj*obj,
	lv_style_t*style,
	xml_style_prop*prop,
	char*val
){
	errno=0;
	xml_style_set_type hand=NULL;
	if(
		prop->type<=STYLE_NONE||
		prop->type>=STYLE_LAST
	)return -1;
	if(prop->hand)hand=prop->hand;
	if(!hand)hand=xml_style_set_types[prop->type];
	if(!hand)return trlog_warn(
		-1,"unsupported style type"
	);
	return hand(obj,style,prop->prop,prop->max,val);
}

static xml_style_prop*get_prop(char*key){
	for(size_t i=0;xml_style_props[i].valid;i++)
		if(strcasecmp(xml_style_props[i].name,key)==0)
			return &xml_style_props[i];
	tlog_warn("unknown style prop %s",key);
	return NULL;
}

static bool style_selector_cmp(list*f,void*data){
	if(!f||!data)return false;
	LIST_DATA_DECLARE(style,f,xml_render_style*);
	return style&&style->selector==*(lv_style_selector_t*)data;
}

bool xml_attr_apply_style(xml_render_obj_attr*obj){
	list*l;
	bool res=false;
	xml_style_prop*prop;
	xml_render_style*style=NULL;
	lv_part_t part=LV_PART_MAIN;
	lv_state_t state=LV_STATE_DEFAULT;
	char*key,*n,*pa=NULL,*st=NULL,*d=NULL;
	if(!(d=key=strdup(obj->key)))return false;
	if((n=strchr(key,':')))*n=0,st=key,key=n+1;
	if((n=strchr(key,':')))*n=0,pa=key,key=n+1;
	if(pa&&!lv_name_to_part(pa,&part))EDONE(tlog_warn("invalid part %s",pa));
	if(st&&!lv_name_to_state(st,&state))EDONE(tlog_warn("invalid state %s",pa));
	if(!(prop=get_prop(key)))goto done;
	lv_style_selector_t sel=part|state;
	if(!(l=list_search_one(obj->obj->styles,style_selector_cmp,&sel))){
		if(!(style=malloc(sizeof(xml_render_style))))return false;
		memset(style,0,sizeof(xml_render_style));
		list_obj_add_new(&obj->obj->styles,style);
		style->selector=sel,style->render=obj->obj->render;
		lv_style_init(&style->style);
	}else style=LIST_DATA(l,xml_render_style*);
	res=style_set_type(obj->obj,&style->style,prop,obj->value)==0;
	done:
	if(d)free(d);
	return res;
}

bool xml_style_apply_style(
	xml_render_style*style,
	const char*k,
	const char*v
){
	list*l;
	xml_style_prop*prop;
	if(strcasecmp(k,"state")==0){
		lv_state_t state=LV_STATE_DEFAULT;
		if(v&&!lv_name_to_state(v,&state))
			return trlog_warn(false,"invalid state %s",v);
		style->selector|=state;
		return true;
	}else if(strcasecmp(k,"part")==0){
		lv_part_t part=LV_PART_MAIN;
		if(v&&!lv_name_to_part(v,&part))
			return trlog_warn(false,"invalid part %s",v);
		style->selector|=part;
		return true;
	}else if(!(l=list_search_one(
		style->render->objects,
		list_render_obj_cmp,
		(void*)"root"
	)))return false;
	LIST_DATA_DECLARE(obj,l,xml_render_obj*);
	if(!(prop=get_prop((char*)k)))return false;
	return style_set_type(obj,&style->style,prop,(char*)v)==0;
}
#endif
#endif

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
#include"render_internal.h"

static int style_set_type(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_state_t state,
	xml_style_prop*prop,
	char*val
){
	errno=0;
	xml_style_set_type hand=NULL;
	lv_style_property_t p=
		prop->prop|
		(state<<LV_STYLE_STATE_POS);
	if(
		prop->type<=STYLE_NONE||
		prop->type>=STYLE_LAST
	)return -1;
	if(prop->hand)hand=prop->hand;
	if(!hand)hand=xml_style_set_types[prop->type];
	if(!hand)return trlog_warn(
		-1,"unknown supported style type"
	);
	return hand(obj,style,p,prop->max,val);
}

static xml_style_part*get_part(xml_render_obj_attr*obj,char*ns){
	if(!ns)ns="default";
	for(size_t i=0;xml_style_parts[i].valid;i++){
		if(obj->obj->type!=xml_style_parts[i].type)continue;
		if(strcasecmp(xml_style_parts[i].name,ns)!=0)continue;
		return &xml_style_parts[i];
	}
	tlog_warn("unknown style part %s",ns);
	return NULL;
}

static xml_style_prop*get_prop(char*key){
	for(size_t i=0;xml_style_props[i].valid;i++)
		if(strcasecmp(xml_style_props[i].name,key)==0)
			return &xml_style_props[i];
	tlog_warn("unknown style prop %s",key);
	return NULL;
}

static bool get_state(char*st,lv_state_t*state){
	xml_style_state*s=NULL;
	if(!st)return true;
	for(size_t i=0;xml_style_states[i].valid;i++){
		if(strcasecmp(
			xml_style_states[i].name,st
		)!=0)continue;
		s=&xml_style_states[i];
		break;
	}
	if(!s){
		tlog_warn("unknown style state %s",st);
		return false;
	}
	*state=s->state;
	return true;
}

bool xml_attr_apply_style(xml_render_obj_attr*obj){
	bool res=false;
	xml_style_part*part;
	xml_style_prop*prop;
	lv_state_t state=LV_STATE_DEFAULT;
	char*key,*n,*pa=NULL,*st=NULL,*d=NULL;
	if(!(d=key=strdup(obj->key)))return false;
	if((n=strchr(key,':')))*n=0,st=key,key=n+1;
	if((n=strchr(key,':')))*n=0,pa=key,key=n+1;
	if(!(part=get_part(obj,pa)))goto done;
	if(!get_state(st,&state))goto done;
	if(!(prop=get_prop(key)))goto done;
	if(part->part>=sizeof(obj->obj->type))
		EDONE(tlog_error("object part exceeds style list array"));
	res=style_set_type(
		obj->obj,
		&obj->obj->style[part->part],
		state,prop,obj->value
	)==0;
	if(res)obj->obj->has_style[part->part]=true;
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
	bool res=false;
	xml_style_prop*prop;
	lv_state_t state=LV_STATE_DEFAULT;
	char*key,*n,*st=NULL,*d=NULL;
	if(!(l=list_search_one(
		style->render->objects,
		list_render_obj_cmp,
		(void*)"root"
	)))return false;
	if(!(d=key=strdup(k)))return false;
	if((n=strchr(key,':')))*n=0,st=key,key=n+1;
	if(!get_state(st,&state))goto done;
	if(!(prop=get_prop(key)))goto done;
	res=style_set_type(
		LIST_DATA(l,xml_render_obj*),
		&style->style,state,prop,(char*)v
	)==0;
	done:
	if(d)free(d);
	return res;
}
#endif
#endif

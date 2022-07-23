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
#include"gui/tools.h"
#include"gui/string.h"
#include"render_internal.h"

static bool parse_attrs(xml_render_obj*obj){
	bool result=true;
	char*k,*v,*n,*ns=NULL;
	const char*key=NULL,*value=NULL;
	xml_render_obj_attr*attr=NULL;
	if(!obj->node)return false;
	if(obj->attrs)return true;
	for(int s=0;s<mxmlElementGetAttrCount(obj->node);s++){
		ns=NULL,k=NULL,v=NULL,key=NULL;
		value=mxmlElementGetAttrByIndex(obj->node,s,&key);
		if(list_search_one(
			obj->attrs,
			list_render_obj_attr_cmp,
			(char*)key)
		)continue;
		if(strcasecmp(key,"xmlns")==0)continue;
		if(strncasecmp(key,"xmlns:",6)==0)continue;
		if(strncasecmp(key,"xsi:",4)==0)continue;
		if(!obj->parent&&strcasecmp(key,"compatible")==0)continue;
		if(!(attr=malloc(sizeof(xml_render_obj_attr))))return false;
		memset(attr,0,sizeof(xml_render_obj_attr));
		if(!(k=strdup(key))||!(v=strdup(value))){
			free(attr);
			result=false;
		}else{
			if((n=strchr(k,':')))*n++=0,ns=k;
			else n=k;
			strcpy(attr->key,n);
			attr->value=v,attr->obj=obj;
			if(!ns){
				if(strcasecmp(k,"id")==0)
					attr->type=ATTR_ID;
				else tlog_error("invalid key %s",k);
			}else if(strcasecmp(ns,"attr")==0)
				attr->type=ATTR_ATTR;
			else if(strcasecmp(ns,"style")==0)
				attr->type=ATTR_STYLE;
			else if(strcasecmp(ns,"event")==0)
				attr->type=ATTR_EVENT;
			else if(strcasecmp(ns,"ref-style")==0)
				attr->type=ATTR_REF_STYLE;
			else tlog_error("invalid namespace %s",ns);
			list_obj_add_new(&obj->attrs,attr);
		}
		if(k)free(k);
	}
	return result;
}

static bool setup_attr_id(xml_render_obj_attr*d,bool resize){
	if(resize)return true;
	if(!d->obj->parent){
		tlog_error("cannot set id on root object");
		return false;
	}
	if(d->obj->id[0]){
		tlog_error("multiple id set");
		return false;
	}
	if(strlen(d->value)>=sizeof(d->obj->id)-1){
		tlog_error("object id too long");
		return false;
	}
	if(list_search_one(
		d->obj->render->objects,
		list_render_obj_cmp,
		(void*)d->value
	)){
		tlog_error("duplicate object id found");
		return false;
	}
	memset(d->obj->id,0,sizeof(d->obj->id));
	strncpy(d->obj->id,d->value,sizeof(d->obj->id)-1);
	return true;
}

static bool setup_attr_attr(xml_render_obj_attr*d,bool resize){
	xml_attr_handle*hand=NULL;
	for(size_t x=0;(hand=&xml_attr_handles[x])->valid;x++){
		if(strcasecmp(hand->name,d->key)!=0)continue;
		d->hand=hand;
		break;
	}
	if(!d->hand){
		tlog_error("unsupported attribute: %s",d->key);
		return false;
	}
	if(resize&&!d->hand->resize)return true;
	if(d->hand->hand_pre&&!d->hand->hand_pre(d)){
		tlog_error("attribute %s pre init failed",d->key);
		return false;
	}
	return true;
}

static bool setup_attr_event(xml_render_obj_attr*d,bool resize){
	list*l;
	char evt[256];
	lv_event_code_t event=LV_EVENT_ALL;
	if(resize)return true;
	if(!lv_name_to_event_code(d->key,&event)){
		tlog_error("unknown event: %s",d->key);
		return false;
	}
	memset(evt,0,sizeof(evt));
	snprintf(
		evt,sizeof(evt)-1,
		"event-%s-%s",
		d->obj->id,
		lv_event_code_to_name(event)
	);
	if(!(l=list_search_one(
		d->obj->render->codes,
		list_render_code_cmp,
		(void*)d->value
	))){
		tlog_error("unknown code snippet id: %s",d->value);
		return false;
	}
	return render_obj_add_code_event_listener(
		d->obj,evt,event,
		LIST_DATA(l,xml_render_code*),NULL
	)==0;
}

static bool setup_attr_style(xml_render_obj_attr*d,bool resize){
	if(!resize&&!xml_attr_apply_style(d)){
		tlog_error("apply style %s failed",d->key);
		return false;
	}
	return true;
}

static bool setup_attr_ref_style(xml_render_obj_attr*d,bool resize){
	list*l;
	bool found=false;
	lv_part_t part=LV_PART_MAIN;
	if(resize)return true;
	if(!lv_name_to_part(d->key,&part)){
		tlog_error("unknown style part %s",d->key);
		return false;
	}
	if((l=list_search_one(
		d->obj->render->styles,
		list_render_style_cmp,
		(void*)d->value
	))){
		LIST_DATA_DECLARE(style,l,xml_render_style*);
		lv_obj_add_style(d->obj->obj,&style->style,style->selector|part);
		return true;
	}
	if((l=list_first(d->obj->render->styles)))do{
		LIST_DATA_DECLARE(style,l,xml_render_style*);
		if(!list_render_style_class_cmp(l,d->value))continue;
		lv_obj_add_style(d->obj->obj,&style->style,style->selector|part);
		found=true;
	}while((l=l->next));
	if(!found)tlog_error("style %s not found",d->value);
	return found;
}

bool render_init_attributes(xml_render_obj*obj,bool resize){
	list*l;
	bool result=true;
	if(!parse_attrs(obj))return false;
	if((l=list_first(obj->attrs)))do{
		LIST_DATA_DECLARE(d,l,xml_render_obj_attr*);
		if(!d)continue;
		switch(d->type){
			case ATTR_ID:if(!setup_attr_id(d,resize))result=false;break;
			case ATTR_ATTR:if(!setup_attr_attr(d,resize))result=false;break;
			case ATTR_EVENT:if(!setup_attr_event(d,resize))result=false;break;
			case ATTR_STYLE:if(!setup_attr_style(d,resize))result=false;break;
			case ATTR_REF_STYLE:if(!setup_attr_ref_style(d,resize))result=false;break;
			default:tlog_error("unknown attribute type %s",d->key);
		}
	}while((l=l->next));
	if(!result)return false;
	if((l=list_first(obj->attrs)))do{
		LIST_DATA_DECLARE(d,l,xml_render_obj_attr*);
		if(!d||!d->hand||d->type!=ATTR_ATTR)continue;
		if(resize&&!d->hand->resize)continue;
		if(
			d->hand->hand_apply&&
			!d->hand->hand_apply(d)
		)result=false;
	}while((l=l->next));
	if(!result)tlog_error("attribute apply failed");
	if(!obj->id[0]){
		size_t i=0;
		char name[sizeof(obj->id)];
		do{
			memset(name,0,sizeof(name));
			snprintf(
				name,sizeof(name),
				"auto-obj-%zu",i++
			);
		}while(list_search_one(
			obj->render->objects,
			list_render_obj_cmp,
			name
		));
		strncpy(obj->id,name,sizeof(obj->id)-1);
	}
	return result;
}
#endif
#endif

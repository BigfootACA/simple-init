/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"../render_internal.h"

bool xml_attr_handle_pre_square(xml_render_obj_attr*obj){
	lv_coord_t s=0;
	if(!obj->obj->parent)return false;
	if(strcasecmp(obj->value,"none")==0)return true;
	else if(strcasecmp(obj->value,"max")==0)
		s=MAX(obj->obj->width,obj->obj->height);
	else if(strcasecmp(obj->value,"min")==0)
		s=MIN(obj->obj->width,obj->obj->height);
	else if(strcasecmp(obj->value,"width")==0)
		s=obj->obj->width;
	else if(strcasecmp(obj->value,"height")==0)
		s=obj->obj->height;
	else if(strcasecmp(obj->value,"parent-width")==0)
		s=obj->obj->parent->width;
	else if(strcasecmp(obj->value,"parent-height")==0)
		s=obj->obj->parent->height;
	else{
		tlog_warn("unknown value: %s",obj->value);
		return false;
	}
	obj->obj->width=s;
	obj->obj->height=s;
	render_expand_parent(obj->obj);
	render_apply_size(obj->obj);
	obj->obj->auto_width=false;
	obj->obj->auto_height=false;
	return true;
}

bool xml_attr_handle_pre_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->width=render_resolve_coord(
		obj->obj,
		obj->obj->parent->width,
		obj->value
	);
	if(obj->value[strlen(obj->value)-1]!='%')
		obj->obj->auto_width=false;
	render_expand_parent_width(obj->obj);
	return true;
}

bool xml_attr_handle_pre_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->height=render_resolve_coord(
		obj->obj,
		obj->obj->parent->height,
		obj->value
	);
	if(obj->value[strlen(obj->value)-1]!='%')
		obj->obj->auto_height=false;
	render_expand_parent_height(obj->obj);
	return true;
}

bool xml_attr_handle_apply_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_width(obj->obj->obj,obj->obj->width);
	return true;
}

bool xml_attr_handle_apply_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_height(obj->obj->obj,obj->obj->height);
	return true;
}

bool xml_attr_handle_pre_max_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->max_width=render_resolve_coord(
		obj->obj,
		obj->obj->parent->width,
		obj->value
	);
	render_expand_parent_width(obj->obj);
	return true;
}

bool xml_attr_handle_pre_max_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->max_height=render_resolve_coord(
		obj->obj,
		obj->obj->parent->height,
		obj->value
	);
	render_expand_parent_height(obj->obj);
	return true;
}

bool xml_attr_handle_pre_min_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->min_width=render_resolve_coord(
		obj->obj,
		obj->obj->parent->width,
		obj->value
	);
	render_expand_parent_width(obj->obj);
	return true;
}

bool xml_attr_handle_pre_min_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->min_height=render_resolve_coord(
		obj->obj,
		obj->obj->parent->height,
		obj->value
	);
	render_expand_parent_height(obj->obj);
	return true;
}
#endif
#endif

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

bool xml_attr_handle_apply_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_width(obj->obj->obj,c);
	return true;
}

bool xml_attr_handle_apply_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_height(obj->obj->obj,c);
	return true;
}

bool xml_attr_handle_apply_size(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_size(obj->obj->obj,c,c);
	return true;
}

bool xml_attr_handle_apply_max_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_max_width(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_max_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_max_height(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_max_size(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_max_width(obj->obj->obj,c,0);
	lv_obj_set_style_max_height(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_min_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_min_width(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_min_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_min_height(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_min_size(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_style_min_width(obj->obj->obj,c,0);
	lv_obj_set_style_min_height(obj->obj->obj,c,0);
	return true;
}

bool xml_attr_handle_apply_cont_width(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_content_width(obj->obj->obj,c);
	return true;
}

bool xml_attr_handle_apply_cont_height(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_content_height(obj->obj->obj,c);
	return true;
}

bool xml_attr_handle_apply_cont_size(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t c=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_content_width(obj->obj->obj,c);
	lv_obj_set_content_height(obj->obj->obj,c);
	return true;
}

#endif
#endif

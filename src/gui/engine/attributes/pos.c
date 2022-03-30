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

bool xml_attr_handle_pre_align(xml_render_obj_attr*obj){
	bool match=false;
	if(!obj->obj->obj->parent)return false;
	for(size_t i=0;xml_align_specs[i].valid;i++){
		if(strcasecmp(
			xml_align_specs[i].name,
			obj->value
		)!=0)continue;
		obj->fields.align_val=
			xml_align_specs[i].align;
		match=true;
		break;
	}
	if(!match)tlog_warn("invalid align: %s",obj->value);
	return match;
}

bool xml_attr_handle_apply_align(xml_render_obj_attr*obj){
	lv_obj_align(
		obj->obj->cont?
		obj->obj->cont:
		obj->obj->obj,
		NULL,
		obj->fields.align_val,
		0,0
	);
	render_reload_pos(obj->obj);
	return true;
}

bool xml_attr_handle_pre_top(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->y=render_resolve_coord(
		obj->obj,
		obj->obj->parent->height,
		obj->value
	);
	return true;
}

bool xml_attr_handle_pre_bottom(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->y=obj->obj->parent->height-
		render_resolve_coord(
			obj->obj,
			obj->obj->parent->height,
			obj->value
		);
	return true;
}

bool xml_attr_handle_pre_left(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->x=render_resolve_coord(
		obj->obj,
		obj->obj->parent->width,
		obj->value
	);
	return true;
}

bool xml_attr_handle_pre_right(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	obj->obj->x=obj->obj->parent->width-
		render_resolve_coord(
			obj->obj,
			obj->obj->parent->width,
			obj->value
		);
	return true;
}

static inline xml_render_obj*_lookup(
	xml_render_obj*obj,
	const char*name,
	lv_coord_t*x,
	lv_coord_t*y
){
	list*l;
	xml_render_obj*of=NULL;
	char*cx=NULL,*cy=NULL,*d;
	char*n=strdup(name);
	if(!n)return NULL;
	if((d=strchr(n,':'))){
		*d++=0,cx=d;
		if((d=strchr(cx,':')))*d++=0,cy=d;
	}
	if(!(l=list_search_one(
		obj->render->objects,
		list_render_obj_cmp,
		(void*)n
	)))tlog_warn("object %s not found",n);
	else{
		of=LIST_DATA(l,xml_render_obj*);
		if(cx&&x)*x=render_resolve_coord(
			of,lv_obj_get_width(of->obj),cx
		);
		if(cy&&y)*y=render_resolve_coord(
			of,lv_obj_get_height(of->obj),cy
		);
	}
	free(n);
	return of;
}

static bool check_skip(xml_render_obj_attr*obj){
	switch(obj->obj->parent->type){
		case OBJ_HOR_BOX:case OBJ_VER_BOX:break;
		default:return false;
	}
	tlog_warn(
		"attribute %s will skip in this parent object type",
		obj->key
	);
	return true;
}

bool xml_attr_handle_apply_top_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	y+=lv_obj_get_style_pad_inner(obj->obj->parent->obj,LV_OBJ_PART_MAIN);
	y+=lv_obj_get_style_margin_bottom(obj->obj->obj,LV_OBJ_PART_MAIN);
	y+=lv_obj_get_style_margin_top(of->obj,LV_OBJ_PART_MAIN);
	lv_obj_align(obj->obj->obj,of->obj,LV_ALIGN_OUT_TOP_MID,x,y);
	render_reload_pos(obj->obj);
	return true;
}

bool xml_attr_handle_apply_left_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	x+=lv_obj_get_style_pad_inner(obj->obj->parent->obj,LV_OBJ_PART_MAIN);
	x+=lv_obj_get_style_margin_right(obj->obj->obj,LV_OBJ_PART_MAIN);
	x+=lv_obj_get_style_margin_left(of->obj,LV_OBJ_PART_MAIN);
	lv_obj_align(obj->obj->obj,of->obj,LV_ALIGN_OUT_LEFT_MID,x,y);
	render_reload_pos(obj->obj);
	return true;
}

bool xml_attr_handle_apply_bottom_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	y+=lv_obj_get_style_pad_inner(obj->obj->parent->obj,LV_OBJ_PART_MAIN);
	y+=lv_obj_get_style_margin_top(obj->obj->obj,LV_OBJ_PART_MAIN);
	y+=lv_obj_get_style_margin_bottom(of->obj,LV_OBJ_PART_MAIN);
	lv_obj_align(obj->obj->obj,of->obj,LV_ALIGN_OUT_BOTTOM_MID,x,y);
	render_reload_pos(obj->obj);
	return true;
}

bool xml_attr_handle_apply_right_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	x+=lv_obj_get_style_pad_inner(obj->obj->parent->obj,LV_OBJ_PART_MAIN);
	x+=lv_obj_get_style_margin_left(obj->obj->obj,LV_OBJ_PART_MAIN);
	x+=lv_obj_get_style_margin_right(of->obj,LV_OBJ_PART_MAIN);
	lv_obj_align(obj->obj->obj,of->obj,LV_ALIGN_OUT_RIGHT_MID,x,y);
	render_reload_pos(obj->obj);
	return true;
}
#endif
#endif

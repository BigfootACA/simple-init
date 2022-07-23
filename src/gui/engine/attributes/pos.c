/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"gui/tools.h"
#include"gui/string.h"
#include"../render_internal.h"

bool xml_attr_handle_pre_align(xml_render_obj_attr*obj){
	if(!obj->obj->obj->parent)return false;
	bool match=lv_name_to_align(obj->value,&obj->fields.align_val);
	if(!match)tlog_warn("invalid align: %s",obj->value);
	return match;
}

bool xml_attr_handle_apply_align(xml_render_obj_attr*obj){
	lv_obj_align_to(
		obj->obj->cont?
		obj->obj->cont:
		obj->obj->obj,
		NULL,
		obj->fields.align_val,
		0,0
	);
	return true;
}

bool xml_attr_handle_apply_x(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_x(obj->obj->obj,render_resolve_coord(obj->obj,0,obj->value));
	return true;
}

bool xml_attr_handle_apply_y(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_y(obj->obj->obj,render_resolve_coord(obj->obj,0,obj->value));
	return true;
}

bool xml_attr_handle_apply_pos(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t pos=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_set_pos(obj->obj->obj,pos,pos);
	return true;
}

bool xml_attr_handle_pre_top(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_y(obj->obj->obj,render_resolve_coord(obj->obj,0,obj->value));
	return true;
}

bool xml_attr_handle_pre_bottom(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t y=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_update_layout(obj->obj->parent->obj);
	lv_coord_t ph=lv_obj_get_height(obj->obj->parent->obj);
	lv_coord_t ch=lv_obj_get_height(obj->obj->obj);
	lv_obj_set_y(obj->obj->obj,ph-ch-y);
	return true;
}

bool xml_attr_handle_pre_left(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_x(obj->obj->obj,render_resolve_coord(obj->obj,0,obj->value));
	return true;
}

bool xml_attr_handle_pre_right(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_coord_t x=render_resolve_coord(obj->obj,0,obj->value);
	lv_obj_update_layout(obj->obj->parent->obj);
	lv_coord_t pw=lv_obj_get_width(obj->obj->parent->obj);
	lv_coord_t cw=lv_obj_get_width(obj->obj->obj);
	lv_obj_set_y(obj->obj->obj,pw-cw-x);
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
		if(cx&&x)*x=render_resolve_coord(of,0,cx);
		if(cy&&y)*y=render_resolve_coord(of,0,cy);
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
	lv_obj_align_to(obj->obj->obj,of->obj,LV_ALIGN_OUT_TOP_MID,x,y);
	return true;
}

bool xml_attr_handle_apply_left_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	lv_obj_align_to(obj->obj->obj,of->obj,LV_ALIGN_OUT_LEFT_MID,x,y);
	return true;
}

bool xml_attr_handle_apply_bottom_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	lv_obj_align_to(obj->obj->obj,of->obj,LV_ALIGN_OUT_BOTTOM_MID,x,y);
	return true;
}

bool xml_attr_handle_apply_right_of(xml_render_obj_attr*obj){
	lv_coord_t x=0,y=0;
	xml_render_obj*of=_lookup(obj->obj,obj->value,&x,&y);
	if(!of)return false;
	if(check_skip(obj))return true;
	lv_obj_align_to(obj->obj->obj,of->obj,LV_ALIGN_OUT_RIGHT_MID,x,y);
	return true;
}
#endif
#endif

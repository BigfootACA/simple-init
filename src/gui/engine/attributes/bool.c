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
#include"../render_internal.h"

static bool xml_attr_handle_pre_boolean(
	xml_render_obj_attr*obj
){
	if(!obj->obj->parent)return false;
	if(string_is_false(obj->value))obj->fields.bool_val=false;
	else if(string_is_true(obj->value))obj->fields.bool_val=true;
	else{
		tlog_warn("invalid %s boolean: %s",obj->key,obj->value);
		return false;
	}
	return true;
}
#define XDECL_BOOL(_name,_expr) bool xml_attr_handle_pre_##_name(xml_render_obj_attr*obj){\
	bool r=xml_attr_handle_pre_boolean(obj);\
	if(r){_expr;}\
	return r;\
}
#define DECL_BOOL(_name)XDECL_BOOL(_name,)

XDECL_BOOL(translate,obj->obj->translate=obj->fields.bool_val)
XDECL_BOOL(auto_width,obj->obj->auto_width=obj->fields.bool_val)
XDECL_BOOL(auto_height,obj->obj->auto_height=obj->fields.bool_val)
DECL_BOOL(drag_parent)
DECL_BOOL(drag_throw)
DECL_BOOL(bind_input)
DECL_BOOL(clickable)
DECL_BOOL(password)
DECL_BOOL(checked)
DECL_BOOL(enabled)
DECL_BOOL(oneline)
DECL_BOOL(recolor)
DECL_BOOL(visible)
DECL_BOOL(hidden)
DECL_BOOL(drag)
DECL_BOOL(fill)

bool xml_attr_handle_apply_checked(
	xml_render_obj_attr*obj
){
	switch(obj->obj->type){
		case OBJ_CHECKBOX:
			lv_checkbox_set_checked(
				obj->obj->obj,
				obj->fields.bool_val
			);
		break;
		default:
			lv_obj_set_checked(
				obj->obj->obj,
				obj->fields.bool_val
			);
		break;
	}
	return true;
}

bool xml_attr_handle_apply_recolor(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_LABEL)
	lv_label_set_recolor(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_enabled(
	xml_render_obj_attr*obj
){
	lv_obj_set_enabled(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_visible(
	xml_render_obj_attr*obj
){
	lv_obj_set_hidden(
		obj->obj->obj,
		!obj->fields.
	bool_val);
	return true;
}

bool xml_attr_handle_apply_hidden(
	xml_render_obj_attr*obj
){
	lv_obj_set_hidden(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_drag(
	xml_render_obj_attr*obj
){
	lv_obj_set_drag(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_drag_throw(
	xml_render_obj_attr*obj
){
	lv_obj_set_drag_throw(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_drag_parent(
	xml_render_obj_attr*obj
){
	lv_obj_set_drag_parent(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_clickable(
	xml_render_obj_attr*obj
){
	lv_obj_set_click(
		obj->obj->obj,
		obj->fields.bool_val
	);
	obj->obj->clickable=obj->fields.bool_val;
	return true;
}
#endif
#endif

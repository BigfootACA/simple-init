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

static void hand_flag(xml_render_obj_attr*obj,lv_obj_flag_t flag){(obj->fields.bool_val?lv_obj_add_flag:lv_obj_clear_flag)(obj->obj->obj,flag);}
static void hand_state(xml_render_obj_attr*obj,lv_state_t state){(obj->fields.bool_val?lv_obj_add_state:lv_obj_clear_state)(obj->obj->obj,state);}
static void hand_flag_r(xml_render_obj_attr*obj,lv_obj_flag_t flag){(obj->fields.bool_val?lv_obj_clear_flag:lv_obj_add_flag)(obj->obj->obj,flag);}
static void hand_state_r(xml_render_obj_attr*obj,lv_state_t state){(obj->fields.bool_val?lv_obj_clear_state:lv_obj_add_state)(obj->obj->obj,state);}

#define DECL_FLAG(_name,flag) bool xml_attr_handle_apply_##_name(xml_render_obj_attr*obj){hand_flag(obj,flag);return true;}
#define DECL_STATE(_name,state) bool xml_attr_handle_apply_##_name(xml_render_obj_attr*obj){hand_state(obj,state);return true;}
#define DECL_FLAG_R(_name,flag) bool xml_attr_handle_apply_##_name(xml_render_obj_attr*obj){hand_flag_r(obj,flag);return true;}
#define DECL_STATE_R(_name,state) bool xml_attr_handle_apply_##_name(xml_render_obj_attr*obj){hand_state_r(obj,state);return true;}
#define XDECL_BOOL(_name,_expr) bool xml_attr_handle_pre_##_name(xml_render_obj_attr*obj){\
	bool r=xml_attr_handle_pre_boolean(obj);\
	if(r){_expr;}\
	return r;\
}
#define DECL_BOOL(_name)XDECL_BOOL(_name,)

XDECL_BOOL(translate,obj->obj->translate=obj->fields.bool_val)
DECL_BOOL(bind_input)
DECL_BOOL(clickable)
DECL_BOOL(password)
DECL_BOOL(checked)
DECL_BOOL(enabled)
DECL_BOOL(oneline)
DECL_BOOL(recolor)
DECL_BOOL(visible)
DECL_BOOL(hidden)
DECL_BOOL(center)
DECL_BOOL(drag)

DECL_FLAG(hidden,LV_OBJ_FLAG_HIDDEN)
DECL_FLAG_R(visible,LV_OBJ_FLAG_HIDDEN)
DECL_FLAG(clickable,LV_OBJ_FLAG_CLICKABLE)
DECL_FLAG(floating,LV_OBJ_FLAG_FLOATING)
DECL_FLAG(scrollable,LV_OBJ_FLAG_SCROLLABLE)
DECL_FLAG(snappable,LV_OBJ_FLAG_SNAPPABLE)
DECL_STATE(edited,LV_STATE_EDITED)
DECL_STATE(hoverd,LV_STATE_HOVERED)
DECL_STATE(checked,LV_STATE_CHECKED)
DECL_STATE(focused,LV_STATE_FOCUSED)
DECL_STATE(pressed,LV_STATE_PRESSED)
DECL_STATE(scrolled,LV_STATE_SCROLLED)
DECL_STATE(disabled,LV_STATE_DISABLED)
DECL_STATE_R(enabled,LV_STATE_DISABLED)

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

bool xml_attr_handle_apply_center(
	xml_render_obj_attr*obj
){
	if(obj->fields.bool_val)
		lv_obj_center(obj->obj->obj);
	return true;
}

#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"render_internal.h"
#include"gui/sysbar.h"
#include"gui/tools.h"

#define DECL_SIMPLE_OBJ_EXPR(_name,_expr)\
bool xml_attr_object_pre_##_name(xml_render_obj*obj){\
	obj->obj=lv_##_name##_create(obj->parent->obj);\
        if(!obj->obj)return false;\
        _expr\
	return true;\
}
#define DECL_SIMPLE_OBJ(_name)\
	DECL_SIMPLE_OBJ_EXPR(_name,)
#define DECL_SIMPLE_OBJ_NO_AUTO(_name)\
	DECL_SIMPLE_OBJ_EXPR(_name,obj->auto_height=false;obj->auto_width=false;)

#if LV_USE_LABEL
DECL_SIMPLE_OBJ(label)
#endif
#if LV_USE_CHECKBOX
DECL_SIMPLE_OBJ(checkbox)
#endif
#if LV_USE_DROPDOWN
DECL_SIMPLE_OBJ(dropdown)
#endif
#if LV_USE_IMG
DECL_SIMPLE_OBJ(img)
#endif
#if LV_USE_ARC
DECL_SIMPLE_OBJ(arc)
#endif
#if LV_USE_BAR
DECL_SIMPLE_OBJ(bar)
#endif
#if LV_USE_IMGBTN
DECL_SIMPLE_OBJ(imgbtn)
#endif
#if LV_USE_BTNMATRIX
DECL_SIMPLE_OBJ(btnmatrix)
#endif
#if LV_USE_CALENDAR
DECL_SIMPLE_OBJ(calendar)
#endif
#if LV_USE_CANVAS
DECL_SIMPLE_OBJ(canvas)
#endif
#if LV_USE_KEYBOARD
DECL_SIMPLE_OBJ(keyboard)
#endif
#if LV_USE_LED
DECL_SIMPLE_OBJ(led)
#endif
#if LV_USE_LINE
DECL_SIMPLE_OBJ(line)
#endif
#if LV_USE_LIST
DECL_SIMPLE_OBJ(list)
#endif
#if LV_USE_ROLLER
DECL_SIMPLE_OBJ(roller)
#endif
#if LV_USE_SLIDER
DECL_SIMPLE_OBJ(slider)
#endif
#if LV_USE_SPINBOX
DECL_SIMPLE_OBJ(spinbox)
#endif
#if LV_USE_SWITCH
DECL_SIMPLE_OBJ(switch)
#endif
#if LV_USE_CHART
DECL_SIMPLE_OBJ(chart)
#endif

bool xml_attr_object_pre_obj(xml_render_obj*obj){
	obj->obj=lv_obj_create(obj->parent->obj);
	if(!obj->obj)return false;
	switch(obj->type){
		case OBJ_HOR_BOX:lv_obj_set_flex_flow(obj->obj,LV_FLEX_FLOW_ROW);break;
		case OBJ_VER_BOX:lv_obj_set_flex_flow(obj->obj,LV_FLEX_FLOW_COLUMN);break;
		case OBJ_WRAPPER:
			lv_obj_clear_flag(obj->obj,LV_OBJ_FLAG_CLICKABLE);
			lv_obj_clear_flag(obj->obj,LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_style_border_width(obj->obj,0,0);
			lv_obj_set_style_bg_opa(obj->obj,LV_OPA_0,0);
		break;
		default:;
	}
	lv_obj_set_size(obj->obj,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
	return true;
}

#if LV_USE_BTN
bool xml_attr_object_pre_btn(xml_render_obj*obj){
	obj->obj=lv_btn_create(obj->parent->obj);
	if(!obj->obj)return false;
	if(obj->type==OBJ_BTN_ITEM)
		lv_style_set_btn_item(obj->obj);
	return true;
}
#endif

#if LV_USE_SPINNER
bool xml_attr_object_pre_spinner(xml_render_obj*obj){
	obj->obj=lv_spinner_create(obj->parent->obj,0,0);
	return obj->obj!=NULL;
}
#endif

#if LV_USE_TEXTAREA
bool xml_attr_object_pre_text(xml_render_obj*obj){
	obj->obj=lv_textarea_create(obj->parent->obj);
	if(!obj->obj)return false;
	lv_textarea_set_text(obj->obj,"");
	obj->bind_input=true;
	return true;
}

static void input_cb(xml_render_event*e){
	lv_input_cb(render_event_get_event(e));
}

bool xml_attr_object_post_text(xml_render_obj*obj){
	if(obj->render->initialized)return true;
	if(obj->bind_input){
		render_obj_add_event_listener(
			obj,"_text_input_cb",
			LV_EVENT_CLICKED,
			input_cb,NULL
		);
	}
	return true;
}
#endif
#endif
#endif

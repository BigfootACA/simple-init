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

#define DECL_SIMPLE_OBJ_EXPR(_name,_expr)\
bool xml_attr_object_pre_##_name(xml_render_obj*obj){\
	obj->obj=lv_##_name##_create(obj->parent->obj,NULL);\
        if(!obj->obj)return false;\
        _expr\
	return true;\
}
#define DECL_SIMPLE_OBJ(_name)\
	DECL_SIMPLE_OBJ_EXPR(_name,)
#define DECL_SIMPLE_OBJ_NO_AUTO(_name)\
	DECL_SIMPLE_OBJ_EXPR(_name,obj->auto_height=false;obj->auto_width=false;)

#if LV_USE_BTN
DECL_SIMPLE_OBJ(btn)
#endif
#if LV_USE_LABEL
DECL_SIMPLE_OBJ(label)
#endif
#if LV_USE_CHECKBOX
DECL_SIMPLE_OBJ(checkbox)
#endif
#if LV_USE_DROPDOWN
DECL_SIMPLE_OBJ(dropdown)
#endif
#if LV_USE_PAGE
DECL_SIMPLE_OBJ_NO_AUTO(page)
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
#if LV_USE_GAUGE
DECL_SIMPLE_OBJ(gauge)
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
#if LV_USE_SPINNER
DECL_SIMPLE_OBJ(spinner)
#endif
#if LV_USE_CPICKER
DECL_SIMPLE_OBJ(cpicker)
#endif
#if LV_USE_CHART
DECL_SIMPLE_OBJ(chart)
#endif

bool xml_attr_object_pre_obj(xml_render_obj*obj){
	lv_obj_t*parent=obj->parent->obj;
	obj->obj=lv_obj_create(parent,NULL);
	if(!obj->obj)return false;
	lv_obj_set_click(obj->obj,false);
	lv_theme_apply(obj->obj,LV_THEME_SCR);
	if(lv_obj_get_style_bg_opa(parent,LV_OBJ_PART_MAIN)==LV_OPA_COVER){
		lv_obj_set_style_local_bg_color(
			obj->obj,
			LV_OBJ_PART_MAIN,
			LV_STATE_DEFAULT,
			lv_obj_get_style_bg_color(parent,LV_OBJ_PART_MAIN)
		);
		lv_obj_set_style_local_bg_opa(
			obj->obj,
			LV_OBJ_PART_MAIN,
			LV_STATE_DEFAULT,
			LV_OPA_TRANSP
		);
	}
	return true;
}

bool xml_attr_object_post_obj(xml_render_obj*obj){
	list*l;
	xml_render_obj*last=NULL;
	lv_coord_t mt,ml,mb,mr,pi,pt,pb,pl,pr;
	pi=lv_obj_get_style_pad_inner(obj->obj,LV_OBJ_PART_MAIN);
	pt=lv_obj_get_style_pad_top(obj->obj,LV_OBJ_PART_MAIN);
	pb=lv_obj_get_style_pad_bottom(obj->obj,LV_OBJ_PART_MAIN);
	pl=lv_obj_get_style_pad_left(obj->obj,LV_OBJ_PART_MAIN);
	pr=lv_obj_get_style_pad_right(obj->obj,LV_OBJ_PART_MAIN);
	if(obj->type!=OBJ_HOR_BOX&&obj->type!=OBJ_VER_BOX)return true;
	if(obj->auto_width)obj->width=0;
	if(obj->auto_height)obj->height=0;
	if((l=list_first(obj->render->objects)))do{
		LIST_DATA_DECLARE(o,l,xml_render_obj*);
		if(!o||o->parent!=obj)continue;
		mt=lv_obj_get_style_margin_top(o->obj,LV_OBJ_PART_MAIN);
		ml=lv_obj_get_style_margin_left(o->obj,LV_OBJ_PART_MAIN);
		switch(obj->type){
			case OBJ_VER_BOX:o->y=mt+(last?last->y+last->height+mb+pi:pt);break;
			case OBJ_HOR_BOX:o->x=ml+(last?last->x+last->width+mr+pi:pl);break;
			default:continue;
		}
		lv_obj_set_pos(o->obj,o->x,o->y);
		render_expand_parent(o);
		mb=lv_obj_get_style_margin_bottom(o->obj,LV_OBJ_PART_MAIN);
		mr=lv_obj_get_style_margin_right(o->obj,LV_OBJ_PART_MAIN);
		last=o;
	}while((l=l->next));
	if((l=list_first(obj->render->objects)))do{
		LIST_DATA_DECLARE(o,l,xml_render_obj*);
		if(!o||o->parent!=obj)continue;
		mt=lv_obj_get_style_margin_top(o->obj,LV_OBJ_PART_MAIN);
		mb=lv_obj_get_style_margin_bottom(o->obj,LV_OBJ_PART_MAIN);
		ml=lv_obj_get_style_margin_left(o->obj,LV_OBJ_PART_MAIN);
		mr=lv_obj_get_style_margin_right(o->obj,LV_OBJ_PART_MAIN);
		switch(obj->type){
			case OBJ_VER_BOX:
				if(o->auto_width){
					o->width=obj->width-pl-ml-pr-mr;
					lv_obj_set_width(o->obj,o->width);
				}else{
					o->x=(obj->width-o->width)/2;
					lv_obj_set_x(o->obj,o->x);
				}
			break;
			case OBJ_HOR_BOX:
				if(o->auto_height){
					o->height=obj->height-pt-mt-mb-pb;
					lv_obj_set_height(o->obj,o->height);
				}else{
					o->y=(obj->height-o->height)/2;
					lv_obj_set_y(o->obj,o->y);
				}
			break;
			default:continue;
		}
		render_expand_parent(o);
	}while((l=l->next));
	return true;
}

#if LV_USE_TEXTAREA
bool xml_attr_object_pre_text(xml_render_obj*obj){
	obj->obj=lv_textarea_create(obj->parent->obj,NULL);
	if(!obj->obj)return false;
	lv_textarea_set_text(obj->obj,"");
	obj->bind_input=true;
	return true;
}

static void input_cb(xml_render_event*e){
	sysbar_focus_input(render_event_get_lvgl_object(e));
	sysbar_keyboard_open();
}

bool xml_attr_object_post_text(xml_render_obj*obj){
	if(obj->render->initialized)return true;
	if(obj->bind_input){
		lv_textarea_set_cursor_hidden(obj->obj,true);
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

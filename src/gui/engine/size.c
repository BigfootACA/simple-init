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

static void fix_max_min(xml_render_obj*obj){
	if(obj->width<obj->min_width)
		obj->width=obj->min_width;
	if(obj->height<obj->min_height)
		obj->height=obj->min_height;
	if(obj->max_width!=0&&obj->width>obj->max_width)
		obj->width=obj->max_width;
	if(obj->max_height!=0&&obj->height>obj->max_height)
		obj->height=obj->max_height;
}

void render_expand_parent_width(xml_render_obj*obj){
	fix_max_min(obj);
	if(!obj->parent)return;
	if(!obj->parent->auto_width)return;
	lv_coord_t w,pr,mr;
	pr=lv_obj_get_style_pad_right(obj->parent->obj,LV_OBJ_PART_MAIN);
	mr=lv_obj_get_style_margin_right(obj->obj,LV_OBJ_PART_MAIN);
	w=obj->width+obj->x+pr+mr;
	if(w>obj->parent->width){
		obj->parent->width=w;
		render_expand_parent_width(obj->parent);
	}
}

void render_expand_parent_height(xml_render_obj*obj){
	fix_max_min(obj);
	if(!obj->parent)return;
	if(!obj->parent->auto_height)return;
	lv_coord_t h,pb,mb;
	pb=lv_obj_get_style_pad_bottom(obj->parent->obj,LV_OBJ_PART_MAIN);
	mb=lv_obj_get_style_margin_bottom(obj->obj,LV_OBJ_PART_MAIN);
	h=obj->height+obj->y+pb+mb;
	if(h>obj->parent->height){
		obj->parent->height=h;
		render_expand_parent_height(obj->parent);
	}
}

void render_expand_parent(xml_render_obj*obj){
	render_expand_parent_width(obj);
	render_expand_parent_height(obj);
}

void render_reload_size(xml_render_obj*obj){
	obj->width=lv_obj_get_width(obj->obj);
	obj->height=lv_obj_get_height(obj->obj);
	render_expand_parent(obj);
}

void render_reload_pos(xml_render_obj*obj){
	if(obj->cont){
		obj->x=lv_obj_get_x(obj->cont);
		obj->y=lv_obj_get_y(obj->cont);
	}else{
		obj->x=lv_obj_get_x(obj->obj);
		obj->y=lv_obj_get_y(obj->obj);
	}
	if(obj->parent){
		if(obj->y==0)obj->y+=lv_obj_get_style_pad_top(
			obj->parent->obj,
			LV_OBJ_PART_MAIN
		);
		if(obj->x==0)obj->x+=lv_obj_get_style_pad_left(
			obj->parent->obj,
			LV_OBJ_PART_MAIN
		);
	}
	render_expand_parent(obj);
}

void render_apply_size(xml_render_obj*obj){
	if(!obj->parent)return;
	lv_coord_t ct=0,cb=0,cl=0,cr=0;
	if(OBJ_IS_CONTAINER(obj)){
		ct=lv_obj_get_style_pad_top(obj->obj,LV_OBJ_PART_MAIN);
		cb=lv_obj_get_style_pad_bottom(obj->obj,LV_OBJ_PART_MAIN);
		cl=lv_obj_get_style_pad_left(obj->obj,LV_OBJ_PART_MAIN);
		cr=lv_obj_get_style_pad_right(obj->obj,LV_OBJ_PART_MAIN);
	}
	if(obj->auto_width)lv_obj_set_width(obj->obj,obj->width);
	if(obj->auto_height)lv_obj_set_height(obj->obj,obj->height);
	if(obj->cont){
		lv_obj_set_x(obj->cont,obj->x);
		lv_obj_set_y(obj->cont,obj->y);
		lv_obj_set_x(obj->obj,cl);
		lv_obj_set_y(obj->obj,cr);
		lv_obj_set_width(obj->cont,cl+obj->width+cr);
		lv_obj_set_height(obj->cont,ct+obj->height+cb);
	}else{
		lv_obj_set_x(obj->obj,obj->x);
		lv_obj_set_y(obj->obj,obj->y);
	}
	render_apply_size(obj->parent);
}
#endif
#endif

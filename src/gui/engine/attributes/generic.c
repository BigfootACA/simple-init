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

bool xml_attr_handle_apply_text(xml_render_obj_attr*obj){
	char*v=obj->value;
	if(v&&*v&&obj->obj->translate)v=_(v);
	switch(obj->obj->type){
		case OBJ_LABEL:lv_label_set_text(obj->obj->obj,v);break;
		case OBJ_TEXTAREA:lv_textarea_set_text(obj->obj->obj,v);break;
		case OBJ_CHECKBOX:lv_checkbox_set_text(obj->obj->obj,v);break;
		case OBJ_DROPDOWN:lv_dropdown_set_text(obj->obj->obj,v);break;
		default:
			tlog_warn("current obj type unsupported attribute %s",obj->key);
			return false;
	}
	render_reload_size(obj->obj);
	return true;
}

bool xml_attr_handle_pre_theme(xml_render_obj_attr*obj){
	bool match=false;
	lv_theme_style_t theme;
	for(size_t i=0;xml_theme_specs[i].valid;i++){
		if(strcasecmp(
			xml_theme_specs[i].name,
			obj->value
		)!=0)continue;
		theme=xml_theme_specs[i].theme;
		match=true;
		break;
	}
	if(!match){
		tlog_warn("invalid theme: %s",obj->value);
		return false;
	}
	lv_theme_apply(obj->obj->obj,theme);
	return true;
}

bool xml_attr_handle_pre_long_mode(xml_render_obj_attr*obj){
	bool match=false;
	for(size_t i=0;xml_label_longs[i].valid;i++){
		if(strcasecmp(
			xml_label_longs[i].name,
			obj->value
		)!=0)continue;
		obj->fields.long_mode_val=
			xml_label_longs[i].mode;
		match=true;
		break;
	}
	if(!match)tlog_warn("invalid long mode: %s",obj->value);
	return match;
}

bool xml_attr_handle_pre_text_align(xml_render_obj_attr*obj){
	bool match=false;
	for(size_t i=0;xml_label_aligns[i].valid;i++){
		if(strcasecmp(
			xml_label_aligns[i].name,
			obj->value
		)!=0)continue;
		obj->fields.text_align_val=
			xml_label_aligns[i].align;
		match=true;
		break;
	}
	if(!match)tlog_warn("invalid text align: %s",obj->value);
	return match;
}

bool xml_attr_handle_pre_layout(xml_render_obj_attr*obj){
	bool match=false;
	for(size_t i=0;xml_layout_specs[i].valid;i++){
		if(strcasecmp(
			xml_layout_specs[i].name,
			obj->value
		)!=0)continue;
		obj->fields.layout_val=
			xml_layout_specs[i].layout;
		match=true;
		break;
	}
	if(!match)tlog_warn("invalid layout: %s",obj->value);
	return match;
}

bool xml_attr_handle_apply_long_mode(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_LABEL)
	lv_label_set_long_mode(
		obj->obj->obj,
		obj->fields.long_mode_val
	);
	return true;
}

bool xml_attr_handle_apply_text_align(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_LABEL)
	lv_label_set_align(
		obj->obj->obj,
		obj->fields.text_align_val
	);
	return true;
}

bool xml_attr_handle_apply_layout(
	xml_render_obj_attr*obj
){
	switch(obj->obj->type){
		case OBJ_BTN:lv_btn_set_layout(obj->obj->obj,obj->fields.layout_val);break;
		case OBJ_LIST:lv_list_set_layout(obj->obj->obj,obj->fields.layout_val);break;
		case OBJ_PAGE:lv_page_set_scrl_layout(obj->obj->obj,obj->fields.layout_val);break;
		default:
			tlog_warn("current obj type unsupported attribute %s",obj->key);
			return false;
	}
	return true;
}
#endif
#endif

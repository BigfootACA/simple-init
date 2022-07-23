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
#include"gui/string.h"
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
	return true;
}

bool xml_attr_handle_pre_long_mode(xml_render_obj_attr*obj){
	bool match=lv_name_to_label_long_mode(
		obj->value,&obj->fields.long_mode_val
	);
	if(!match)tlog_warn("invalid long mode: %s",obj->value);
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

bool xml_attr_handle_pre_size_mode(xml_render_obj_attr*obj){
	bool match=lv_name_to_img_size_mode(
		obj->value,&obj->fields.size_mode_val
	);
	if(!match)tlog_warn("invalid size mode: %s",obj->value);
	return match;
}

bool xml_attr_handle_apply_size_mode(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_IMG)
	lv_img_set_size_mode(
		obj->obj->obj,
		obj->fields.size_mode_val
	);
	return true;
}

#endif
#endif

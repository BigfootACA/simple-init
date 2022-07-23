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

bool xml_attr_handle_apply_oneline(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_TEXTAREA)
	lv_textarea_set_one_line(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_password(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_TEXTAREA)
	lv_textarea_set_password_mode(
		obj->obj->obj,
		obj->fields.bool_val
	);
	return true;
}

bool xml_attr_handle_apply_max_len(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_TEXTAREA)
	lv_textarea_set_max_length(
		obj->obj->obj,
		obj->fields.uint32_val
	);
	return true;
}

bool xml_attr_handle_apply_placeholder(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_TEXTAREA)
	lv_textarea_set_placeholder_text(
		obj->obj->obj,
		obj->value
	);
	return true;
}
#endif
#endif

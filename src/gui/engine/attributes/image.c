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

bool xml_attr_handle_apply_src(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_IMG)
	lv_img_set_src(
		obj->obj->obj,
		obj->value
	);
	return true;
}

bool xml_attr_handle_apply_zoom(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_IMG)
	lv_img_set_zoom(
		obj->obj->obj,
		obj->fields.uint16_val
	);
	return true;
}

bool xml_attr_handle_apply_fill(
	xml_render_obj_attr*obj
){
	CHECK_TYPE(obj,OBJ_IMG)
	lv_img_fill_image(
		obj->obj->obj,
		obj->obj->width,
		obj->obj->height
	);
	return true;
}

bool xml_attr_handle_pre_zoom(xml_render_obj_attr*obj){
	size_t len;
	uint16_t val;
	char*end=NULL;
	bool percent=false,result=false;
	if((len=strlen(obj->value))<=0)goto done;
	if(obj->value[len-1]=='%')
		percent=true,obj->value[len-1]=0;
	errno=0;
	val=(uint16_t)strtol(obj->value,&end,0);
	if(*end||obj->value==end||errno!=0)
		EDONE(tlog_error("invalid value: %s",obj->value));
	if(percent)val=val*256/100;
	if(val<=0||val>=768)
		EDONE(tlog_error("invalid value: %s",obj->value));
	obj->fields.uint16_val=val;
	result=true;
	done:
	return result;
}
#endif
#endif

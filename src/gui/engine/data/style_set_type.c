/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"str.h"
#include"gui/tools.h"
#include"../render_internal.h"

static int style_set_type_int(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	lv_style_int_t i=(lv_style_int_t)
		render_resolve_coord(obj,max,val);
	_lv_style_set_int(style,p,i);
	return 0;
}

static int style_set_type_color(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_color_t c;
	if(!lv_parse_color_string(val,&c))
		return trlog_warn(
			-1,"invalid color value: %s",val
		);
	_lv_style_set_color(style,p,c);
	return 0;
}

static int style_set_type_bool(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	if(string_is_false(val))
		_lv_style_set_int(style,p,0);
	else if(string_is_true(val))
		_lv_style_set_int(style,p,1);
	else return trlog_warn(
		-1,"invalid boolean value: %s",val
	);
	return 0;
}

static int style_set_type_opa(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	errno=0;
	char*end=NULL;
	int i=(int)strtol(val,&end,0);
	if(*end||end==val||errno!=0||i<0||i>100)
		return trlog_warn(
			-1,"invalid opa value: %s",val
		);
	_lv_style_set_opa(style,p,i*max/100);
	return 0;
}

static int style_set_type_blend(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_blend_mode_t mode=LV_BLEND_MODE_NORMAL;
	#ifdef LV_USE_BLEND_MODES
	if(strcasecmp(val,"subtractive")==0)
		mode=LV_BLEND_MODE_SUBTRACTIVE;
	else if(strcasecmp(val,"additive")==0)
		mode=LV_BLEND_MODE_ADDITIVE;
	else if(strcasecmp(val,"normal")==0)
		mode=LV_BLEND_MODE_NORMAL;
	else return trlog_warn(
		-1,"invalid blend mode value: %s",val
	);
	#endif
	_lv_style_set_int(style,p,mode);
	return 0;
}

static int style_set_type_grad_dir(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_grad_dir_t dir;
	if(strcasecmp(val,"none")==0)
		dir=LV_GRAD_DIR_NONE;
	else if(strcasecmp(val,"hor")==0)
		dir=LV_GRAD_DIR_HOR;
	else if(strcasecmp(val,"ver")==0)
		dir=LV_GRAD_DIR_VER;
	else if(strcasecmp(val,"horizontal")==0)
		dir=LV_GRAD_DIR_HOR;
	else if(strcasecmp(val,"vertical")==0)
		dir=LV_GRAD_DIR_VER;
	else return trlog_warn(
		-1,"invalid grad dir value: %s",val
	);
	_lv_style_set_int(style,p,dir);
	return 0;
}

static int style_set_type_border_side(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_border_side_t side;
	if(strcasecmp(val,"none")==0)
		side=LV_BORDER_SIDE_NONE;
	else if(strcasecmp(val,"bottom")==0)
		side=LV_BORDER_SIDE_BOTTOM;
	else if(strcasecmp(val,"top")==0)
		side=LV_BORDER_SIDE_TOP;
	else if(strcasecmp(val,"left")==0)
		side=LV_BORDER_SIDE_LEFT;
	else if(strcasecmp(val,"right")==0)
		side=LV_BORDER_SIDE_RIGHT;
	else if(strcasecmp(val,"full")==0)
		side=LV_BORDER_SIDE_FULL;
	else return trlog_warn(
		-1,"invalid border side value: %s",val
	);
	_lv_style_set_int(style,p,side);
	return 0;
}

static int style_set_type_text_decor(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_text_decor_t decor;
	if(strcasecmp(val,"none")==0)
		decor=LV_TEXT_DECOR_NONE;
	else if(strcasecmp(val,"underline")==0)
		decor=LV_TEXT_DECOR_UNDERLINE;
	else if(strcasecmp(val,"strikethrough")==0)
		decor=LV_TEXT_DECOR_STRIKETHROUGH;
	else return trlog_warn(
		-1,"invalid text decor value: %s",val
	);
	_lv_style_set_int(style,p,decor);
	return 0;
}

static int style_set_type_align(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_align_t align;
	bool match=false;
	for(size_t i=0;xml_align_specs[i].valid;i++){
		if(strcasecmp(
			xml_align_specs[i].name,
			val
		)!=0)continue;
		align=xml_align_specs[i].align;
		match=true;
		break;
	}
	if(!match)return trlog_warn(
		-1,"invalid grad dir value: %s",val
	);
	_lv_style_set_int(style,p,align);
	return 0;
}

static int style_set_type_string(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	_lv_style_set_ptr(style,p,val);
	return 0;
}

xml_style_set_type xml_style_set_types[]={
	[STYLE_NONE]        = NULL,
	[STYLE_INT]         = style_set_type_int,
	[STYLE_BOOL]        = style_set_type_bool,
	[STYLE_COLOR]       = style_set_type_color,
	[STYLE_OPA]         = style_set_type_opa,
	[STYLE_STRING]      = style_set_type_string,
	[STYLE_FONT]        = NULL,
	[STYLE_IMAGE]       = NULL,
	[STYLE_ANIM]        = NULL,
	[STYLE_BLEND]       = style_set_type_blend,
	[STYLE_GRAD_DIR]    = style_set_type_grad_dir,
	[STYLE_ALIGN]       = style_set_type_align,
	[STYLE_BORDER_SIDE] = style_set_type_border_side,
	[STYLE_TEXT_DECOR]  = style_set_type_text_decor,
	[STYLE_LAST]        = NULL
};
#endif

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

static int style_set_type_int(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max,
	char*val
){
	lv_style_value_t v;
	v.num=render_resolve_coord(obj,max,val);
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_color(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	if(!lv_parse_color_string(val,&v.color))return trlog_warn(
		-1,"invalid color value: %s",val
	);
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_bool(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	if(string_is_false(val))v.num=0;
	else if(string_is_true(val))v.num=1;
	else return trlog_warn(
		-1,"invalid boolean value: %s",val
	);
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_blend(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_blend_mode_t mode=LV_BLEND_MODE_NORMAL;
	#ifdef LV_USE_BLEND_MODES
	if(!lv_name_to_blend_mode(val,&mode))return trlog_warn(
		-1,"invalid blend mode value: %s",val
	);
	#endif
	v.num=mode;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_grad_dir(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_grad_dir_t dir=LV_DIR_NONE;
	if(!lv_name_to_grad_dir(val,&dir))return trlog_warn(
		-1,"invalid grad dir value: %s",val
	);
	v.num=dir;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_border_side(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_border_side_t side=LV_BORDER_SIDE_NONE;
	if(!lv_name_to_border_side(val,&side))return trlog_warn(
		-1,"invalid border side value: %s",val
	);
	v.num=side;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_text_decor(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_text_decor_t decor=LV_TEXT_DECOR_NONE;
	if(!lv_name_to_text_decor(val,&decor))return trlog_warn(
		-1,"invalid text decor value: %s",val
	);
	v.num=decor;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_text_align(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_text_align_t align=LV_TEXT_ALIGN_AUTO;
	if(!lv_name_to_text_align(val,&align))return trlog_warn(
		-1,"invalid text align value: %s",val
	);
	v.num=align;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_align(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v;
	lv_align_t align=LV_ALIGN_DEFAULT;
	if(!lv_name_to_align(val,&align))return trlog_warn(
		-1,"invalid align value: %s",val
	);
	v.num=align;
	lv_style_set_prop(style,p,v);
	return 0;
}

static int style_set_type_string(
	xml_render_obj*obj __attribute__((unused)),
	lv_style_t*style,
	lv_style_prop_t p,
	lv_coord_t max __attribute__((unused)),
	char*val
){
	lv_style_value_t v={.ptr=val};
	lv_style_set_prop(style,p,v);
	return 0;
}

xml_style_set_type xml_style_set_types[]={
	[STYLE_NONE]           = NULL,
	[STYLE_INT]            = style_set_type_int,
	[STYLE_BOOL]           = style_set_type_bool,
	[STYLE_COLOR]          = style_set_type_color,
	[STYLE_COLOR_FILTER]   = NULL,
	[STYLE_OPA]            = style_set_type_int,
	[STYLE_STRING]         = style_set_type_string,
	[STYLE_FONT]           = NULL,
	[STYLE_IMAGE]          = NULL,
	[STYLE_ANIM]           = NULL,
	[STYLE_BLEND]          = style_set_type_blend,
	[STYLE_GRAD_DIR]       = style_set_type_grad_dir,
	[STYLE_GRAD]           = NULL,
	[STYLE_ALIGN]          = style_set_type_align,
	[STYLE_BORDER_SIDE]    = style_set_type_border_side,
	[STYLE_TEXT_DECOR]     = style_set_type_text_decor,
	[STYLE_TEXT_ALIGN]     = style_set_type_text_align,
	[STYLE_BASE_DIR]       = NULL,
	[STYLE_TRANSITION]     = NULL,
	[STYLE_DITHER]         = NULL,
	[STYLE_LAST]           = NULL
};
#endif
#endif

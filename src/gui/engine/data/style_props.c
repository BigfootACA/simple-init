/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"../render_internal.h"

#define DECL_STYLE(_prop,_name,_type,_max){\
	.valid=true,\
	.prop=(_prop),\
	.name=(#_name),\
	.type=(_type),\
	.hand=NULL,\
        .max=(_max),\
},
#define DECL_STYLE_ALIAS(_hand,_name,_type){\
	.valid=true,\
	.prop=0,\
	.name=(#_name),\
	.type=(_type),\
	.hand=(_hand),\
        .max=0,\
},

static int _prop_margin_hor(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_MARGIN_RIGHT|p,
		max,val
	);
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_MARGIN_LEFT|p,
		max,val
	);
	return 0;
}

static int _prop_margin_ver(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_MARGIN_TOP|p,
		max,val
	);
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_MARGIN_BOTTOM|p,
		max,val
	);
	return 0;
}

static int _prop_margin_all(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	_prop_margin_hor(obj,style,p,max,val);
	_prop_margin_ver(obj,style,p,max,val);
	return 0;
}

static int _prop_pad_hor(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_PAD_RIGHT|p,
		max,val
	);
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_PAD_LEFT|p,
		max,val
	);
	return 0;
}

static int _prop_pad_ver(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_PAD_TOP|p,
		max,val
	);
	xml_style_set_types[STYLE_INT](
		obj,style,
		LV_STYLE_PAD_BOTTOM|p,
		max,val
	);
	return 0;
}

static int _prop_pad_all(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
){
	_prop_pad_hor(obj,style,p,max,val);
	_prop_pad_ver(obj,style,p,max,val);
	return 0;
}

xml_style_prop xml_style_props[]={
	DECL_STYLE(LV_STYLE_RADIUS,                 radius,                 STYLE_INT,         LV_RADIUS_CIRCLE )
	DECL_STYLE(LV_STYLE_CLIP_CORNER,            clip-corner,            STYLE_BOOL,        0                )
	DECL_STYLE(LV_STYLE_SIZE,                   size,                   STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSFORM_WIDTH,        transform-width,        STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSFORM_HEIGHT,       transform-height,       STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSFORM_ANGLE,        transform-angle,        STYLE_INT,         360              )
	DECL_STYLE(LV_STYLE_TRANSFORM_ZOOM,         transform-zoom,         STYLE_INT,         256              )
	DECL_STYLE(LV_STYLE_OPA_SCALE,              opa-scale,              STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_PAD_TOP,                pad-top,                STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_PAD_BOTTOM,             pad-bottom,             STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_PAD_LEFT,               pad-left,               STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_PAD_RIGHT,              pad-right,              STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_PAD_INNER,              pad-inner,              STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_MARGIN_TOP,             margin-top,             STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_MARGIN_BOTTOM,          margin-bottom,          STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_MARGIN_LEFT,            margin-left,            STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_MARGIN_RIGHT,           margin-right,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_BG_BLEND_MODE,          bg-blend-mode,          STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_BG_MAIN_STOP,           bg-main-stop,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_BG_GRAD_STOP,           bg-grad-stop,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_BG_GRAD_DIR,            bg-grad-dir,            STYLE_GRAD_DIR,    0                )
	DECL_STYLE(LV_STYLE_BG_COLOR,               bg-color,               STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_BG_GRAD_COLOR,          bg-grad-color,          STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_BG_OPA,                 bg-opa,                 STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_BORDER_WIDTH,           border-width,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_BORDER_SIDE,            border-side,            STYLE_BORDER_SIDE, 0                )
	DECL_STYLE(LV_STYLE_BORDER_BLEND_MODE,      border-blend_mode,      STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_BORDER_POST,            border-post,            STYLE_BOOL,        0                )
	DECL_STYLE(LV_STYLE_BORDER_COLOR,           border-color,           STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_BORDER_OPA,             border-opa,             STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_OUTLINE_WIDTH,          outline-width,          STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_OUTLINE_PAD,            outline-pad,            STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_OUTLINE_BLEND_MODE,     outline-blend_mode,     STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_OUTLINE_COLOR,          outline-color,          STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_OUTLINE_OPA,            outline-opa,            STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_SHADOW_WIDTH,           shadow-width,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SHADOW_OFS_X,           shadow-ofs-x,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SHADOW_OFS_Y,           shadow-ofs-y,           STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SHADOW_SPREAD,          shadow-spread,          STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SHADOW_BLEND_MODE,      shadow-blend_mode,      STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_SHADOW_COLOR,           shadow-color,           STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_SHADOW_OPA,             shadow-opa,             STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_PATTERN_REPEAT,         pattern-repeat,         STYLE_BOOL,        0                )
	DECL_STYLE(LV_STYLE_PATTERN_BLEND_MODE,     pattern-blend-mode,     STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_PATTERN_RECOLOR,        pattern-recolor,        STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_PATTERN_OPA,            pattern-opa,            STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_PATTERN_RECOLOR_OPA,    pattern-recolor_opa,    STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_PATTERN_IMAGE,          pattern-image,          STYLE_IMAGE,       0                )
	DECL_STYLE(LV_STYLE_VALUE_LETTER_SPACE,     value-letter-space,     STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_VALUE_LINE_SPACE,       value-line-space,       STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_VALUE_BLEND_MODE,       value-blend-mode,       STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_VALUE_OFS_X,            value-ofs-x,            STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_VALUE_OFS_Y,            value-ofs-y,            STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_VALUE_ALIGN,            value-align,            STYLE_ALIGN,       0                )
	DECL_STYLE(LV_STYLE_VALUE_COLOR,            value-color,            STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_VALUE_OPA,              value-opa,              STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_VALUE_FONT,             value-font,             STYLE_FONT,        0                )
	DECL_STYLE(LV_STYLE_VALUE_STR,              value-str,              STYLE_STRING,      0                )
	DECL_STYLE(LV_STYLE_TEXT_LETTER_SPACE,      text-letter-space,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TEXT_LINE_SPACE,        text-line-space,        STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TEXT_DECOR,             text-decor,             STYLE_TEXT_DECOR,  0                )
	DECL_STYLE(LV_STYLE_TEXT_BLEND_MODE,        text-blend-mode,        STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_TEXT_COLOR,             text-color,             STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_TEXT_SEL_COLOR,         text-sel-color,         STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_TEXT_SEL_BG_COLOR,      text-sel-bg-color,      STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_TEXT_OPA,               text-opa,               STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_TEXT_FONT,              text-font,              STYLE_FONT,        0                )
	DECL_STYLE(LV_STYLE_LINE_WIDTH,             line-width,             STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_LINE_BLEND_MODE,        line-blend-mode,        STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_LINE_DASH_WIDTH,        line-dash-width,        STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_LINE_DASH_GAP,          line-dash-gap,          STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_LINE_ROUNDED,           line-rounded,           STYLE_BOOL,        0                )
	DECL_STYLE(LV_STYLE_LINE_COLOR,             line-color,             STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_LINE_OPA,               line-opa,               STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_IMAGE_BLEND_MODE,       image-blend-mode,       STYLE_BLEND,       0                )
	DECL_STYLE(LV_STYLE_IMAGE_RECOLOR,          image-recolor,          STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_IMAGE_OPA,              image-opa,              STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_IMAGE_RECOLOR_OPA,      image-recolor-opa,      STYLE_OPA,         LV_OPA_COVER     )
	DECL_STYLE(LV_STYLE_TRANSITION_TIME,        transition-time,        STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_DELAY,       transition-delay,       STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_1,      transition-prop-1,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_2,      transition-prop-2,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_3,      transition-prop-3,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_4,      transition-prop-4,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_5,      transition-prop-5,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PROP_6,      transition-prop-6,      STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SCALE_WIDTH,            scale-width,            STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SCALE_BORDER_WIDTH,     scale-border-width,     STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SCALE_END_BORDER_WIDTH, scale-end-border-width, STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SCALE_END_LINE_WIDTH,   scale-end-line-width,   STYLE_INT,         0                )
	DECL_STYLE(LV_STYLE_SCALE_GRAD_COLOR,       scale-grad-color,       STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_SCALE_END_COLOR,        scale-end-color,        STYLE_COLOR,       0                )
	DECL_STYLE(LV_STYLE_TRANSITION_PATH,        transition-path,        STYLE_ANIM,        0                )
	DECL_STYLE_ALIAS(_prop_margin_all,          margin,                 STYLE_INT)
	DECL_STYLE_ALIAS(_prop_margin_all,          margin-all,             STYLE_INT)
	DECL_STYLE_ALIAS(_prop_margin_ver,          margin-ver,             STYLE_INT)
	DECL_STYLE_ALIAS(_prop_margin_hor,          margin-hor,             STYLE_INT)
	DECL_STYLE_ALIAS(_prop_margin_ver,          margin-vertical,        STYLE_INT)
	DECL_STYLE_ALIAS(_prop_margin_hor,          margin-horizontal,      STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_all,             pad,                    STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_all,             pad-all,                STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_ver,             pad-ver,                STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_hor,             pad-hor,                STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_ver,             pad-vertical,           STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_hor,             pad-horizontal,         STYLE_INT)

	{.valid=false,.prop=0,.max=0,.name="",.type=0,.hand=NULL}
};
#endif
#endif

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

static int _prop_pad_hor(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_prop_t p,
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
	lv_style_prop_t p,
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
	lv_style_prop_t p,
	lv_coord_t max,
	char*val
){
	_prop_pad_hor(obj,style,p,max,val);
	_prop_pad_ver(obj,style,p,max,val);
	return 0;
}

xml_style_prop xml_style_props[]={
	DECL_STYLE(LV_STYLE_WIDTH,                   width,                   STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_MIN_WIDTH,               min-width,               STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_MAX_WIDTH,               max-width,               STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_HEIGHT,                  height,                  STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_MIN_HEIGHT,              min-height,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_MAX_HEIGHT,              max-height,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_X,                       x,                       STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_Y,                       y,                       STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_ALIGN,                   align,                   STYLE_ALIGN,        0)
	DECL_STYLE(LV_STYLE_LAYOUT,                  layout,                  STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_RADIUS,                  radius,                  STYLE_INT,          LV_RADIUS_CIRCLE)

	DECL_STYLE(LV_STYLE_PAD_TOP,                 pad-top,                 STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_PAD_BOTTOM,              pad-bottom,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_PAD_LEFT,                pad-left,                STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_PAD_RIGHT,               pad-right,               STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_PAD_ROW,                 pad-row,                 STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_PAD_COLUMN,              pad-column,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_BASE_DIR,                base-dir,                STYLE_BASE_DIR,     0)
	DECL_STYLE(LV_STYLE_CLIP_CORNER,             clip-corner,             STYLE_BOOL,         0)

	DECL_STYLE(LV_STYLE_BG_COLOR,                bg-color,                STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_BG_OPA,                  bg-opa,                  STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_BG_GRAD_COLOR,           bg-grad-color,           STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_BG_GRAD_DIR,             bg-grad-dir,             STYLE_GRAD_DIR,     0)
	DECL_STYLE(LV_STYLE_BG_MAIN_STOP,            bg-main-stop,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_BG_GRAD_STOP,            bg-grad-stop,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_BG_GRAD,                 bg-grad,                 STYLE_GRAD,         0)
	DECL_STYLE(LV_STYLE_BG_DITHER_MODE,          bg-dither-mode,          STYLE_DITHER,       0)
	DECL_STYLE(LV_STYLE_BG_IMG_SRC,              bg-img-src,              STYLE_IMAGE,        0)
	DECL_STYLE(LV_STYLE_BG_IMG_OPA,              bg-img-opa,              STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_BG_IMG_RECOLOR,          bg-img-recolor,          STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_BG_IMG_RECOLOR_OPA,      bg-img-recolor-opa,      STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_BG_IMG_TILED,            bg-img-tiled,            STYLE_BOOL,         0)

	DECL_STYLE(LV_STYLE_BORDER_COLOR,            border-color,            STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_BORDER_OPA,              border-opa,              STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_BORDER_WIDTH,            border-width,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_BORDER_SIDE,             border-side,             STYLE_GRAD_DIR,     0)
	DECL_STYLE(LV_STYLE_BORDER_POST,             border-post,             STYLE_BOOL,         0)
	DECL_STYLE(LV_STYLE_OUTLINE_WIDTH,           outline-width,           STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_OUTLINE_COLOR,           outline-color,           STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_OUTLINE_OPA,             outline-opa,             STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_OUTLINE_PAD,             outline-pad,             STYLE_INT,          0)

	DECL_STYLE(LV_STYLE_SHADOW_WIDTH,            shadow-width,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_SHADOW_OFS_X,            shadow-ofs-x,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_SHADOW_OFS_Y,            shadow-ofs-y,            STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_SHADOW_SPREAD,           shadow-spread,           STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_SHADOW_COLOR,            shadow-color,            STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_SHADOW_OPA,              shadow-opa,              STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_IMG_OPA,                 img-opa,                 STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_IMG_RECOLOR,             img-recolor,             STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_IMG_RECOLOR_OPA,         img-recolor-opa,         STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_LINE_WIDTH,              line-width,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_LINE_DASH_WIDTH,         line-dash-width,         STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_LINE_DASH_GAP,           line-dash-gap,           STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_LINE_ROUNDED,            line-rounded,            STYLE_BOOL,         0)
	DECL_STYLE(LV_STYLE_LINE_COLOR,              line-color,              STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_LINE_OPA,                line-opa,                STYLE_OPA,          LV_OPA_COVER)

	DECL_STYLE(LV_STYLE_ARC_WIDTH,               arc-width,               STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_ARC_ROUNDED,             arc-rounded,             STYLE_BOOL,         0)
	DECL_STYLE(LV_STYLE_ARC_COLOR,               arc-color,               STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_ARC_OPA,                 arc-opa,                 STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_ARC_IMG_SRC,             arc-opa,                 STYLE_IMAGE,        0)
	DECL_STYLE(LV_STYLE_TEXT_COLOR,              text-color,              STYLE_COLOR,        0)
	DECL_STYLE(LV_STYLE_TEXT_OPA,                text-opa,                STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_TEXT_FONT,               text-font,               STYLE_FONT,         0)
	DECL_STYLE(LV_STYLE_TEXT_LETTER_SPACE,       text-letter-space,       STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TEXT_LINE_SPACE,         text-line-space,         STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TEXT_DECOR,              text-decor,              STYLE_TEXT_DECOR,   0)
	DECL_STYLE(LV_STYLE_TEXT_ALIGN,              text-align,              STYLE_TEXT_ALIGN,   0)

	DECL_STYLE(LV_STYLE_OPA,                     opa,                     STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_COLOR_FILTER_DSC,        color-filter-dsc,        STYLE_COLOR_FILTER, 0)
	DECL_STYLE(LV_STYLE_COLOR_FILTER_OPA,        color-filter-opa,        STYLE_OPA,          LV_OPA_COVER)
	DECL_STYLE(LV_STYLE_ANIM_TIME,               anim-time,               STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_ANIM_SPEED,              anim-speed,              STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSITION,              transition,              STYLE_TRANSITION,   0)
	DECL_STYLE(LV_STYLE_BLEND_MODE,              blend-mode,              STYLE_BLEND,        0)
	DECL_STYLE(LV_STYLE_TRANSFORM_WIDTH,         transform-width,         STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSFORM_HEIGHT,        transform-height,        STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSLATE_X,             translate-x,             STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSLATE_Y,             translate-y,             STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSFORM_ZOOM,          transform-zoom,          STYLE_INT,          256)
	DECL_STYLE(LV_STYLE_TRANSFORM_ANGLE,         transform-angle,         STYLE_INT,          360)
	DECL_STYLE(LV_STYLE_TRANSFORM_PIVOT_X,       translate-pivot-x,       STYLE_INT,          0)
	DECL_STYLE(LV_STYLE_TRANSFORM_PIVOT_Y,       translate-pivot-y,       STYLE_INT,          0)

	DECL_STYLE_ALIAS(_prop_pad_all,              pad,                     STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_all,              pad-all,                 STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_ver,              pad-ver,                 STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_hor,              pad-hor,                 STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_ver,              pad-vertical,            STYLE_INT)
	DECL_STYLE_ALIAS(_prop_pad_hor,              pad-horizontal,          STYLE_INT)

	{.valid=false,.prop=0,.max=0,.name="",.type=0,.hand=NULL}
};
#endif
#endif

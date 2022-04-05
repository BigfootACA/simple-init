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

#define DECL_LONG_MODE(_name,_mode){\
	.valid=true,\
	.name=(#_name),\
	.mode=(_mode)\
}

xml_label_long xml_label_longs[]={
	DECL_LONG_MODE(expand,        LV_LABEL_LONG_EXPAND),
	DECL_LONG_MODE(break,         LV_LABEL_LONG_BREAK),
	DECL_LONG_MODE(dot,           LV_LABEL_LONG_DOT),
	DECL_LONG_MODE(sroll,         LV_LABEL_LONG_SROLL),
	DECL_LONG_MODE(scroll,        LV_LABEL_LONG_SROLL),
	DECL_LONG_MODE(sroll-cric,    LV_LABEL_LONG_SROLL_CIRC),
	DECL_LONG_MODE(sroll-cricle,  LV_LABEL_LONG_SROLL_CIRC),
	DECL_LONG_MODE(scroll-cric,   LV_LABEL_LONG_SROLL_CIRC),
	DECL_LONG_MODE(scroll-cricle, LV_LABEL_LONG_SROLL_CIRC),
	DECL_LONG_MODE(crop,          LV_LABEL_LONG_CROP),
	{.valid=false,.name="",.mode=0}
};
#endif
#endif

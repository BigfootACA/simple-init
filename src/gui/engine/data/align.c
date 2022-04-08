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

#define DECL_ALIGN(_name,_align){\
	.valid=true,\
	.name=(#_name),\
	.align=(_align)\
}

xml_align_spec xml_align_specs[]={
	DECL_ALIGN(center,           LV_ALIGN_CENTER),
	DECL_ALIGN(middle,           LV_ALIGN_CENTER),
	DECL_ALIGN(mid,              LV_ALIGN_CENTER),
	DECL_ALIGN(left,             LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(top-left,         LV_ALIGN_OUT_TOP_LEFT),
	DECL_ALIGN(mid-left,         LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(bottom-left,      LV_ALIGN_OUT_BOTTOM_LEFT),
	DECL_ALIGN(left-top,         LV_ALIGN_OUT_TOP_LEFT),
	DECL_ALIGN(left-mid,         LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(left-middle,      LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(left-center,      LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(left-bottom,      LV_ALIGN_OUT_BOTTOM_LEFT),
	DECL_ALIGN(right,            LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(top-right,        LV_ALIGN_OUT_TOP_RIGHT),
	DECL_ALIGN(mid-right,        LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(middle-right,     LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(center-right,     LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(bottom-right,     LV_ALIGN_OUT_BOTTOM_RIGHT),
	DECL_ALIGN(right-top,        LV_ALIGN_OUT_TOP_RIGHT),
	DECL_ALIGN(right-mid,        LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(right-middle,     LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(right-center,     LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(right-bottom,     LV_ALIGN_OUT_BOTTOM_RIGHT),
	DECL_ALIGN(top,              LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(top-mid,          LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(top-middle,       LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(top-center,       LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(bottom,           LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(bottom-mid,       LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(bottom-middle,    LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(bottom-center,    LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(out-left,         LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-top-left,     LV_ALIGN_OUT_TOP_LEFT),
	DECL_ALIGN(out-mid-left,     LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-middle-left,  LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-center-left,  LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-bottom-left,  LV_ALIGN_OUT_BOTTOM_LEFT),
	DECL_ALIGN(out-left-top,     LV_ALIGN_OUT_TOP_LEFT),
	DECL_ALIGN(out-left-mid,     LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-left-middle,  LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-left-center,  LV_ALIGN_OUT_LEFT_MID),
	DECL_ALIGN(out-left-bottom,  LV_ALIGN_OUT_BOTTOM_LEFT),
	DECL_ALIGN(out-right,        LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-top-right,    LV_ALIGN_OUT_TOP_RIGHT),
	DECL_ALIGN(out-mid-right,    LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-middle-right, LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-center-right, LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-bottom-right, LV_ALIGN_OUT_BOTTOM_RIGHT),
	DECL_ALIGN(out-right-top,    LV_ALIGN_OUT_TOP_RIGHT),
	DECL_ALIGN(out-right-mid,    LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-right-middle, LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-right-center, LV_ALIGN_OUT_RIGHT_MID),
	DECL_ALIGN(out-right-bottom, LV_ALIGN_OUT_BOTTOM_RIGHT),
	DECL_ALIGN(out-top,          LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(out-top-mid,      LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(out-top-middle,   LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(out-top-center,   LV_ALIGN_OUT_TOP_MID),
	DECL_ALIGN(out-bottom,       LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(out-bottom-mid,   LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(out-bottom-middle,LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(out-bottom-center,LV_ALIGN_OUT_BOTTOM_MID),
	DECL_ALIGN(in-left,          LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-top-left,      LV_ALIGN_IN_TOP_LEFT),
	DECL_ALIGN(in-mid-left,      LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-middle-left,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-center-left,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-bottom-left,   LV_ALIGN_IN_BOTTOM_LEFT),
	DECL_ALIGN(in-left-top,      LV_ALIGN_IN_TOP_LEFT),
	DECL_ALIGN(in-left-mid,      LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-left-middle,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-left-center,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-left-middle,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-left-center,   LV_ALIGN_IN_LEFT_MID),
	DECL_ALIGN(in-left-bottom,   LV_ALIGN_IN_BOTTOM_LEFT),
	DECL_ALIGN(in-right,         LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-top-right,     LV_ALIGN_IN_TOP_RIGHT),
	DECL_ALIGN(in-mid-right,     LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-middle-right,  LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-center-right,  LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-bottom-right,  LV_ALIGN_IN_BOTTOM_RIGHT),
	DECL_ALIGN(in-right-top,     LV_ALIGN_IN_TOP_RIGHT),
	DECL_ALIGN(in-right-mid,     LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-right-middle,  LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-right-center,  LV_ALIGN_IN_RIGHT_MID),
	DECL_ALIGN(in-right-bottom,  LV_ALIGN_IN_BOTTOM_RIGHT),
	DECL_ALIGN(in-top,           LV_ALIGN_IN_TOP_MID),
	DECL_ALIGN(in-top-mid,       LV_ALIGN_IN_TOP_MID),
	DECL_ALIGN(in-top-middle,    LV_ALIGN_IN_TOP_MID),
	DECL_ALIGN(in-top-center,    LV_ALIGN_IN_TOP_MID),
	DECL_ALIGN(in-bottom,        LV_ALIGN_IN_BOTTOM_MID),
	DECL_ALIGN(in-bottom-mid,    LV_ALIGN_IN_BOTTOM_MID),
	DECL_ALIGN(in-bottom-middle, LV_ALIGN_IN_BOTTOM_MID),
	DECL_ALIGN(in-bottom-center, LV_ALIGN_IN_BOTTOM_MID),
	{.valid=false,.name="",.align=0}
};
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"../render_internal.h"

#define DECL_LAYOUT(_name,_layout){\
	.valid=true,\
	.name=(#_name),\
	.layout=(_layout)\
}

xml_layout_spec xml_layout_specs[]={
	DECL_LAYOUT(off,           LV_LAYOUT_OFF),
	DECL_LAYOUT(center,        LV_LAYOUT_CENTER),
	DECL_LAYOUT(col-left,      LV_LAYOUT_COLUMN_LEFT),
	DECL_LAYOUT(col-mid,       LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(col-middle,    LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(col-center,    LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(col-right,     LV_LAYOUT_COLUMN_RIGHT),
	DECL_LAYOUT(column-left,   LV_LAYOUT_COLUMN_LEFT),
	DECL_LAYOUT(column-mid,    LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(column-middle, LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(column-center, LV_LAYOUT_COLUMN_MID),
	DECL_LAYOUT(column-right,  LV_LAYOUT_COLUMN_RIGHT),
	DECL_LAYOUT(row-top,       LV_LAYOUT_ROW_TOP),
	DECL_LAYOUT(row-mid,       LV_LAYOUT_ROW_MID),
	DECL_LAYOUT(row-middle,    LV_LAYOUT_ROW_MID),
	DECL_LAYOUT(row-center,    LV_LAYOUT_ROW_MID),
	DECL_LAYOUT(row-bottom,    LV_LAYOUT_ROW_BOTTOM),
	DECL_LAYOUT(pretty-top,    LV_LAYOUT_PRETTY_TOP),
	DECL_LAYOUT(pretty-mid,    LV_LAYOUT_PRETTY_MID),
	DECL_LAYOUT(pretty-middle, LV_LAYOUT_PRETTY_MID),
	DECL_LAYOUT(pretty-center, LV_LAYOUT_PRETTY_MID),
	DECL_LAYOUT(pretty-bottom, LV_LAYOUT_PRETTY_BOTTOM),
	DECL_LAYOUT(grid,          LV_LAYOUT_GRID),
	{.valid=false,.name="",.layout=0}
};
#endif

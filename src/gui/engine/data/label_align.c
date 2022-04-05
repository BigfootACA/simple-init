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

#define DECL_LABEL_ALIGN(_name,_align){\
	.valid=true,\
	.name=(#_name),\
	.align=(_align)\
}

xml_label_align xml_label_aligns[]={
	DECL_LABEL_ALIGN(left,   LV_LABEL_ALIGN_LEFT),
	DECL_LABEL_ALIGN(center, LV_LABEL_ALIGN_CENTER),
	DECL_LABEL_ALIGN(right,  LV_LABEL_ALIGN_RIGHT),
	DECL_LABEL_ALIGN(auto,   LV_LABEL_ALIGN_AUTO),
	{.valid=false,.name="",.align=0}
};
#endif
#endif

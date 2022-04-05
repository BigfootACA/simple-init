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

#define DECL_STYLE_STATE(_name,_state){.valid=true,.name=(#_name),.state=(_state)}

xml_style_state xml_style_states[]={
	DECL_STYLE_STATE(default,  LV_STATE_DEFAULT),
	DECL_STYLE_STATE(checked,  LV_STATE_CHECKED),
	DECL_STYLE_STATE(focused,  LV_STATE_FOCUSED),
	DECL_STYLE_STATE(edited,   LV_STATE_EDITED),
	DECL_STYLE_STATE(hovered,  LV_STATE_HOVERED),
	DECL_STYLE_STATE(pressed,  LV_STATE_PRESSED),
	DECL_STYLE_STATE(disabled, LV_STATE_DISABLED),
	{.valid=false,.name="",.state=0}
};
#endif
#endif

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

#define DECL_ATTR_HANDLER(name) \
	extern bool xml_attr_handle_pre_##name(xml_render_obj_attr*obj);\
	extern bool xml_attr_handle_apply_##name(xml_render_obj_attr*obj);

#define DECL_ATTR_HAND(_name,_hand_pre,_hand_apply,_resize){\
		.valid=true,\
		.name=(#_name),\
		.resize=(_resize),\
		.hand_pre=(_hand_pre),\
		.hand_apply=(_hand_apply)\
	}
#define DECL_ATTR_CHAND(_name,_hand,_resize) DECL_ATTR_HAND(\
	_name,\
	xml_attr_handle_pre_##_hand,\
	xml_attr_handle_apply_##_hand,\
	_resize\
)
#define DECL_ATTR_PCHAND(_name,_hand,_resize) DECL_ATTR_HAND(\
	_name,\
	xml_attr_handle_pre_##_hand,\
	NULL,\
	_resize\
)
#define DECL_ATTR_ACHAND(_name,_hand,_resize) DECL_ATTR_HAND(\
	_name,\
	NULL,\
	xml_attr_handle_apply_##_hand,\
	_resize\
)
#define DECL_ATTR_XHAND(_name,_resize)\
	DECL_ATTR_CHAND(_name,_name,_resize)
#define DECL_ATTR_PXHAND(_name,_resize)\
	DECL_ATTR_PCHAND(_name,_name,_resize)
#define DECL_ATTR_AXHAND(_name,_resize)\
	DECL_ATTR_ACHAND(_name,_name,_resize)

DECL_ATTR_HANDLER(top);
DECL_ATTR_HANDLER(left);
DECL_ATTR_HANDLER(right);
DECL_ATTR_HANDLER(width);
DECL_ATTR_HANDLER(height);
DECL_ATTR_HANDLER(bottom);
DECL_ATTR_HANDLER(top_of);
DECL_ATTR_HANDLER(left_of);
DECL_ATTR_HANDLER(right_of);
DECL_ATTR_HANDLER(bottom_of);
DECL_ATTR_HANDLER(max_width);
DECL_ATTR_HANDLER(min_width);
DECL_ATTR_HANDLER(max_height);
DECL_ATTR_HANDLER(min_height);
DECL_ATTR_HANDLER(bind_input);
DECL_ATTR_HANDLER(auto_width);
DECL_ATTR_HANDLER(auto_height);
DECL_ATTR_HANDLER(placeholder);
DECL_ATTR_HANDLER(drag_parent);
DECL_ATTR_HANDLER(drag_throw);
DECL_ATTR_HANDLER(text_align);
DECL_ATTR_HANDLER(long_mode);
DECL_ATTR_HANDLER(clickable);
DECL_ATTR_HANDLER(translate);
DECL_ATTR_HANDLER(password);
DECL_ATTR_HANDLER(checked);
DECL_ATTR_HANDLER(recolor);
DECL_ATTR_HANDLER(enabled);
DECL_ATTR_HANDLER(oneline);
DECL_ATTR_HANDLER(visible);
DECL_ATTR_HANDLER(hidden);
DECL_ATTR_HANDLER(layout);
DECL_ATTR_HANDLER(align);
DECL_ATTR_HANDLER(theme);
DECL_ATTR_HANDLER(square);
DECL_ATTR_HANDLER(text);
DECL_ATTR_HANDLER(drag);
DECL_ATTR_HANDLER(zoom);
DECL_ATTR_HANDLER(fill);
DECL_ATTR_HANDLER(src);

xml_attr_handle xml_attr_handles[]={
	DECL_ATTR_XHAND(width,           true),
	DECL_ATTR_XHAND(height,          true),
	DECL_ATTR_PXHAND(top,            true),
	DECL_ATTR_PXHAND(bottom,         true),
	DECL_ATTR_PXHAND(left,           true),
	DECL_ATTR_PXHAND(right,          true),
	DECL_ATTR_PXHAND(square,         true),
	DECL_ATTR_ACHAND(top-of,         top_of,        true),
	DECL_ATTR_ACHAND(bottom-of,      bottom_of,     true),
	DECL_ATTR_ACHAND(left-of,        left_of,       true),
	DECL_ATTR_ACHAND(right-of,       right_of,      true),
	DECL_ATTR_PCHAND(max-width,      max_width,     true),
	DECL_ATTR_PCHAND(max-height,     max_height,    true),
	DECL_ATTR_PCHAND(min-width,      min_width,     true),
	DECL_ATTR_PCHAND(min-height,     min_height,    true),
	DECL_ATTR_PCHAND(bind-input,     bind_input,    false),
	DECL_ATTR_PCHAND(auto-width,     auto_width,    false),
	DECL_ATTR_PCHAND(auto-height,    auto_height,   false),
	DECL_ATTR_CHAND(drag-parent,     drag_parent,   false),
	DECL_ATTR_CHAND(drag-throw,      drag_throw,    false),
	DECL_ATTR_CHAND(long-mode,       long_mode,     false),
	DECL_ATTR_CHAND(text-align,      text_align,    false),
	DECL_ATTR_XHAND(layout,          false),
	DECL_ATTR_XHAND(drag,            false),
	DECL_ATTR_XHAND(clickable,       false),
	DECL_ATTR_XHAND(recolor,         false),
	DECL_ATTR_XHAND(checked,         false),
	DECL_ATTR_XHAND(enabled,         false),
	DECL_ATTR_XHAND(visible,         false),
	DECL_ATTR_XHAND(hidden,          false),
	DECL_ATTR_XHAND(oneline,         false),
	DECL_ATTR_XHAND(password,        false),
	DECL_ATTR_XHAND(align,           true),
	DECL_ATTR_XHAND(zoom,            true),
	DECL_ATTR_XHAND(fill,            true),
	DECL_ATTR_AXHAND(placeholder,    false),
	DECL_ATTR_PXHAND(translate,      false),
	DECL_ATTR_PXHAND(theme,          false),
	DECL_ATTR_AXHAND(text,           false),
	DECL_ATTR_AXHAND(src,            false),
	{.valid=false,.name="",.resize=false,.hand_pre=NULL,.hand_apply=NULL}
};
#endif
#endif

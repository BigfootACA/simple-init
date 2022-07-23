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

DECL_ATTR_HANDLER(x);
DECL_ATTR_HANDLER(y);
DECL_ATTR_HANDLER(top);
DECL_ATTR_HANDLER(pos);
DECL_ATTR_HANDLER(size);
DECL_ATTR_HANDLER(left);
DECL_ATTR_HANDLER(right);
DECL_ATTR_HANDLER(width);
DECL_ATTR_HANDLER(height);
DECL_ATTR_HANDLER(bottom);
DECL_ATTR_HANDLER(center);
DECL_ATTR_HANDLER(top_of);
DECL_ATTR_HANDLER(left_of);
DECL_ATTR_HANDLER(right_of);
DECL_ATTR_HANDLER(bottom_of);
DECL_ATTR_HANDLER(max_size);
DECL_ATTR_HANDLER(min_size);
DECL_ATTR_HANDLER(cont_size);
DECL_ATTR_HANDLER(max_width);
DECL_ATTR_HANDLER(min_width);
DECL_ATTR_HANDLER(cont_width);
DECL_ATTR_HANDLER(max_height);
DECL_ATTR_HANDLER(min_height);
DECL_ATTR_HANDLER(cont_height);
DECL_ATTR_HANDLER(bind_input);
DECL_ATTR_HANDLER(placeholder);
DECL_ATTR_HANDLER(flex_align);
DECL_ATTR_HANDLER(flex_flow);
DECL_ATTR_HANDLER(flex_grow);
DECL_ATTR_HANDLER(grid_align);
DECL_ATTR_HANDLER(grid_cell);
DECL_ATTR_HANDLER(grid_dsc);
DECL_ATTR_HANDLER(size_mode);
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
DECL_ATTR_HANDLER(align);
DECL_ATTR_HANDLER(text);
DECL_ATTR_HANDLER(zoom);
DECL_ATTR_HANDLER(fill);
DECL_ATTR_HANDLER(src);

xml_attr_handle xml_attr_handles[]={
	DECL_ATTR_AXHAND(x,              true),
	DECL_ATTR_AXHAND(y,              true),
	DECL_ATTR_AXHAND(pos,            true),
	DECL_ATTR_AXHAND(size,           true),
	DECL_ATTR_AXHAND(width,          true),
	DECL_ATTR_AXHAND(height,         true),
	DECL_ATTR_PXHAND(top,            true),
	DECL_ATTR_PXHAND(bottom,         true),
	DECL_ATTR_PXHAND(left,           true),
	DECL_ATTR_PXHAND(right,          true),
	DECL_ATTR_ACHAND(top-of,         top_of,        true),
	DECL_ATTR_ACHAND(bottom-of,      bottom_of,     true),
	DECL_ATTR_ACHAND(left-of,        left_of,       true),
	DECL_ATTR_ACHAND(right-of,       right_of,      true),
	DECL_ATTR_ACHAND(max-size,       max_size,      true),
	DECL_ATTR_ACHAND(max-width,      max_width,     true),
	DECL_ATTR_ACHAND(max-height,     max_height,    true),
	DECL_ATTR_ACHAND(min-size,       min_size,      true),
	DECL_ATTR_ACHAND(min-width,      min_width,     true),
	DECL_ATTR_ACHAND(min-height,     min_height,    true),
	DECL_ATTR_ACHAND(cont-size,      cont_size,     true),
	DECL_ATTR_ACHAND(cont-width,     cont_width,    true),
	DECL_ATTR_ACHAND(cont-height,    cont_height,   true),
	DECL_ATTR_ACHAND(content-size,   cont_size,     true),
	DECL_ATTR_ACHAND(content-width,  cont_width,    true),
	DECL_ATTR_ACHAND(content-height, cont_height,   true),
	DECL_ATTR_PCHAND(bind-input,     bind_input,    false),
	DECL_ATTR_ACHAND(flex-flow,      flex_flow,     false),
	DECL_ATTR_ACHAND(flex-grow,      flex_grow,     false),
	DECL_ATTR_ACHAND(flex-align,     flex_align,    false),
	DECL_ATTR_ACHAND(grid-align,     grid_align,    false),
	DECL_ATTR_ACHAND(grid-cell,      grid_cell,     false),
	DECL_ATTR_CHAND(grid-dsc,        grid_dsc,      false),
	DECL_ATTR_CHAND(long-mode,       long_mode,     false),
	DECL_ATTR_CHAND(size-mode,       size_mode,     false),
	DECL_ATTR_XHAND(clickable,       false),
	DECL_ATTR_XHAND(recolor,         false),
	DECL_ATTR_XHAND(checked,         false),
	DECL_ATTR_XHAND(enabled,         false),
	DECL_ATTR_XHAND(visible,         false),
	DECL_ATTR_XHAND(hidden,          false),
	DECL_ATTR_XHAND(oneline,         false),
	DECL_ATTR_XHAND(password,        false),
	DECL_ATTR_XHAND(center,          true),
	DECL_ATTR_XHAND(align,           true),
	DECL_ATTR_XHAND(zoom,            true),
	DECL_ATTR_AXHAND(fill,           true),
	DECL_ATTR_AXHAND(placeholder,    false),
	DECL_ATTR_PXHAND(translate,      false),
	DECL_ATTR_AXHAND(text,           false),
	DECL_ATTR_AXHAND(src,            false),
	{.valid=false,.name="",.resize=false,.hand_pre=NULL,.hand_apply=NULL}
};
#endif
#endif

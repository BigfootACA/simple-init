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

#define DECL_EVENT(_name,_event){.valid=true,.name=(#_name),.event=(_event)}

xml_event_spec xml_event_specs[]={
	DECL_EVENT(all,                 LV_EVENT_ALL),
	DECL_EVENT(pressed,             LV_EVENT_PRESSED),
	DECL_EVENT(pressing,            LV_EVENT_PRESSING),
	DECL_EVENT(press-lost,          LV_EVENT_PRESS_LOST),
	DECL_EVENT(short-clicked,       LV_EVENT_SHORT_CLICKED),
	DECL_EVENT(long-pressed,        LV_EVENT_LONG_PRESSED),
	DECL_EVENT(long-pressed-repeat, LV_EVENT_LONG_PRESSED_REPEAT),
	DECL_EVENT(clicked,             LV_EVENT_CLICKED),
	DECL_EVENT(released,            LV_EVENT_RELEASED),
	DECL_EVENT(drag-begin,          LV_EVENT_DRAG_BEGIN),
	DECL_EVENT(drag-end,            LV_EVENT_DRAG_END),
	DECL_EVENT(drag-throw-begin,    LV_EVENT_DRAG_THROW_BEGIN),
	DECL_EVENT(gesture,             LV_EVENT_GESTURE),
	DECL_EVENT(key,                 LV_EVENT_KEY),
	DECL_EVENT(focused,             LV_EVENT_FOCUSED),
	DECL_EVENT(defocused,           LV_EVENT_DEFOCUSED),
	DECL_EVENT(leave,               LV_EVENT_LEAVE),
	DECL_EVENT(value-changed,       LV_EVENT_VALUE_CHANGED),
	DECL_EVENT(insert,              LV_EVENT_INSERT),
	DECL_EVENT(refresh,             LV_EVENT_REFRESH),
	DECL_EVENT(apply,               LV_EVENT_APPLY),
	DECL_EVENT(cancel,              LV_EVENT_CANCEL),
	DECL_EVENT(delete,              LV_EVENT_DELETE),
	{.valid=false,.name="",.event=0}
};
#endif
#endif
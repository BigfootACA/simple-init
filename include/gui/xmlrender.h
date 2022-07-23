/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _XMLRENDER_H
#define _XMLRENDER_H
#include"gui.h"
#include"gui/activity.h"
#define RENDER_COMPATIBLE_LEVEL   0x00000002
#define ACTIVITY_COMPATIBLE_LEVEL 0x00000002
typedef enum xml_obj_type xml_obj_type;
typedef struct xml_render xml_render;
typedef struct xml_render_obj xml_render_obj;
typedef struct xml_render_code xml_render_code;
typedef struct xml_render_style xml_render_style;
typedef struct xml_render_event xml_render_event;
typedef struct xml_attr_handle xml_attr_handle;
typedef struct xml_obj_handle xml_obj_handle;
typedef void(*xml_render_event_cb)(xml_render_event*event);
enum xml_obj_type{
	OBJ_NONE=0,
	OBJ_OBJ,
	OBJ_VER_BOX,
	OBJ_HOR_BOX,
	OBJ_WRAPPER,
	OBJ_PAGE,
	OBJ_BTN,
	OBJ_BTN_ITEM,
	OBJ_LABEL,
	OBJ_TEXTAREA,
	OBJ_IMG,
	OBJ_CHECKBOX,
	OBJ_DROPDOWN,
	OBJ_ARC,
	OBJ_BAR,
	OBJ_IMGBTN,
	OBJ_BTNMATRIX,
	OBJ_CALENDAR,
	OBJ_CANVAS,
	OBJ_GAUGE,
	OBJ_KEYBOARD,
	OBJ_LED,
	OBJ_LINE,
	OBJ_LIST,
	OBJ_ROLLER,
	OBJ_SLIDER,
	OBJ_SPINBOX,
	OBJ_SWITCH,
	OBJ_SPINNER,
	OBJ_CPICKER,
	OBJ_CHART,
};
#ifdef ASSETS_H
extern bool render_set_content_assets(xml_render*render,entry_file*file);
extern bool xml_assets_file_load_activity(entry_file*file);
extern bool xml_assets_load_all_activity(entry_dir*dir);
extern bool xml_assets_dir_load_activity(entry_dir*dir,const char*path);
extern bool xml_assets_dir_load_all_activity(entry_dir*dir,const char*path);
#endif
extern bool xml_string_load_activity(const char*content);
extern bool xml_sstring_load_activity(const char*content,size_t len);
extern bool xml_rootfs_load_activity(const char*path);
extern bool xml_rootfs_load_all_activity(const char*path);
extern void render_free(xml_render*r);
extern void render_obj_free(xml_render_obj*o);
extern void render_code_free(xml_render_code*o);
extern void render_style_free(xml_render_style*o);
extern xml_render*render_create(void*user_data);
extern bool render_resize(xml_render*render);
extern bool render_parse(xml_render*render,lv_obj_t*root);
extern int render_activity_draw(struct gui_activity*act);
extern int render_activity_resize(struct gui_activity*act);
extern int render_activity_get_focus(struct gui_activity*act);
extern int render_activity_lost_focus(struct gui_activity*act);
extern void*render_get_user_data(xml_render*render);
extern void render_set_user_data(xml_render*render,void*data);
extern const char*render_obj_get_id(xml_render_obj*obj);
extern lv_obj_t*render_obj_get_object(xml_render_obj*obj);
extern xml_obj_type render_obj_get_type(xml_render_obj*obj);
extern void*render_obj_get_user_data(xml_render_obj*obj);
extern void render_obj_set_user_data(xml_render_obj*obj,void*data);
extern const char*render_event_get_event_id(xml_render_event*event);
extern const char*render_event_get_obj_id(xml_render_event*event);
extern lv_obj_t*render_event_get_lvgl_object(xml_render_event*event);
extern xml_render_obj*render_event_get_object(xml_render_event*event);
extern lv_event_t*render_event_get_event(xml_render_event*event);
extern void*render_event_get_initial_data(xml_render_event*event);
extern void*render_event_get_trigger_data(xml_render_event*event);
extern bool render_set_content_sstring(
	xml_render*render,
	const char*content,size_t len
);
extern bool render_set_content_string(
	xml_render*render,
	const char*content
);
extern bool render_set_content_rootfs(
	xml_render*render,
	const char*path
);
extern lv_coord_t render_resolve_coord(
	xml_render_obj*obj,
	lv_coord_t parent,
	const char*name
);
extern xml_render_obj*render_lookup_object(
	xml_render*render,
	const char*name
);
extern xml_render_code*render_lookup_code(
	xml_render*render,
	const char*name
);
extern xml_render_style*render_lookup_style(
	xml_render*render,
	const char*name
);
extern int render_obj_add_event_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_event_cb callback,
	void*data
);
extern int render_add_event_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_event_cb callback,
	void*data
);
extern int render_obj_add_code_event_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_code_t event,
	xml_render_code*code,
	void*data
);
extern int render_obj_del_event_listener(
	xml_render_obj*obj,
	const char*evt_id
);
extern int render_del_event_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id
);
extern int render_obj_trigger_listener(
	xml_render_obj*obj,
	const char*evt_id,
	lv_event_t*event,
	void*data
);
extern int render_trigger_listener(
	xml_render*render,
	const char*obj_id,
	const char*evt_id,
	lv_event_t*event,
	void*data
);
extern int render_obj_trigger_event(
	xml_render_obj*obj,
	lv_event_t*event,
	void*data
);
extern int render_trigger_event(
	xml_render*render,
	const char*obj_id,
	lv_event_t*event,
	void*data
);
#endif

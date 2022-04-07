/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _RENDER_INTERNAL_H
#define _RENDER_INTERNAL_H

#include<stdint.h>
#include"str.h"
#include"list.h"
#include"xlua.h"
#include"lock.h"
#include"mxml.h"
#include"logger.h"
#include"pathnames.h"
#include"gui/xmlrender.h"
#define TAG "xml-render"

#define XML_ASSET_ROOT _PATH_USR"/share/simple-init/gui/"
#define LUA_RENDER "XML UI Render"
#define LUA_RENDER_OBJ "XML UI Render Object"
#define LUA_RENDER_EVENT "XML UI Render Event"
#define LUA_RENDER_EVENT_INFO "XML UI Render Event Info"
#define OBJ_IS_CONTAINER(obj) (\
	obj->type==OBJ_OBJ||\
	obj->type==OBJ_HOR_BOX||\
	obj->type==OBJ_VER_BOX\
)

#define CHECK_TYPE(_obj,_type)\
	if((_obj)->obj->type!=(_type)){\
		tlog_warn(\
			"unsupported attribute %s "\
			"for object type %s",\
			(_obj)->key,#_type\
		);\
		return false;\
	}

typedef enum xml_attr_type xml_attr_type;
typedef enum xml_style_type xml_style_type;
typedef struct xml_label_align xml_label_align;
typedef struct xml_label_long xml_label_long;
typedef struct xml_event_info xml_event_info;
typedef struct xml_style_state xml_style_state;
typedef struct xml_style_prop xml_style_prop;
typedef struct xml_style_part xml_style_part;
typedef struct xml_align_spec xml_align_spec;
typedef struct xml_theme_spec xml_theme_spec;
typedef struct xml_event_spec xml_event_spec;
typedef struct xml_layout_spec xml_layout_spec;
typedef struct xml_render_obj_attr xml_render_obj_attr;
typedef struct xml_attr_handle xml_attr_handle;
typedef struct xml_obj_handle xml_obj_handle;
typedef struct xml_render_doc xml_render_doc;

typedef int(*xml_style_set_type)(
	xml_render_obj*obj,
	lv_style_t*style,
	lv_style_property_t p,
	lv_coord_t max,
	char*val
);

typedef bool(*xml_attr_handler)(
	xml_render_obj_attr*obj
);

typedef bool(*xml_obj_handler)(
	xml_render_obj*obj
);

enum xml_style_type{
	STYLE_NONE=0,
	STYLE_INT,
	STYLE_BOOL,
	STYLE_COLOR,
	STYLE_OPA,
	STYLE_STRING,
	STYLE_FONT,
	STYLE_IMAGE,
	STYLE_ANIM,
	STYLE_BLEND,
	STYLE_GRAD_DIR,
	STYLE_ALIGN,
	STYLE_BORDER_SIDE,
	STYLE_TEXT_DECOR,
	STYLE_LAST,
};

enum xml_attr_type{
	ATTR_NONE=0,
	ATTR_ID,
	ATTR_ATTR,
	ATTR_STYLE,
	ATTR_EVENT,
	ATTR_REF_STYLE,
};

struct xml_render_event{
	xml_event_info*info;
	lv_event_t event;
	void*data;
};

struct xml_event_info{
	char obj_name[
		256-
		sizeof(xml_render_obj*)-
		sizeof(xml_render_event_cb)
	];
	char event_id[
		256-
		sizeof(lv_event_t)-
		sizeof(void*)
	];
	lv_event_t event;
	xml_render_obj*obj;
	xml_render_code*code;
	xml_render_event_cb callback;
	void*data;
};

struct xml_style_prop{
	bool valid;
	xml_style_type type;
	lv_style_property_t prop;
	char name[
		256-
		sizeof(void*)-
		sizeof(lv_coord_t)-
		sizeof(lv_style_property_t)-
		sizeof(xml_style_set_type)
	];
	lv_coord_t max;
	xml_style_set_type hand;
};

struct xml_style_state{
	bool valid;
	char name[
		256-
		sizeof(bool)-
		sizeof(lv_state_t)
	];
	lv_state_t state;
};

struct xml_style_part{
	bool valid;
	char name[
		256-
		sizeof(xml_obj_type)-
		sizeof(bool)-
		sizeof(uint8_t)
	];
	uint8_t part;
	xml_obj_type type;
};

struct xml_align_spec{
	bool valid;
	char name[
		256-
		sizeof(lv_align_t)-
		sizeof(bool)
	];
	lv_align_t align;
};

struct xml_layout_spec{
	bool valid;
	char name[
		256-
		sizeof(lv_layout_t)-
		sizeof(bool)
	];
	lv_layout_t layout;
};

struct xml_event_spec{
	bool valid;
	char name[
		256-
		sizeof(lv_event_t)-
		sizeof(bool)
	];
	lv_event_t event;
};

struct xml_label_align{
	bool valid;
	char name[
		256-
		sizeof(lv_label_align_t)-
		sizeof(bool)
	];
	lv_label_align_t align;
};

struct xml_label_long{
	bool valid;
	char name[
		256-
		sizeof(lv_label_long_mode_t)-
		sizeof(bool)
	];
	lv_label_long_mode_t mode;
};

struct xml_theme_spec{
	bool valid;
	char name[
		256-
		sizeof(lv_theme_style_t)-
		sizeof(bool)
	];
	lv_theme_style_t theme;
};

struct xml_render_obj_attr{
	xml_render_obj*obj;
	xml_attr_handle*hand;
	xml_attr_type type;
	char key[
		256-
		sizeof(xml_render_obj*)-
		sizeof(xml_attr_handle*)-
		sizeof(char*)-
		sizeof(xml_attr_type)-
		sizeof(void*)
	];
	char*value;
	union{
		bool bool_val;
		uint8_t uint8_val;
		uint16_t uint16_val;
		uint32_t uint32_val;
		uint64_t uint64_val;
		lv_align_t align_val;
		lv_layout_t layout_val;
		lv_label_align_t text_align_val;
		lv_label_long_mode_t long_mode_val;
	}fields;
};

struct xml_render_obj{
	char id[256];
	bool auto_width;
	bool auto_height;
	bool bind_input;
	bool clickable;
	bool translate;
	xml_obj_type type;
	xml_obj_handle*hand;
	bool has_style[16];
	lv_style_t style[16];
	lv_obj_t*obj;
	lv_obj_t*cont;
	mxml_node_t*node;
	xml_render_obj*parent;
	xml_render*render;
	lv_coord_t x,y;
	lv_coord_t width;
	lv_coord_t max_width;
	lv_coord_t min_width;
	lv_coord_t height;
	lv_coord_t max_height;
	lv_coord_t min_height;
	list*attrs;
	list*styles;
	list*callbacks;
	void*data;
};

struct xml_render_style{
	char id[256];
	xml_render*render;
	lv_style_t style;
	mxml_node_t*node;
};

struct xml_render_code{
	char id[256];
	xml_render*render;
	mxml_node_t*node;
	char*code;
	size_t len;
};

struct xml_render_doc{
	mxml_node_t*document;
	mxml_node_t*root_node;
};

struct xml_render{
	uint32_t compatible_level;
	bool initialized;
	struct gui_activity*activity;
	char*content;
	size_t length;
	lv_obj_t*root_obj;
	#ifdef ENABLE_LUA
	lua_State*lua;
	#endif
	list*docs;
	list*codes;
	list*styles;
	list*objects;
	list*callbacks;
	mutex_t lock;
	void*data;
};

struct xml_attr_handle{
	bool valid;
	bool resize;
	char name[
		256-
		sizeof(xml_attr_handler)*2-
		sizeof(bool)*2
	];
	xml_attr_handler hand_pre;
	xml_attr_handler hand_apply;
};

struct xml_obj_handle{
	bool valid;
	char name[
		256-
		sizeof(bool)-
		sizeof(xml_obj_type)-
		sizeof(xml_obj_handler)*2
	];
	xml_obj_type type;
	xml_obj_handler pre_hand;
	xml_obj_handler post_hand;
};

struct lua_render_data{xml_render*render;};
struct lua_render_obj_data{xml_render_obj*obj;};
struct lua_render_event_data{xml_render_event*event;};
struct lua_render_event_info_data{xml_event_info*info;};

extern xml_label_long xml_label_longs[];
extern xml_label_align xml_label_aligns[];
extern xml_style_set_type xml_style_set_types[];
extern xml_style_prop xml_style_props[];
extern xml_style_state xml_style_states[];
extern xml_style_part xml_style_parts[];
extern xml_attr_handle xml_attr_handles[];
extern xml_obj_handle xml_obj_handles[];
extern xml_align_spec xml_align_specs[];
extern xml_event_spec xml_event_specs[];
extern xml_theme_spec xml_theme_specs[];
extern xml_layout_spec xml_layout_specs[];

extern void render_to_lua(lua_State*L,xml_render*render);
extern void render_obj_to_lua(lua_State*L,xml_render_obj*obj);
extern void render_event_to_lua(lua_State*L,xml_render_event*event);
extern void render_event_info_to_lua(lua_State*L,xml_event_info*info);

extern bool list_render_obj_attr_cmp(list*l,void*d);
extern bool list_render_obj_cmp(list*l,void*d);
extern bool list_render_style_cmp(list*l,void*d);
extern bool list_render_code_cmp(list*l,void*d);
extern void lv_render_event_cb(lv_obj_t*obj,lv_event_t event);
extern bool xml_attr_apply_style(xml_render_obj_attr*obj);
extern bool render_init_attributes(xml_render_obj*obj,bool resize);
extern void render_expand_parent_width(xml_render_obj*obj);
extern void render_expand_parent_height(xml_render_obj*obj);
extern void render_expand_parent(xml_render_obj*obj);
extern void render_reload_size(xml_render_obj*obj);
extern void render_reload_pos(xml_render_obj*obj);
extern void render_apply_size(xml_render_obj*obj);
extern void render_obj_attr_free(xml_render_obj_attr*o);
extern bool render_move_callbacks(xml_render*render);
extern int render_code_exec_run(xml_render_code*code);
extern int render_lua_init_event(lua_State*L);
extern bool xml_style_apply_style(
	xml_render_style*style,
	const char*k,
	const char*v
);
extern bool render_parse_object(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node
);
extern xml_render_code*render_load_code(
	xml_render*render,
	mxml_node_t*node
);
extern xml_render_obj*render_obj_new(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node
);
extern xml_render_style*render_style_new(
	xml_render*render,
	mxml_node_t*node
);
#endif

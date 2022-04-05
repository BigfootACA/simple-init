/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LUA
#include"gui/lua.h"
#define DECL_FUNC(name)extern int lua_lvgl_##name(lua_State*L)
#define DECL_OBJ_FUNC(name) DECL_FUNC(obj_##name)
DECL_FUNC(get_scr_act);
DECL_OBJ_FUNC(tostring);
DECL_OBJ_FUNC(create);
DECL_OBJ_FUNC(del);
DECL_OBJ_FUNC(del_async);
DECL_OBJ_FUNC(clean);
DECL_OBJ_FUNC(invalidate);
DECL_OBJ_FUNC(move_foreground);
DECL_OBJ_FUNC(move_background);
DECL_OBJ_FUNC(realign);
DECL_OBJ_FUNC(set_parent);
DECL_OBJ_FUNC(get_parent);
DECL_OBJ_FUNC(get_screen);
DECL_OBJ_FUNC(get_focused_obj);
DECL_OBJ_FUNC(x);
DECL_OBJ_FUNC(y);
DECL_OBJ_FUNC(width);
DECL_OBJ_FUNC(width_fit);
DECL_OBJ_FUNC(width_margin);
DECL_OBJ_FUNC(height);
DECL_OBJ_FUNC(height_fit);
DECL_OBJ_FUNC(height_margin);
DECL_OBJ_FUNC(pos);
DECL_OBJ_FUNC(size);
DECL_OBJ_FUNC(set_auto_realign);
DECL_OBJ_FUNC(set_hidden);
DECL_OBJ_FUNC(set_adv_hittest);
DECL_OBJ_FUNC(set_click);
DECL_OBJ_FUNC(set_top);
DECL_OBJ_FUNC(set_drag);
DECL_OBJ_FUNC(set_drag_throw);
DECL_OBJ_FUNC(set_drag_parent);
DECL_OBJ_FUNC(set_focus_parent);
DECL_OBJ_FUNC(set_gesture_parent);
DECL_OBJ_FUNC(set_parent_event);
DECL_OBJ_FUNC(is_visible);
DECL_OBJ_FUNC(get_auto_realign);
DECL_OBJ_FUNC(get_hidden);
DECL_OBJ_FUNC(get_adv_hittest);
DECL_OBJ_FUNC(get_click);
DECL_OBJ_FUNC(get_top);
DECL_OBJ_FUNC(get_drag);
DECL_OBJ_FUNC(get_drag_throw);
DECL_OBJ_FUNC(get_drag_parent);
DECL_OBJ_FUNC(get_focus_parent);
DECL_OBJ_FUNC(get_parent_event);
DECL_OBJ_FUNC(get_gesture_parent);
DECL_OBJ_FUNC(is_focused);

static struct luaL_Reg lua_lvgl_func[]={
	{"add",    lua_lvgl_obj_create},
	{"create", lua_lvgl_obj_create},

	{"del",             lua_lvgl_obj_del},
	{"delete",          lua_lvgl_obj_del},
	{"rm",              lua_lvgl_obj_del},
	{"remove",          lua_lvgl_obj_del},
	{"del_async",       lua_lvgl_obj_del_async},
	{"rm_async",        lua_lvgl_obj_del_async},
	{"delete_async",    lua_lvgl_obj_del_async},
	{"remove_async",    lua_lvgl_obj_del_async},
	{"inv",             lua_lvgl_obj_invalidate},
	{"invalidate",      lua_lvgl_obj_invalidate},
	{"realign",         lua_lvgl_obj_realign},
	{"move_fg",         lua_lvgl_obj_move_foreground},
	{"move_foreground", lua_lvgl_obj_move_foreground},
	{"move_bg",         lua_lvgl_obj_move_background},
	{"move_background", lua_lvgl_obj_move_background},

	{"set_parent",      lua_lvgl_obj_set_parent},
	{"get_parent",      lua_lvgl_obj_get_parent},
	{"get_screen",      lua_lvgl_obj_get_screen},
	{"get_focused",     lua_lvgl_obj_get_focused_obj},
	{"get_focused_obj", lua_lvgl_obj_get_focused_obj},

	// object position
	{"pos",          lua_lvgl_obj_pos},
	{"get_pos",      lua_lvgl_obj_pos},
	{"set_pos",      lua_lvgl_obj_pos},
	{"position",     lua_lvgl_obj_pos},
	{"get_position", lua_lvgl_obj_pos},
	{"set_position", lua_lvgl_obj_pos},
	{"x",            lua_lvgl_obj_x},
	{"get_x",        lua_lvgl_obj_x},
	{"set_x",        lua_lvgl_obj_x},
	{"y",            lua_lvgl_obj_y},
	{"get_y",        lua_lvgl_obj_y},
	{"set_y",        lua_lvgl_obj_y},

	// object size
	{"size",              lua_lvgl_obj_size},
	{"get_size",          lua_lvgl_obj_size},
	{"set_size",          lua_lvgl_obj_size},
	{"w",                 lua_lvgl_obj_width},
	{"width",             lua_lvgl_obj_width},
	{"get_width",         lua_lvgl_obj_width},
	{"set_width",         lua_lvgl_obj_width},
	{"width_fit",         lua_lvgl_obj_width_fit},
	{"get_width_fit",     lua_lvgl_obj_width_fit},
	{"set_width_fit",     lua_lvgl_obj_width_fit},
	{"width_margin",      lua_lvgl_obj_width_margin},
	{"get_width_margin",  lua_lvgl_obj_width_margin},
	{"set_width_margin",  lua_lvgl_obj_width_margin},
	{"h",                 lua_lvgl_obj_height},
	{"height",            lua_lvgl_obj_height},
	{"get_height",        lua_lvgl_obj_height},
	{"set_height",        lua_lvgl_obj_height},
	{"height_fit",        lua_lvgl_obj_height_fit},
	{"get_height_fit",    lua_lvgl_obj_height_fit},
	{"set_height_fit",    lua_lvgl_obj_height_fit},
	{"height_margin",     lua_lvgl_obj_height_margin},
	{"get_height_margin", lua_lvgl_obj_height_margin},
	{"set_height_margin", lua_lvgl_obj_height_margin},

	// object boolean values
	{"set_auto_realign",   lua_lvgl_obj_set_auto_realign},
	{"set_hidden",         lua_lvgl_obj_set_hidden},
	{"set_adv_hittest",    lua_lvgl_obj_set_adv_hittest},
	{"set_click",          lua_lvgl_obj_set_click},
	{"set_top",            lua_lvgl_obj_set_top},
	{"set_drag",           lua_lvgl_obj_set_drag},
	{"set_drag_throw",     lua_lvgl_obj_set_drag_throw},
	{"set_drag_parent",    lua_lvgl_obj_set_drag_parent},
	{"set_focus_parent",   lua_lvgl_obj_set_focus_parent},
	{"set_gesture_parent", lua_lvgl_obj_set_gesture_parent},
	{"set_parent_event",   lua_lvgl_obj_set_parent_event},
	{"is_visible",         lua_lvgl_obj_is_visible},
	{"get_auto_realign",   lua_lvgl_obj_get_auto_realign},
	{"get_hidden",         lua_lvgl_obj_get_hidden},
	{"get_adv_hittest",    lua_lvgl_obj_get_adv_hittest},
	{"get_click",          lua_lvgl_obj_get_click},
	{"get_top",            lua_lvgl_obj_get_top},
	{"get_drag",           lua_lvgl_obj_get_drag},
	{"get_drag_throw",     lua_lvgl_obj_get_drag_throw},
	{"get_drag_parent",    lua_lvgl_obj_get_drag_parent},
	{"get_focus_parent",   lua_lvgl_obj_get_focus_parent},
	{"get_parent_event",   lua_lvgl_obj_get_parent_event},
	{"get_gesture_parent", lua_lvgl_obj_get_gesture_parent},
	{"is_focused",         lua_lvgl_obj_is_focused},
	{NULL, NULL},
};

static struct luaL_Reg lvgl_lib[]={
	{"get_active_screen", lua_lvgl_get_scr_act},
	{"get_act_scr",       lua_lvgl_get_scr_act},
	{"scr_act",           lua_lvgl_get_scr_act},
	{NULL, NULL},
};

LUAMOD_API int luaopen_lvgl(lua_State*L){
	xlua_create_metatable(L,LUA_LVGL_OBJ,lua_lvgl_func,lua_lvgl_obj_tostring,NULL);
	luaL_newlib(L,lvgl_lib);
	return 1;
}

#endif
#endif

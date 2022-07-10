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
DECL_OBJ_FUNC(set_parent);
DECL_OBJ_FUNC(get_parent);
DECL_OBJ_FUNC(get_screen);
DECL_OBJ_FUNC(x);
DECL_OBJ_FUNC(y);
DECL_OBJ_FUNC(get_x2);
DECL_OBJ_FUNC(get_y2);
DECL_OBJ_FUNC(get_x_aligned);
DECL_OBJ_FUNC(get_y_aligned);
DECL_OBJ_FUNC(width);
DECL_OBJ_FUNC(height);
DECL_OBJ_FUNC(content_width);
DECL_OBJ_FUNC(content_height);
DECL_OBJ_FUNC(get_self_width);
DECL_OBJ_FUNC(get_self_height);
DECL_OBJ_FUNC(pos);
DECL_OBJ_FUNC(size);
DECL_OBJ_FUNC(is_visible);
DECL_OBJ_FUNC(center);
DECL_OBJ_FUNC(refr_pos);
DECL_OBJ_FUNC(refr_size);
DECL_OBJ_FUNC(is_layout_positioned);
DECL_OBJ_FUNC(refresh_self_size);
DECL_OBJ_FUNC(mark_layout_as_dirty);
DECL_OBJ_FUNC(update_layout);

static struct luaL_Reg lua_lvgl_func[]={
	{"add",    lua_lvgl_obj_create},
	{"create", lua_lvgl_obj_create},

	{"del",                    lua_lvgl_obj_del},
	{"delete",                 lua_lvgl_obj_del},
	{"rm",                     lua_lvgl_obj_del},
	{"remove",                 lua_lvgl_obj_del},
	{"del_async",              lua_lvgl_obj_del_async},
	{"rm_async",               lua_lvgl_obj_del_async},
	{"delete_async",           lua_lvgl_obj_del_async},
	{"remove_async",           lua_lvgl_obj_del_async},
	{"inv",                    lua_lvgl_obj_invalidate},
	{"invalidate",             lua_lvgl_obj_invalidate},
	{"center",                 lua_lvgl_obj_center},
	{"move_fg",                lua_lvgl_obj_move_foreground},
	{"move_foreground",        lua_lvgl_obj_move_foreground},
	{"move_bg",                lua_lvgl_obj_move_background},
	{"move_background",        lua_lvgl_obj_move_background},
	{"mark_layout_as_dirty",   lua_lvgl_obj_mark_layout_as_dirty},
	{"update_layout",          lua_lvgl_obj_update_layout},
	{"refr_pos",               lua_lvgl_obj_refr_pos},

	{"set_parent",      lua_lvgl_obj_set_parent},
	{"get_parent",      lua_lvgl_obj_get_parent},
	{"get_screen",      lua_lvgl_obj_get_screen},

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
	{"x2",           lua_lvgl_obj_get_x2},
	{"get_x2",       lua_lvgl_obj_get_x2},
	{"y2",           lua_lvgl_obj_get_y2},
	{"get_y2",       lua_lvgl_obj_get_y2},

	// object size
	{"size",                lua_lvgl_obj_size},
	{"get_size",            lua_lvgl_obj_size},
	{"set_size",            lua_lvgl_obj_size},
	{"w",                   lua_lvgl_obj_width},
	{"width",               lua_lvgl_obj_width},
	{"get_width",           lua_lvgl_obj_width},
	{"set_width",           lua_lvgl_obj_width},
	{"content_width",       lua_lvgl_obj_content_width},
	{"get_content_width",   lua_lvgl_obj_content_width},
	{"set_content_width",   lua_lvgl_obj_content_width},
	{"cont_width",          lua_lvgl_obj_content_width},
	{"get_cont_width",      lua_lvgl_obj_content_width},
	{"set_cont_width",      lua_lvgl_obj_content_width},
	{"self_width",          lua_lvgl_obj_get_self_width},
	{"get_self_width",      lua_lvgl_obj_get_self_width},
	{"h",                   lua_lvgl_obj_height},
	{"height",              lua_lvgl_obj_height},
	{"get_height",          lua_lvgl_obj_height},
	{"set_height",          lua_lvgl_obj_height},
	{"content_height",      lua_lvgl_obj_content_height},
	{"get_content_height",  lua_lvgl_obj_content_height},
	{"set_content_height",  lua_lvgl_obj_content_height},
	{"cont_height",         lua_lvgl_obj_content_height},
	{"get_cont_height",     lua_lvgl_obj_content_height},
	{"set_cont_height",     lua_lvgl_obj_content_height},
	{"self_height",         lua_lvgl_obj_get_self_height},
	{"get_self_height",     lua_lvgl_obj_get_self_height},

	// object boolean values
	{"refr_size",            lua_lvgl_obj_refr_size},
	{"is_layout_positioned", lua_lvgl_obj_is_layout_positioned},
	{"refresh_self_size",    lua_lvgl_obj_refresh_self_size},
	{"is_visible",           lua_lvgl_obj_is_visible},
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

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

static int lua_activity_get_last(lua_State*L){
	struct gui_activity*act=guiact_get_last();
	if(!act)lua_pushnil(L);
	else guiact_to_lua(L,act);
	return 1;
}

static int lua_activity_back(lua_State*L){
	lua_pushinteger(L,guiact_do_back());
	return 1;
}

static int lua_activity_home(lua_State*L){
	lua_pushinteger(L,guiact_do_home());
	return 1;
}

static int lua_activity_start(lua_State*L){
	char*name=(char*)luaL_checkstring(L,1);
	char*arg=(char*)luaL_optstring(L,2,NULL);
	lua_pushinteger(L,guiact_start_activity_by_name(name,arg));
	return 1;
}

static int lua_activity_is_active_name(lua_State*L){
	lua_pushboolean(L,guiact_is_active_name(luaL_checkstring(L,1)));
	return 1;
}

static int lua_activity_is_active_page(lua_State*L){
	struct lua_lvgl_obj_data*o=luaL_checkudata(L,1,LUA_LVGL_OBJ);
	lua_pushboolean(L,guiact_is_active_page(o->obj));
	return 1;
}

static int lua_activity_has_name(lua_State*L){
	lua_pushboolean(L,guiact_has_activity_name(luaL_checkstring(L,1)));
	return 1;
}

static int lua_activity_has_page(lua_State*L){
	struct lua_lvgl_obj_data*o=luaL_checkudata(L,1,LUA_LVGL_OBJ);
	lua_pushboolean(L,guiact_has_activity_page(o->obj));
	return 1;
}

static int lua_activity_act_name(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushstring(L,act->act->name);
	return 1;
}

static int lua_activity_act_title(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushstring(L,act->act->reg->title);
	return 1;
}

static int lua_activity_act_icon(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushstring(L,act->act->reg->icon);
	return 1;
}

static int lua_activity_act_args(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushstring(L,act->act->args);
	return 1;
}

static int lua_activity_act_mask(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushboolean(L,act->act->mask);
	return 1;
}

static int lua_activity_act_changed(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	if(!lua_isnoneornil(L,2)){
		luaL_checktype(L,2,LUA_TBOOLEAN);
		act->act->data_changed=lua_toboolean(L,2);
	}
	lua_pushboolean(L,act->act->data_changed);
	return 1;
}

static int lua_activity_act_page(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lvgl_obj_to_lua(L,act->act->page);
	return 1;
}

static int lua_activity_act_width(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushinteger(L,act->act->w);
	return 1;
}

static int lua_activity_act_height(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushinteger(L,act->act->h);
	return 1;
}

static int lua_activity_act_reg(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	guireg_to_lua(L,act->act->reg);
	return 1;
}

static int lua_activity_act_tostring(lua_State*L){
	struct lua_gui_activity_data*act=luaL_checkudata(L,1,LUA_GUI_ACTIVITY);
	lua_pushfstring(L,"GUI Activity %s (%p)",act->act->name,act->act);
	return 1;
}

static int lua_activity_reg_name(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushstring(L,reg->reg->name);
	return 1;
}

static int lua_activity_reg_title(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushstring(L,reg->reg->title);
	return 1;
}

static int lua_activity_reg_icon(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushstring(L,reg->reg->icon);
	return 1;
}

static int lua_activity_reg_mask(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushboolean(L,reg->reg->mask);
	return 1;
}

static int lua_activity_reg_show_app(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushboolean(L,reg->reg->show_app);
	return 1;
}

static int lua_activity_reg_open_file(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushboolean(L,reg->reg->open_file);
	return 1;
}

static int lua_activity_reg_fullscreen(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushboolean(L,reg->reg->full_screen);
	return 1;
}

static int lua_activity_reg_tostring(lua_State*L){
	struct lua_gui_register_data*reg=luaL_checkudata(L,1,LUA_GUI_REGISTER);
	lua_pushfstring(L,"GUI Activity Register %s (%p)",reg->reg->name,reg->reg);
	return 1;
}

void guiact_to_lua(lua_State*L,struct gui_activity*act){
	struct lua_gui_activity_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_gui_activity_data));
	luaL_getmetatable(L,LUA_GUI_ACTIVITY);
	lua_setmetatable(L,-2);
	e->act=act;
}

void guireg_to_lua(lua_State*L,struct gui_register*reg){
	struct lua_gui_register_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_gui_register_data));
	luaL_getmetatable(L,LUA_GUI_ACTIVITY);
	lua_setmetatable(L,-2);
	e->reg=reg;
}

static struct luaL_Reg lua_activity_func[]={
	{"name",         lua_activity_act_name},
	{"get_name",     lua_activity_act_name},
	{"title",        lua_activity_act_title},
	{"get_title",    lua_activity_act_title},
	{"icon",         lua_activity_act_icon},
	{"get_icon",     lua_activity_act_icon},
	{"page",         lua_activity_act_page},
	{"get_page",     lua_activity_act_page},
	{"args",         lua_activity_act_args},
	{"get_args",     lua_activity_act_args},
	{"mask",         lua_activity_act_mask},
	{"is_mask",      lua_activity_act_mask},
	{"changed",      lua_activity_act_changed},
	{"is_changed",   lua_activity_act_changed},
	{"set_changed",  lua_activity_act_changed},
	{"w",            lua_activity_act_width},
	{"width",        lua_activity_act_width},
	{"get_width",    lua_activity_act_width},
	{"h",            lua_activity_act_height},
	{"height",       lua_activity_act_height},
	{"get_height",   lua_activity_act_height},
	{"reg",          lua_activity_act_reg},
	{"get_reg",      lua_activity_act_reg},
	{"get_register", lua_activity_act_reg},
	{NULL, NULL},
};

static struct luaL_Reg lua_register_func[]={
	{"name",          lua_activity_reg_name},
	{"get_name",      lua_activity_reg_name},
	{"title",         lua_activity_reg_title},
	{"get_title",     lua_activity_reg_title},
	{"icon",          lua_activity_reg_icon},
	{"get_icon",      lua_activity_reg_icon},
	{"mask",          lua_activity_reg_mask},
	{"is_mask",       lua_activity_reg_mask},
	{"show_app",      lua_activity_reg_show_app},
	{"is_show_app",   lua_activity_reg_show_app},
	{"open_file",     lua_activity_reg_open_file},
	{"is_open_file",  lua_activity_reg_open_file},
	{"fullscreen",    lua_activity_reg_fullscreen},
	{"is_fullscreen", lua_activity_reg_fullscreen},
	{NULL, NULL},
};

static struct luaL_Reg activity_lib[]={
	{"current",        lua_activity_get_last},
	{"get_last",       lua_activity_get_last},
	{"back",           lua_activity_back},
	{"home",           lua_activity_home},
	{"start",          lua_activity_start},
	{"is_active_name", lua_activity_is_active_name},
	{"is_active_page", lua_activity_is_active_page},
	{"has_name",       lua_activity_has_name},
	{"has_page",       lua_activity_has_page},
	{NULL, NULL},
};

LUAMOD_API int luaopen_activity(lua_State*L){
	xlua_create_metatable(L,LUA_GUI_ACTIVITY,lua_activity_func,lua_activity_act_tostring,NULL);
	xlua_create_metatable(L,LUA_GUI_REGISTER,lua_register_func,lua_activity_reg_tostring,NULL);
	luaL_newlib(L,activity_lib);
	return 1;
}
#endif
#endif

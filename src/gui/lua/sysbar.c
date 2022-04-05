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

static int lua_sysbar_keyboard(lua_State*L){
	if(!lua_isnoneornil(L,1)){
		luaL_checktype(L,1,LUA_TBOOLEAN);
		if(lua_toboolean(L,1))sysbar_keyboard_open();
		else sysbar_keyboard_close();
	}
	lua_pushboolean(L,sysbar.keyboard!=NULL);
	return 1;
}

static int lua_sysbar_edit_menu(lua_State*L){
	if(!lua_isnoneornil(L,1)){
		luaL_checktype(L,1,LUA_TBOOLEAN);
		if(lua_toboolean(L,1))sysbar_edit_menu_show();
		else sysbar_edit_menu_hide();
	}
	lua_pushboolean(L,lv_obj_is_visible(sysbar.edit_menu));
	return 1;
}

static int lua_sysbar_fullscreen(lua_State*L){
	if(!lua_isnoneornil(L,1)){
		luaL_checktype(L,1,LUA_TBOOLEAN);
		sysbar_set_full_screen(lua_toboolean(L,1));
	}
	lua_pushboolean(L,sysbar.full_screen);
	return 1;
}

static int lua_sysbar_ctrl_pad(lua_State*L){
	if(!lua_isnoneornil(L,1)){
		luaL_checktype(L,1,LUA_TBOOLEAN);
		if(lua_toboolean(L,1))ctrl_pad_show();
		else ctrl_pad_hide();
	}
	lua_pushboolean(L,ctrl_pad_is_show());
	return 1;
}

static int lua_sysbar_show_keyboard(lua_State*L){
	sysbar_keyboard_open();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_show_edit_menu(lua_State*L){
	sysbar_edit_menu_show();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_show_ctrl_pad(lua_State*L){
	ctrl_pad_show();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_hide_keyboard(lua_State*L){
	sysbar_keyboard_close();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_hide_edit_menu(lua_State*L){
	sysbar_edit_menu_hide();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_hide_ctrl_pad(lua_State*L){
	ctrl_pad_hide();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_toggle_keyboard(lua_State*L){
	sysbar_keyboard_toggle();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_toggle_edit_menu(lua_State*L){
	if(lv_obj_is_visible(sysbar.edit_menu))sysbar_edit_menu_hide();
	else sysbar_edit_menu_show();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_toggle_ctrl_pad(lua_State*L){
	ctrl_pad_toggle();
	lua_pushboolean(L,true);
	return 1;
}

static int lua_sysbar_focus_input(lua_State*L){
	struct lua_lvgl_obj_data*obj=luaL_checkudata(L,1,LUA_LVGL_OBJ);
	sysbar_focus_input(obj->obj);
	return 1;
}

static struct luaL_Reg sysbar_lib[]={
	{"set_show_keyboard",    lua_sysbar_keyboard},
	{"set_show_edit_menu",   lua_sysbar_edit_menu},
	{"set_show_fullscreen",  lua_sysbar_fullscreen},
	{"set_show_ctrl_pad",    lua_sysbar_ctrl_pad},
	{"is_show_keyboard",     lua_sysbar_keyboard},
	{"is_show_edit_menu",    lua_sysbar_edit_menu},
	{"is_show_fullscreen",   lua_sysbar_fullscreen},
	{"is_show_ctrl_pad",     lua_sysbar_ctrl_pad},
	{"show_keyboard",        lua_sysbar_show_keyboard},
	{"show_edit_menu",       lua_sysbar_show_edit_menu},
	{"show_ctrl_pad",        lua_sysbar_show_ctrl_pad},
	{"hide_keyboard",        lua_sysbar_hide_keyboard},
	{"hide_edit_menu",       lua_sysbar_hide_edit_menu},
	{"hide_ctrl_pad",        lua_sysbar_hide_ctrl_pad},
	{"toggle_keyboard",      lua_sysbar_toggle_keyboard},
	{"toggle_edit_menu",     lua_sysbar_toggle_edit_menu},
	{"toggle_ctrl_pad",      lua_sysbar_toggle_ctrl_pad},
	{"focus_input",          lua_sysbar_focus_input},
	{NULL, NULL},
};

LUAMOD_API int luaopen_sysbar(lua_State*L){
	luaL_newlib(L,sysbar_lib);
	return 1;
}
#endif
#endif

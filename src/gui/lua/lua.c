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

void lua_gui_init(lua_State*L){
	lua_pushinteger(L,gui_dpi);
	lua_setglobal(L,"gui_dpi");
	lua_pushinteger(L,gui_dpi_force);
	lua_setglobal(L,"gui_dpi_force");
	lua_pushinteger(L,gui_dpi_def);
	lua_setglobal(L,"gui_dpi_def");
	lua_pushinteger(L,gui_font_size);
	lua_setglobal(L,"gui_font_size");
	lua_pushinteger(L,gui_w);
	lua_setglobal(L,"gui_w");
	lua_pushinteger(L,gui_h);
	lua_setglobal(L,"gui_h");
	lua_pushinteger(L,gui_sw);
	lua_setglobal(L,"gui_sw");
	lua_pushinteger(L,gui_sh);
	lua_setglobal(L,"gui_sh");
	lua_pushinteger(L,gui_sx);
	lua_setglobal(L,"gui_sx");
	lua_pushinteger(L,gui_sy);
	lua_setglobal(L,"gui_sy");
	lua_pushboolean(L,gui_run);
	lua_setglobal(L,"gui_run");
	lua_pushboolean(L,gui_dark);
	lua_setglobal(L,"gui_dark");
	#ifndef ENABLE_UEFI
	lua_pushboolean(L,gui_sleep);
	lua_setglobal(L,"gui_sleep");
	#endif
}

#endif
#endif

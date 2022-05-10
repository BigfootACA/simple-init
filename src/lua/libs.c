/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"xlua.h"

const luaL_Reg lua_core_libs[]={
	{LUA_GNAME,       luaopen_base},
	{LUA_LOADLIBNAME, luaopen_package},
	{LUA_COLIBNAME,   luaopen_coroutine},
	{LUA_TABLIBNAME,  luaopen_table},
	{LUA_IOLIBNAME,   luaopen_io},
	{LUA_OSLIBNAME,   luaopen_os},
	{LUA_STRLIBNAME,  luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_UTF8LIBNAME, luaopen_utf8},
	{LUA_DBLIBNAME,   luaopen_debug},
	{NULL, NULL}
};

const luaL_Reg simple_init_lua_libs[]={
	{"data",     luaopen_data},
	{"conf",     luaopen_conf},
	{"logger",   luaopen_logger},
	#ifdef ENABLE_GUI
	{"lvgl",     luaopen_lvgl},
	{"sysbar",   luaopen_sysbar},
	{"activity", luaopen_activity},
	#ifdef ENABLE_MXML
	{"render",   luaopen_xml_render},
	#endif
	#endif
	#ifdef ENABLE_UEFI
	{"uefi",     luaopen_uefi},
	#else
	{"init",     luaopen_init},
	#endif
	{NULL,NULL}
};

const luaL_Reg simple_init_lua_regs[]={
	{"feature", lua_feature},
	{NULL,NULL}
};
#endif

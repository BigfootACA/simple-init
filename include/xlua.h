/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _XLUA_H
#define _XLUA_H
#include"lua.h"
#include"lualib.h"
#include"lauxlib.h"

extern const luaL_Reg lua_core_libs[];
extern const luaL_Reg simple_init_lua_libs[];
extern const luaL_Reg simple_init_lua_regs[];
LUAMOD_API int luaopen_conf(lua_State*L);
LUAMOD_API int luaopen_init(lua_State*L);
LUAMOD_API int luaopen_uefi(lua_State*L);
LUAMOD_API int luaopen_lvgl(lua_State*L);
LUAMOD_API int luaopen_logger(lua_State*L);
LUAMOD_API int luaopen_sysbar(lua_State*L);
LUAMOD_API int luaopen_activity(lua_State*L);
LUAMOD_API int luaopen_xml_render(lua_State*L);
LUAMOD_API int lua_feature(lua_State*L);
extern lua_State*xlua_init();
extern lua_State*xlua_math();
extern int xlua_eval_string(lua_State*L,const char*expr);
extern int xlua_return_string(lua_State*L,const char*expr);
extern void xlua_show_error(lua_State*L,char*tag);
extern int xlua_loadfile(lua_State*L,const char*name);
extern int xlua_run(lua_State*L,char*tag,const char*name);
extern void xlua_run_confd(lua_State*L,char*tag,const char*key,...);
extern int xlua_create_metatable(
	lua_State*L,
	const char*name,
	const luaL_Reg*funcs,
	lua_CFunction tostring,
	lua_CFunction gc
);
#endif

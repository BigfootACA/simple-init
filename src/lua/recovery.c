/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"xlua.h"
#include"recovery.h"

static int lua_recovery_ui_print(lua_State*L){
	recovery_ui_print(luaL_checkstring(L,1));
	return 0;
}

static int lua_recovery_set_progress(lua_State*L){
	recovery_set_progress(luaL_checknumber(L,1));
	return 0;
}

static int lua_recovery_progress(lua_State*L){
	recovery_progress(
		luaL_checknumber(L,1),
		luaL_checkinteger(L,2)
	);
	return 0;
}

static int lua_recovery_log(lua_State*L){
	recovery_log(luaL_checkstring(L,1));
	return 0;
}

static int lua_recovery_clear_display(lua_State*L){
	recovery_clear_display();
	return 0;
}

static int lua_recovery_ui_printf(lua_State*L){
	lua_getglobal(L,"string");
	lua_getfield(L,-1,"format");
	lua_insert(L,1);
	lua_pop(L,1);
	lua_pushvalue(L,lua_upvalueindex(1));
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-2,LUA_MULTRET);
	recovery_ui_print(lua_tostring(L,-1));
	return 0;
}

static int lua_recovery_logf(lua_State*L){
	lua_getglobal(L,"string");
	lua_getfield(L,-1,"format");
	lua_insert(L,1);
	lua_pop(L,1);
	lua_pushvalue(L,lua_upvalueindex(1));
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-2,LUA_MULTRET);
	recovery_log(lua_tostring(L,-1));
	return 0;

}

static luaL_Reg recovery_lib[]={
	{"ui_print",      lua_recovery_ui_print},
	{"set_progress",  lua_recovery_set_progress},
	{"progress",      lua_recovery_progress},
	{"log",           lua_recovery_log},
	{"clear_display", lua_recovery_clear_display},
	{"ui_printf",     lua_recovery_ui_printf},
	{"logf",          lua_recovery_logf},
	{NULL, NULL}
};

LUAMOD_API int luaopen_recovery(lua_State*L){
	if(recovery_out_fd>=0)luaL_newlib(L,recovery_lib);
	return 1;
}
#endif

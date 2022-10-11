/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#ifdef ENABLE_UEFI
#include"uefi/lua_uefi.h"
#include"locate.h"

static int lua_locate_find_name(lua_State*L){
	char buf[256];
	ZeroMem(buf,sizeof(buf));
	char*r=locate_find_name(buf,sizeof(buf));
	if(!r)lua_pushnil(L);
	else lua_pushstring(L,r);
	return 1;
}

static int lua_locate_get_handle_by_tag(lua_State*L){
	const char*tag=luaL_checkstring(L,1);
	EFI_HANDLE*hand=locate_get_handle_by_tag(tag);
	uefi_handle_to_lua(L,hand);
	return 1;
}

static luaL_Reg locate_lib[]={
	{"get_handle_by_tag", lua_locate_get_handle_by_tag},
	{"find_name",         lua_locate_find_name},
	{NULL, NULL}
};

LUAMOD_API int luaopen_locate(lua_State*L){
	luaL_newlib(L,locate_lib);
	return 1;
}
#endif
#endif

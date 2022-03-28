/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiHandToString(lua_State*L){
	GET_HANDLE(L,1,hand);
	lua_pushfstring(L,LUA_UEFI_HANDLE" %p",hand->hand);
	return 1;
}

static int LuaUefiHandToDevicePath(lua_State*L){
	GET_HANDLE(L,2,hand);
	uefi_device_path_to_lua(L,DevicePathFromHandle(hand->hand));
	return 1;
}

static int LuaUefiHandToFileDevicePath(lua_State*L){
	GET_HANDLE(L,2,hand);
	const char*str=luaL_checkstring(L,2);
	CHAR16*path=NULL;
	UINTN paths=AsciiStrLen(str)*sizeof(CHAR16);
	if(!(path=AllocateZeroPool(paths+sizeof(CHAR16))))goto fail;
	AsciiStrToUnicodeStrS(str,path,paths);
	uefi_device_path_to_lua(L,FileDevicePath(hand->hand,path));
	done:
	if(path)FreePool(path);
	return 1;
	fail:lua_pushnil(L);goto done;
}

void uefi_handle_to_lua(lua_State*L,EFI_HANDLE hand){
	struct lua_uefi_hand_data*e;
	if(!hand){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_hand_data));
	luaL_getmetatable(L,LUA_UEFI_HANDLE);
	lua_setmetatable(L,-2);
	e->hand=hand;
}

struct lua_uefi_meta_table LuaUefiHandMetaTable={
	.name=LUA_UEFI_HANDLE,
	.reg=(const luaL_Reg[]){
		{"ToDevicePath",     LuaUefiHandToDevicePath},
		{"ToFileDevicePath", LuaUefiHandToFileDevicePath},
		{NULL, NULL}
	},
	.tostring=LuaUefiHandToString,
	.gc=NULL,
};

#endif

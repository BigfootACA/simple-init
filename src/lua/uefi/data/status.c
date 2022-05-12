/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiStatusToString(lua_State*L){
	GET_STATUS(L,1,st);
	lua_pushstring(L,efi_status_to_string(st->st));
	return 1;
}

static int LuaUefiStatusToShortString(lua_State*L){
	GET_STATUS(L,1,st);
	lua_pushstring(L,efi_status_to_short_string(st->st));
	return 1;
}

static int LuaUefiStatusToErrno(lua_State*L){
	GET_STATUS(L,1,st);
	lua_pushinteger(L,efi_status_to_errno(st->st));
	return 1;
}

static int LuaUefiStatusToInteger(lua_State*L){
	GET_STATUS(L,1,st);
	lua_pushinteger(L,st->st);
	return 1;
}

static int LuaUefiStatusIsError(lua_State*L){
	GET_STATUS(L,1,st);
	lua_pushboolean(L,EFI_ERROR(st->st));
	return 1;
}

void uefi_str_status_to_lua(lua_State*L,const char*st){
	EFI_STATUS status=0;
	struct lua_uefi_status_data*e;
	if(!efi_short_string_to_status(st,&status)){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_status_data));
	luaL_getmetatable(L,LUA_UEFI_STATUS);
	lua_setmetatable(L,-2);
	e->st=status;
}

void uefi_status_to_lua(lua_State*L,EFI_STATUS st){
	uefi_lua_check_status(L,st);
	struct lua_uefi_status_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_uefi_status_data));
	luaL_getmetatable(L,LUA_UEFI_STATUS);
	lua_setmetatable(L,-2);
	e->st=st;
}

struct lua_uefi_meta_table LuaUefiStatusMetaTable={
	.name=LUA_UEFI_STATUS,
	.reg=(const luaL_Reg[]){
		{"ToShortString", LuaUefiStatusToShortString},
		{"ToString",      LuaUefiStatusToString},
		{"ToInteger",     LuaUefiStatusToInteger},
		{"ToErrno",       LuaUefiStatusToErrno},
		{"IsError",       LuaUefiStatusIsError},
		{NULL, NULL}
	},
	.tostring=LuaUefiStatusToString,
	.gc=NULL,
};

#endif
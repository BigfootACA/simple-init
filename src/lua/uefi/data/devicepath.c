/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiDevicePathToString(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	CHAR8*dpt8=NULL;
	CHAR16*dpt16=NULL;
	if(!dp->dp)goto fail;
	if(!(dpt16=ConvertDevicePathToText(dp->dp,FALSE,FALSE)))goto fail;
	UINTN dpt8s=StrLen(dpt16)*sizeof(CHAR8);
	if(!(dpt8=AllocateZeroPool(dpt8s+1)))goto fail;
	if(EFI_ERROR(UnicodeStrToAsciiStrS(dpt16,dpt8,dpt8s)))goto fail;
	lua_pushstring(L,dpt8);
	done:
	if(dpt16)FreePool(dpt16);
	if(dpt8)FreePool(dpt8);
	return 1;
	fail:lua_pushnil(L);goto done;
}

static int LuaUefiDevicePathToChar16(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	if(!dp->dp)goto fail;
	CHAR16*dpt16=ConvertDevicePathToText(dp->dp,FALSE,FALSE);
	if(!dpt16)goto fail;
	uefi_char16_16_to_lua(L,TRUE,dpt16);
	return 1;
	fail:
	lua_pushnil(L);
	return 1;
}

static int LuaUefiDevicePathFromString(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	const char*str=luaL_checkstring(L,2);
	CHAR16*dpt=NULL;
	UINTN dpts=AsciiStrLen(str)*sizeof(CHAR16);
	EFI_DEVICE_PATH_PROTOCOL*dpx=NULL;
	if(!(dpt=AllocateZeroPool(dpts+sizeof(CHAR16))))goto done;
	AsciiStrToUnicodeStrS(str,dpt,dpts);
	if((dpx=ConvertTextToDevicePath(dpt)))dp->dp=dpx;
	done:
	lua_pushboolean(L,dpx!=NULL);
	if(dpt)FreePool(dpt);
	return 1;
}

static int LuaUefiDevicePathFromChar16(lua_State*L){
	CHAR16*c16=NULL;
	GET_DEVICE_PATH(L,1,dp);
	lua_arg_get_char16(L,2,false,&c16);
	if(!c16)return luaL_argerror(L,2,"get argument failed");
	EFI_DEVICE_PATH_PROTOCOL*dpx=NULL;
	if((dpx=ConvertTextToDevicePath(c16)))dp->dp=dpx;
	lua_pushboolean(L,dpx!=NULL);
	FreePool(c16);
	return 1;
}

static int LuaUefiDevicePathFromHandle(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	GET_HANDLE(L,2,hand);
	EFI_DEVICE_PATH_PROTOCOL*dpx=NULL;
	if((dpx=DevicePathFromHandle(hand->hand)))dp->dp=dpx;
	lua_pushboolean(L,dpx!=NULL);
	return 1;
}

void uefi_device_path_to_lua(lua_State*L,EFI_DEVICE_PATH_PROTOCOL*dp){
	struct lua_uefi_device_path_data*e;
	if(!dp){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_device_path_data));
	luaL_getmetatable(L,LUA_UEFI_CHAR16);
	lua_setmetatable(L,-2);
	e->dp=dp;
}

struct lua_uefi_meta_table LuaUefiDevicePathMetaTable={
	.name=LUA_UEFI_DEVICE_PATH,
	.reg=(const luaL_Reg[]){
		{"ToString",   LuaUefiDevicePathToString},
		{"ToChar16",   LuaUefiDevicePathToChar16},
		{"SetString",  LuaUefiDevicePathFromString},
		{"FromString", LuaUefiDevicePathFromString},
		{"SetChar16",  LuaUefiDevicePathFromChar16},
		{"FromChar16", LuaUefiDevicePathFromChar16},
		{"SetHandle",  LuaUefiDevicePathFromHandle},
		{"FromHandle", LuaUefiDevicePathFromHandle},
		{NULL, NULL}
	},
	.tostring=LuaUefiDevicePathToString,
	.gc=NULL,
};

#endif

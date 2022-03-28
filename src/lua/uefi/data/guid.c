/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiGuidToRawString(lua_State*L){
	char buff[64];
	GET_GUID(L,1,guid);
	ZeroMem(buff,sizeof(buff));
	AsciiSPrint(buff,sizeof(buff),"%g",&guid->guid);
	lua_pushstring(L,buff);
	return 1;
}

static int LuaUefiGuidToString(lua_State*L){
	char buff[64];
	const char*name;
	GET_GUID(L,1,guid);
	ZeroMem(buff,sizeof(buff));
	AsciiSPrint(buff,sizeof(buff),"%g",&guid->guid);
	name=
	lua_pushstring(L,buff);
	return 1;
}

static int LuaUefiGuidFromString(lua_State*L){
	GET_GUID(L,1,guid);
	const char*str=luaL_checkstring(L,2);
	ZeroMem(&guid->guid,sizeof(EFI_GUID));
	uefi_status_to_lua(L,AsciiStrToGuid(str,&guid->guid));
	return 1;
}

static int LuaUefiGuidToProtocol(lua_State*L){
	GET_GUID(L,1,guid);
	OPT_BS(L,2,bs);
	OPT_DATA(L,3,reg);
	VOID*d=NULL;
	EFI_BOOT_SERVICES*b=bs?bs->bs:gBS;
	EFI_STATUS status=b->LocateProtocol(
		&guid->guid,reg?reg->data:NULL,&d
	);
	uefi_status_to_lua(L,status);
	uefi_data_to_protocol(L,&guid->guid,d,TRUE);
	return 2;
}

void uefi_guid_to_lua(lua_State*L,CONST EFI_GUID*guid){
	struct lua_uefi_guid_data*e;
	if(!guid){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_guid_data));
	luaL_getmetatable(L,LUA_UEFI_GUID);
	lua_setmetatable(L,-2);
	CopyGuid(&e->guid,guid);
}

void uefi_str_guid_to_lua(lua_State*L,CHAR8*str){
	EFI_GUID guid;
	AsciiStrToGuid(str,&guid);
	uefi_guid_to_lua(L,&guid);
}

void uefi_char16_guid_to_lua(lua_State*L,CHAR16*str){
	EFI_GUID guid;
	StrToGuid(str,&guid);
	uefi_guid_to_lua(L,&guid);
}

struct lua_uefi_meta_table LuaUefiGuidMetaTable={
	.name=LUA_UEFI_GUID,
	.reg=(const luaL_Reg[]){
		{"ToString",   LuaUefiGuidToRawString},
		{"ToProtocol", LuaUefiGuidToProtocol},
		{"SetString",  LuaUefiGuidFromString},
		{"FromString", LuaUefiGuidFromString},
		{NULL, NULL}
	},
	.tostring=LuaUefiGuidToString,
	.gc=NULL,
};

#endif

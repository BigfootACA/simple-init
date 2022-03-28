/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SIMPLE_FILE_SYSTEM "UEFI Simple File System Protocol"
struct lua_uefi_simple_file_system_proto{EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_simple_file_system_proto,LUA_UEFI_PROTO_SIMPLE_FILE_SYSTEM)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiSimpleFileSystemProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SIMPLE_FILE_SYSTEM" %p",proto->proto);
	return 1;
}

static int LuaUefiSimpleFileSystemProtocolOpenVolume(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_FILE_PROTOCOL*fp=NULL;
	EFI_STATUS status=proto->proto->OpenVolume(proto->proto,&fp);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	return 2;
}

static int LuaUefiSimpleFileSystemProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

void uefi_simple_file_system_protocol_to_lua(lua_State*L,EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*proto){
	struct lua_uefi_simple_file_system_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_simple_file_system_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SIMPLE_FILE_SYSTEM);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSimpleFileSystemProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SIMPLE_FILE_SYSTEM,
	.reg=(const luaL_Reg[]){
		{"Revision",   LuaUefiSimpleFileSystemProtocolRevision},
		{"OpenVolume", LuaUefiSimpleFileSystemProtocolOpenVolume},
		{NULL, NULL}
	},
	.tostring=LuaUefiSimpleFileSystemProtocolToString,
	.gc=NULL,
};

#endif

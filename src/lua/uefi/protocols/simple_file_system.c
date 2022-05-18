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

static int LuaUefiSimpleFileSystemProtocolOpen(lua_State*L){
	GET_PROTO(L,1,proto);
	LuaUefiSimpleFileSystemProtocolOpenVolume(L);
	lua_copy(L,-1,1);
	lua_pop(L,2);
	lua_getfield(L,1,"Open");
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-1,LUA_MULTRET);
	return 2;
}

static int LuaUefiSimpleFileSystemProtocolOpenDir(lua_State*L){
	GET_PROTO(L,1,proto);
	LuaUefiSimpleFileSystemProtocolOpenVolume(L);
	lua_copy(L,-1,1);
	lua_pop(L,2);
	lua_getfield(L,1,"OpenDir");
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-1,LUA_MULTRET);
	return 2;
}

static int LuaUefiSimpleFileSystemProtocolFileToData(lua_State*L){
	GET_PROTO(L,1,proto);
	LuaUefiSimpleFileSystemProtocolOpenVolume(L);
	lua_copy(L,-1,1);
	lua_pop(L,2);
	lua_getfield(L,1,"FileToData");
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-1,LUA_MULTRET);
	return 2;
}

static int LuaUefiSimpleFileSystemProtocolGetInfo(lua_State*L){
	GET_PROTO(L,1,proto);
	LuaUefiSimpleFileSystemProtocolOpenVolume(L);
	lua_copy(L,-1,1);
	lua_pop(L,2);
	lua_getfield(L,1,"GetInfo");
	lua_insert(L,1);
	uefi_guid_to_lua(L,&gEfiFileSystemInfoGuid);
	lua_call(L,2,LUA_MULTRET);
	return 2;
}

static int LuaUefiSimpleFileSystemProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*uefi_lua_to_simple_file_system_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
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
		{"Open",       LuaUefiSimpleFileSystemProtocolOpen},
		{"OpenDir",    LuaUefiSimpleFileSystemProtocolOpenDir},
		{"GetInfo",    LuaUefiSimpleFileSystemProtocolGetInfo},
		{"FileToData", LuaUefiSimpleFileSystemProtocolFileToData},
		{NULL, NULL}
	},
	.tostring=LuaUefiSimpleFileSystemProtocolToString,
	.gc=NULL,
};

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_RAM_DISK "UEFI RAM Disk Protocol"
struct lua_uefi_ramdisk_proto{EFI_RAM_DISK_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_ramdisk_proto,LUA_UEFI_PROTO_RAM_DISK)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiRamDiskProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_RAM_DISK" %p",proto->proto);
	return 1;
}

static int LuaUefiRamDiskProtocolRegister(lua_State*L){
	size_t ds=0;
	EFI_GUID guid;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_data(L,2,false,&data,&ds);
	lua_arg_get_guid(L,3,false,&guid);
	OPT_DEVICE_PATH(L,4,parent);
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	if(!data||ds<=0)
		return luaL_argerror(L,2,"invalid data");
	EFI_STATUS status=proto->proto->Register(
		(UINT64)(UINTN)data,(UINT64)ds,
		&guid,parent?parent->dp:NULL,&dp
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!dp)lua_pushnil(L);
	else uefi_device_path_to_lua(L,dp);
	return 2;
}

static int LuaUefiRamDiskProtocolUnregister(lua_State*L){
	GET_PROTO(L,1,proto);
	GET_DEVICE_PATH(L,2,dp);
	EFI_STATUS status=proto->proto->Unregister(dp->dp);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_RAM_DISK_PROTOCOL*uefi_lua_to_ramdisk_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_ramdisk_protocol_to_lua(lua_State*L,EFI_RAM_DISK_PROTOCOL*proto){
	struct lua_uefi_ramdisk_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_ramdisk_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_RAM_DISK);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiRamDiskProtocolMetaTable={
	.name=LUA_UEFI_PROTO_RAM_DISK,
	.reg=(const luaL_Reg[]){
		{"Register",   LuaUefiRamDiskProtocolRegister},
		{"Unregister", LuaUefiRamDiskProtocolUnregister},
		{NULL, NULL}
	},
	.tostring=LuaUefiRamDiskProtocolToString,
	.gc=NULL,
};

#endif

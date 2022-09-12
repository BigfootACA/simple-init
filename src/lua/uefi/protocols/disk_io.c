/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_DISK_IO "UEFI Disk IO Protocol"
struct lua_uefi_disk_io_proto{EFI_DISK_IO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_disk_io_proto,LUA_UEFI_PROTO_DISK_IO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiDiskIOProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_DISK_IO" %p",proto->proto);
	return 1;
}

static int LuaUefiDiskIOProtocolReadDisk(lua_State*L){
	VOID*buffer=NULL;
	GET_PROTO(L,1,proto);
	UINT32 media=luaL_checkinteger(L,2);
	UINT64 off=luaL_checkinteger(L,3);
	UINTN size=luaL_checkinteger(L,4);
	if(!(buffer=AllocateZeroPool(size))){
		uefi_status_to_lua(L,EFI_OUT_OF_RESOURCES);
		lua_pushnil(L);
		return 2;
	}
	EFI_STATUS status=proto->proto->ReadDisk(
		proto->proto,media,off,size,buffer
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)){
		lua_pushnil(L);
		FreePool(buffer);
	}else uefi_data_to_lua(L,TRUE,buffer,size);
	return 2;
}

static int LuaUefiDiskIOProtocolWriteDisk(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	UINT32 media=luaL_checkinteger(L,2);
	UINT64 off=luaL_checkinteger(L,3);
	lua_arg_get_data(L,4,false,&data,&ds);
	UINTN size=luaL_optinteger(L,5,ds);
	if(!data)return luaL_argerror(L,4,"empty data");
	if(size<=0)return luaL_argerror(L,5,"empty data");
	EFI_STATUS status=proto->proto->WriteDisk(
		proto->proto,media,off,size,data
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))size=0;
	lua_pushinteger(L,size);
	return 2;
}

static int LuaUefiDiskIOProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

EFI_DISK_IO_PROTOCOL*uefi_lua_to_disk_io_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_disk_io_protocol_to_lua(lua_State*L,EFI_DISK_IO_PROTOCOL*proto){
	struct lua_uefi_disk_io_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_disk_io_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_DISK_IO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiDiskIOProtocolMetaTable={
	.name=LUA_UEFI_PROTO_DISK_IO,
	.reg=(const luaL_Reg[]){
		{"Revision",  LuaUefiDiskIOProtocolRevision},
		{"ReadDisk",  LuaUefiDiskIOProtocolReadDisk},
		{"WriteDisk", LuaUefiDiskIOProtocolWriteDisk},
		{NULL, NULL}
	},
	.tostring=LuaUefiDiskIOProtocolToString,
	.gc=NULL,
};

#endif

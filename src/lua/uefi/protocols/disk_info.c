/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_DISK_INFO "UEFI Disk Info Protocol"
struct lua_uefi_disk_info_proto{EFI_DISK_INFO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_disk_info_proto,LUA_UEFI_PROTO_DISK_INFO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiDiskInfoProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_DISK_INFO" %p",proto->proto);
	return 1;
}

static int LuaUefiDiskInfoProtocolInterface(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_guid_to_lua(L,&proto->proto->Interface);
	return 1;
}

static int LuaUefiDiskInfoProtocolInquiry(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT32 size=0;
	VOID*data=NULL;
	EFI_STATUS status=proto->proto->Inquiry(
		proto->proto,&data,&size
	);
	if(status==EFI_BUFFER_TOO_SMALL){
		if(!(data=AllocateZeroPool(size)))
			return luaL_error(L,"allocate buffer failed");
		status=proto->proto->Inquiry(
			proto->proto,&data,&size
		);
	}
	uefi_status_to_lua(L,status);
	uefi_data_to_lua(L,TRUE,data,(UINTN)size);
	return 2;
}

static int LuaUefiDiskInfoProtocolIdentify(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT32 size=0;
	VOID*data=NULL;
	EFI_STATUS status=proto->proto->Identify(
		proto->proto,&data,&size
	);
	if(status==EFI_BUFFER_TOO_SMALL){
		if(!(data=AllocateZeroPool(size)))
			return luaL_error(L,"allocate buffer failed");
		status=proto->proto->Identify(
			proto->proto,&data,&size
		);
	}
	uefi_status_to_lua(L,status);
	uefi_data_to_lua(L,TRUE,data,(UINTN)size);
	return 2;
}

static int LuaUefiDiskInfoProtocolSenseData(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT8 number=0;
	UINT32 size=0;
	VOID*data=NULL;
	EFI_STATUS status=proto->proto->SenseData(
		proto->proto,&data,&size,&number
	);
	if(status==EFI_BUFFER_TOO_SMALL){
		if(!(data=AllocateZeroPool(size)))
			return luaL_error(L,"allocate buffer failed");
		status=proto->proto->SenseData(
			proto->proto,&data,&size,&number
		);
	}
	uefi_status_to_lua(L,status);
	uefi_data_to_lua(L,TRUE,data,(UINTN)size);
	lua_pushinteger(L,number);
	return 3;
}

static int LuaUefiDiskInfoProtocolWhichIde(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT32 channel=0,device=0;
	EFI_STATUS status=proto->proto->WhichIde(
		proto->proto,&channel,&device
	);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,channel);
	lua_pushinteger(L,device);
	return 3;
}

EFI_DISK_INFO_PROTOCOL*uefi_lua_to_disk_info_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_disk_info_protocol_to_lua(lua_State*L,EFI_DISK_INFO_PROTOCOL*proto){
	struct lua_uefi_disk_info_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_disk_info_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_DISK_INFO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiDiskInfoProtocolMetaTable={
	.name=LUA_UEFI_PROTO_DISK_INFO,
	.reg=(const luaL_Reg[]){
		{"Interface", LuaUefiDiskInfoProtocolInterface},
		{"Inquiry",   LuaUefiDiskInfoProtocolInquiry},
		{"Identify",  LuaUefiDiskInfoProtocolIdentify},
		{"SenseData", LuaUefiDiskInfoProtocolSenseData},
		{"WhichIde",  LuaUefiDiskInfoProtocolWhichIde},
		{NULL, NULL}
	},
	.tostring=LuaUefiDiskInfoProtocolToString,
	.gc=NULL,
};

#endif

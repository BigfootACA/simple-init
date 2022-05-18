/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_LOADED_IMAGE "UEFI Loaded Image Protocol"
struct lua_uefi_loaded_image_proto{EFI_LOADED_IMAGE_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_loaded_image_proto,LUA_UEFI_PROTO_LOADED_IMAGE)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiLoadedImageProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_LOADED_IMAGE" %p",proto->proto);
	return 1;
}

static int LuaUefiLoadedImageProtocolParentHandle(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_handle_to_lua(L,proto->proto->ParentHandle);
	return 1;
}

static int LuaUefiLoadedImageProtocolSystemTable(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_st_to_lua(L,proto->proto->SystemTable);
	return 1;
}

static int LuaUefiLoadedImageProtocolDeviceHandle(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_handle_to_lua(L,proto->proto->DeviceHandle);
	return 1;
}

static int LuaUefiLoadedImageProtocolFilePath(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_device_path_to_lua(L,proto->proto->FilePath);
	return 1;
}

static int LuaUefiLoadedImageProtocolLoadOptions(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_data(L,2,true,&data,&ds);
	if(data){
		proto->proto->LoadOptions=data;
		proto->proto->LoadOptionsSize=ds;
	}
	uefi_data_to_lua(L,FALSE,
		proto->proto->LoadOptions,
		proto->proto->LoadOptionsSize
	);
	return 1;
}

static int LuaUefiLoadedImageProtocolImageCodeType(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushstring(L,uefi_memory_type_to_str(proto->proto->ImageCodeType));
	return 1;
}

static int LuaUefiLoadedImageProtocolImageDataType(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushstring(L,uefi_memory_type_to_str(proto->proto->ImageDataType));
	return 1;
}

static int LuaUefiLoadedImageProtocolImage(lua_State*L){
	GET_PROTO(L,1,proto);
	uefi_data_to_lua(L,FALSE,
		proto->proto->ImageBase,
		proto->proto->ImageSize
	);
	return 1;
}

static int LuaUefiLoadedImageProtocolUnload(lua_State*L){
	GET_PROTO(L,1,proto);
	GET_HANDLE(L,2,hand);
	EFI_STATUS status=proto->proto->Unload(hand->hand);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLoadedImageProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

EFI_LOADED_IMAGE_PROTOCOL*uefi_lua_to_loaded_image_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_loaded_image_protocol_to_lua(lua_State*L,EFI_LOADED_IMAGE_PROTOCOL*proto){
	struct lua_uefi_loaded_image_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_loaded_image_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_LOADED_IMAGE);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiLoadedImageProtocolMetaTable={
	.name=LUA_UEFI_PROTO_LOADED_IMAGE,
	.reg=(const luaL_Reg[]){
		{"Revision",       LuaUefiLoadedImageProtocolRevision},
		{"ParentHandle",   LuaUefiLoadedImageProtocolParentHandle},
		{"SystemTable",    LuaUefiLoadedImageProtocolSystemTable},
		{"DeviceHandle",   LuaUefiLoadedImageProtocolDeviceHandle},
		{"FilePath",       LuaUefiLoadedImageProtocolFilePath},
		{"LoadOptions",    LuaUefiLoadedImageProtocolLoadOptions},
		{"SetLoadOptions", LuaUefiLoadedImageProtocolLoadOptions},
		{"ImageCodeType",  LuaUefiLoadedImageProtocolImageCodeType},
		{"ImageDataType",  LuaUefiLoadedImageProtocolImageDataType},
		{"Image",          LuaUefiLoadedImageProtocolImage},
		{"Unload",         LuaUefiLoadedImageProtocolUnload},
		{NULL, NULL}
	},
	.tostring=LuaUefiLoadedImageProtocolToString,
	.gc=NULL,
};

#endif

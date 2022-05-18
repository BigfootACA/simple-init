/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_DEFERRED_IMAGE_LOAD "UEFI Deferred Image Load Protocol"
struct lua_uefi_deferred_image_load_proto{EFI_DEFERRED_IMAGE_LOAD_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_deferred_image_load_proto,LUA_UEFI_PROTO_DEFERRED_IMAGE_LOAD)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiDeferredImageLoadProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_DEFERRED_IMAGE_LOAD" %p",proto->proto);
	return 1;
}

static int LuaUefiDeferredImageLoadProtocolGetImageInfo(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN index=luaL_checkinteger(L,2);
	UINTN size=0;
	VOID*data=NULL;
	BOOLEAN bootopt=FALSE;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_STATUS status=proto->proto->GetImageInfo(
		proto->proto,index,&dp,&data,&size,&bootopt
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)){
		lua_pushnil(L);
		lua_pushnil(L);
	}else{
		uefi_device_path_to_lua(L,dp);
		uefi_data_to_lua(L,FALSE,data,size);
	}
	lua_pushboolean(L,bootopt);
	return 4;
}

EFI_DEFERRED_IMAGE_LOAD_PROTOCOL*uefi_lua_to_deferred_image_load_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_deferred_image_load_protocol_to_lua(lua_State*L,EFI_DEFERRED_IMAGE_LOAD_PROTOCOL*proto){
	struct lua_uefi_deferred_image_load_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_deferred_image_load_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_DEFERRED_IMAGE_LOAD);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiDeferredImageLoadProtocolMetaTable={
	.name=LUA_UEFI_PROTO_DEFERRED_IMAGE_LOAD,
	.reg=(const luaL_Reg[]){
		{"GetImageInfo", LuaUefiDeferredImageLoadProtocolGetImageInfo},
		{NULL, NULL}
	},
	.tostring=LuaUefiDeferredImageLoadProtocolToString,
	.gc=NULL,
};

#endif

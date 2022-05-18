/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_COMPONENT_NAME2 "UEFI Component Name 2 Protocol"
struct lua_uefi_component_name2_proto{EFI_COMPONENT_NAME2_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_component_name2_proto,LUA_UEFI_PROTO_COMPONENT_NAME2)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiComponentName2ProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_COMPONENT_NAME2" %p",proto->proto);
	return 1;
}

static int LuaUefiComponentName2ProtocolGetDriverName(lua_State*L){
	GET_PROTO(L,1,proto);
	const char*lang=luaL_checkstring(L,2);
	CHAR16*name=NULL;
	EFI_STATUS status=proto->proto->GetDriverName(
		proto->proto,(CHAR8*)lang,&name
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!name)lua_pushnil(L);
	else uefi_char16_16_to_lua(L,FALSE,name);
	return 2;
}

static int LuaUefiComponentName2ProtocolGetControllerName(lua_State*L){
	GET_PROTO(L,1,proto);
	GET_HANDLE(L,2,ctrl);
	OPT_HANDLE(L,3,child);
	const char*lang=luaL_checkstring(L,4);
	CHAR16*name=NULL;
	EFI_STATUS status=proto->proto->GetControllerName(
		proto->proto,
		ctrl?ctrl->hand:NULL,
		child?child->hand:NULL,
		(CHAR8*)lang,&name
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!name)lua_pushnil(L);
	else uefi_char16_16_to_lua(L,FALSE,name);
	return 2;
}

static int LuaUefiComponentName2ProtocolSupportedLanguages(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushstring(L,proto->proto->SupportedLanguages);
	return 1;
}

EFI_COMPONENT_NAME2_PROTOCOL*uefi_lua_to_component_name2_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_component_name2_protocol_to_lua(lua_State*L,EFI_COMPONENT_NAME2_PROTOCOL*proto){
	struct lua_uefi_component_name2_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_component_name2_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_COMPONENT_NAME2);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiComponentName2ProtocolMetaTable={
	.name=LUA_UEFI_PROTO_COMPONENT_NAME2,
	.reg=(const luaL_Reg[]){
		{"GetDriverName",      LuaUefiComponentName2ProtocolGetDriverName},
		{"GetControllerName",  LuaUefiComponentName2ProtocolGetControllerName},
		{"SupportedLanguages", LuaUefiComponentName2ProtocolSupportedLanguages},
		{NULL, NULL}
	},
	.tostring=LuaUefiComponentName2ProtocolToString,
	.gc=NULL,
};

#endif

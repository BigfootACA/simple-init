/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SECURITY_ARCH "UEFI Security Code Architectural Protocol"
struct lua_uefi_security_arch_proto{EFI_SECURITY_ARCH_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_security_arch_proto,LUA_UEFI_PROTO_SECURITY_ARCH)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiSecurityArchProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SECURITY_ARCH" %p",proto->proto);
	return 1;
}

static int LuaUefiSecurityArchProtocolFileAuthenticationState(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT32 st=luaL_checkinteger(L,2);
	GET_DEVICE_PATH(L,3,dp);
	EFI_STATUS status=proto->proto->FileAuthenticationState(
		proto->proto,st,dp->dp
	);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_SECURITY_ARCH_PROTOCOL*uefi_lua_to_security_arch_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_security_arch_protocol_to_lua(lua_State*L,EFI_SECURITY_ARCH_PROTOCOL*proto){
	struct lua_uefi_security_arch_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_security_arch_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SECURITY_ARCH);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSecurityArchProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SECURITY_ARCH,
	.reg=(const luaL_Reg[]){
		{"FileAuthenticationState", LuaUefiSecurityArchProtocolFileAuthenticationState},
		{NULL, NULL}
	},
	.tostring=LuaUefiSecurityArchProtocolToString,
	.gc=NULL,
};

#endif

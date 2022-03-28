/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SECURITY2_ARCH "UEFI Security2 Code Architectural Protocol"
struct lua_uefi_security2_arch_proto{EFI_SECURITY2_ARCH_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_security2_arch_proto,LUA_UEFI_PROTO_SECURITY2_ARCH)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiSecurity2ArchProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SECURITY2_ARCH" %p",proto->proto);
	return 1;
}

static int LuaUefiSecurity2ArchProtocolFileAuthentication(lua_State*L){
	GET_PROTO(L,1,proto);
	OPT_DEVICE_PATH(L,2,dp);
	GET_DATA(L,3,file);
	luaL_checktype(L,4,LUA_TBOOLEAN);
	BOOLEAN boot_policy=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->FileAuthentication(
		proto->proto,
		dp?dp->dp:NULL,
		file->data,
		file->size,
		boot_policy
	);
	uefi_status_to_lua(L,status);
	return 1;
}

void uefi_security2_arch_protocol_to_lua(lua_State*L,EFI_SECURITY2_ARCH_PROTOCOL*proto){
	struct lua_uefi_security2_arch_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_security2_arch_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SECURITY2_ARCH);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSecurity2ArchProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SECURITY2_ARCH,
	.reg=(const luaL_Reg[]){
		{"FileAuthentication", LuaUefiSecurity2ArchProtocolFileAuthentication},
		{NULL, NULL}
	},
	.tostring=LuaUefiSecurity2ArchProtocolToString,
	.gc=NULL,
};

#endif

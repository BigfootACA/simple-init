/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_BOOT_MANAGER_POLICY "UEFI Boot Manager Policy Protocol"
struct lua_uefi_boot_manager_policy_proto{EFI_BOOT_MANAGER_POLICY_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_boot_manager_policy_proto,LUA_UEFI_PROTO_BOOT_MANAGER_POLICY)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiBootManagerPolicyProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_BOOT_MANAGER_POLICY" %p",proto->proto);
	return 1;
}

static int LuaUefiBootManagerPolicyProtocolConnectDevicePath(lua_State*L){
	GET_PROTO(L,1,proto);
	GET_DEVICE_PATH(L,2,dp);
	luaL_checktype(L,3,LUA_TBOOLEAN);
	BOOLEAN rec=lua_toboolean(L,3);
	EFI_STATUS status=proto->proto->ConnectDevicePath(
		proto->proto,dp->dp,rec
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBootManagerPolicyProtocolConnectDeviceClass(lua_State*L){
	GET_PROTO(L,1,proto);
	GET_GUID(L,2,guid);
	EFI_STATUS status=proto->proto->ConnectDeviceClass(
		proto->proto,&guid->guid
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBootManagerPolicyProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

void uefi_boot_manager_policy_protocol_to_lua(lua_State*L,EFI_BOOT_MANAGER_POLICY_PROTOCOL*proto){
	struct lua_uefi_boot_manager_policy_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_boot_manager_policy_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_BOOT_MANAGER_POLICY);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiBootManagerPolicyProtocolMetaTable={
	.name=LUA_UEFI_PROTO_BOOT_MANAGER_POLICY,
	.reg=(const luaL_Reg[]){
		{"Revision",           LuaUefiBootManagerPolicyProtocolRevision},
		{"ConnectDevicePath",  LuaUefiBootManagerPolicyProtocolConnectDevicePath},
		{"ConnectDeviceClass", LuaUefiBootManagerPolicyProtocolConnectDeviceClass},
		{NULL, NULL}
	},
	.tostring=LuaUefiBootManagerPolicyProtocolToString,
	.gc=NULL,
};

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_ACPI_TABLE "UEFI ACPI Table Protocol"
struct lua_uefi_acpi_table_proto{EFI_ACPI_TABLE_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_acpi_table_proto,LUA_UEFI_PROTO_ACPI_TABLE)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiAcpiTableProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_ACPI_TABLE" %p",proto->proto);
	return 1;
}

static int LuaUefiAcpiTableProtocolInstall(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN key=0;
	EFI_STATUS status=proto->proto->InstallAcpiTable(
		proto->proto,data,ds,&key
	);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,key);
	return 2;
}

static int LuaUefiAcpiTableProtocolUninstall(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer key=luaL_checkinteger(L,2);
	EFI_STATUS status=proto->proto->UninstallAcpiTable(
		proto->proto,(UINTN)key
	);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_ACPI_TABLE_PROTOCOL*uefi_lua_to_acpi_table_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_acpi_table_protocol_to_lua(lua_State*L,EFI_ACPI_TABLE_PROTOCOL*proto){
	struct lua_uefi_acpi_table_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_acpi_table_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_ACPI_TABLE);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiAcpiTableProtocolMetaTable={
	.name=LUA_UEFI_PROTO_ACPI_TABLE,
	.reg=(const luaL_Reg[]){
		{"InstallAcpiTable",   LuaUefiAcpiTableProtocolInstall},
		{"UninstallAcpiTable", LuaUefiAcpiTableProtocolUninstall},
		{NULL, NULL}
	},
	.tostring=LuaUefiAcpiTableProtocolToString,
	.gc=NULL,
};

#endif

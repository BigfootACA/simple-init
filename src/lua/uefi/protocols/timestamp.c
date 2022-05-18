/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_TIMESTAMP "UEFI TimeStamp Protocol"
struct lua_uefi_timestamp_proto{EFI_TIMESTAMP_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_timestamp_proto,LUA_UEFI_PROTO_TIMESTAMP)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiTimeStampProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_TIMESTAMP" %p",proto->proto);
	return 1;
}

static int LuaUefiTimeStampProtocolGetProperties(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_TIMESTAMP_PROPERTIES prop;
	EFI_STATUS status=proto->proto->GetProperties(&prop);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))lua_pushnil(L);
	else{
		lua_createtable(L,0,0);
		lua_pushliteral(L,"Frequency");
		lua_pushinteger(L,prop.Frequency);
		lua_settable(L,-3);
		lua_pushliteral(L,"EndValue");
		lua_pushinteger(L,prop.EndValue);
		lua_settable(L,-3);
	}
	return 2;
}

static int LuaUefiTimeStampProtocolGetTimestamp(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->GetTimestamp());
	return 2;
}

EFI_TIMESTAMP_PROTOCOL*uefi_lua_to_timestamp_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_timestamp_protocol_to_lua(lua_State*L,EFI_TIMESTAMP_PROTOCOL*proto){
	struct lua_uefi_timestamp_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_timestamp_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_TIMESTAMP);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiTimeStampProtocolMetaTable={
	.name=LUA_UEFI_PROTO_TIMESTAMP,
	.reg=(const luaL_Reg[]){
		{"GetProperties", LuaUefiTimeStampProtocolGetProperties},
		{"GetTimestamp",  LuaUefiTimeStampProtocolGetTimestamp},
		{NULL, NULL}
	},
	.tostring=LuaUefiTimeStampProtocolToString,
	.gc=NULL,
};

#endif

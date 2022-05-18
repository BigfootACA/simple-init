/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SIMPLE_TEXT_INPUT "UEFI Simple Text Input Protocol"
struct lua_uefi_simple_text_input_proto{EFI_SIMPLE_TEXT_INPUT_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_simple_text_input_proto,LUA_UEFI_PROTO_SIMPLE_TEXT_INPUT)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiSimpleTextInProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SIMPLE_TEXT_INPUT" %p",proto->proto);
	return 1;
}

static int LuaUefiSimpleTextInProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN ext_verify=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->Reset(proto->proto,ext_verify);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextInProtocolReadKeyStroke(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_INPUT_KEY key;
	EFI_STATUS status=proto->proto->ReadKeyStroke(proto->proto,&key);
	uefi_status_to_lua(L,status);
	uefi_input_key_to_lua(L,&key);
	return 2;
}

static int LuaUefiSimpleTextInProtocolWaitForKey(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN ei=0;
	EFI_STATUS status=EFI_INVALID_PARAMETER;
	if(proto->proto->WaitForKey)
		status=gBS->WaitForEvent(1,&proto->proto->WaitForKey,&ei);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_SIMPLE_TEXT_INPUT_PROTOCOL*uefi_lua_to_simple_text_input_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_simple_text_input_protocol_to_lua(lua_State*L,EFI_SIMPLE_TEXT_INPUT_PROTOCOL*proto){
	struct lua_uefi_simple_text_input_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_simple_text_input_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SIMPLE_TEXT_INPUT);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSimpleTextInProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SIMPLE_TEXT_INPUT,
	.reg=(const luaL_Reg[]){
		{"Reset",         LuaUefiSimpleTextInProtocolReset},
		{"ReadKeyStroke", LuaUefiSimpleTextInProtocolReadKeyStroke},
		{"WaitForKey",    LuaUefiSimpleTextInProtocolWaitForKey},
		{NULL, NULL}
	},
	.tostring=LuaUefiSimpleTextInProtocolToString,
	.gc=NULL,
};

#endif

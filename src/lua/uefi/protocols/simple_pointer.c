/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SIMPLE_POINTER "UEFI Simple Pointer Protocol"
struct lua_uefi_simple_pointer_proto{EFI_SIMPLE_POINTER_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_simple_pointer_proto,LUA_UEFI_PROTO_SIMPLE_POINTER)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void load_state(lua_State*L,EFI_SIMPLE_POINTER_STATE*st){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"RelativeMovementX");
	lua_pushinteger(L,st->RelativeMovementX);
	lua_settable(L,-3);
	lua_pushliteral(L,"RelativeMovementY");
	lua_pushinteger(L,st->RelativeMovementY);
	lua_settable(L,-3);
	lua_pushliteral(L,"RelativeMovementZ");
	lua_pushinteger(L,st->RelativeMovementZ);
	lua_settable(L,-3);
	lua_pushliteral(L,"LeftButton");
	lua_pushboolean(L,st->LeftButton);
	lua_settable(L,-3);
	lua_pushliteral(L,"RightButton");
	lua_pushboolean(L,st->RightButton);
	lua_settable(L,-3);
}

static void load_mode(lua_State*L,EFI_SIMPLE_POINTER_MODE*mode){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"ResolutionX");
	lua_pushinteger(L,mode->ResolutionX);
	lua_settable(L,-3);
	lua_pushliteral(L,"ResolutionY");
	lua_pushinteger(L,mode->ResolutionY);
	lua_settable(L,-3);
	lua_pushliteral(L,"ResolutionZ");
	lua_pushinteger(L,mode->ResolutionZ);
	lua_settable(L,-3);
	lua_pushliteral(L,"LeftButton");
	lua_pushboolean(L,mode->LeftButton);
	lua_settable(L,-3);
	lua_pushliteral(L,"RightButton");
	lua_pushboolean(L,mode->RightButton);
	lua_settable(L,-3);
}

static int LuaUefiSimplePointerProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SIMPLE_POINTER" %p",proto->proto);
	return 1;
}

static int LuaUefiSimplePointerProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN ext_verify=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->Reset(proto->proto,ext_verify);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimplePointerProtocolGetState(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_SIMPLE_POINTER_STATE st;
	EFI_STATUS status=proto->proto->GetState(proto->proto,&st);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))lua_pushnil(L);
	else load_state(L,&st);
	return 2;
}

static int LuaUefiSimplePointerProtocolMode(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Mode)lua_pushnil(L);
	else load_mode(L,proto->proto->Mode);
	return 2;
}

static int LuaUefiSimplePointerProtocolWaitForInput(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN ei=0;
	EFI_STATUS status=EFI_INVALID_PARAMETER;
	if(proto->proto->WaitForInput)
		status=gBS->WaitForEvent(1,&proto->proto->WaitForInput,&ei);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_SIMPLE_POINTER_PROTOCOL*uefi_lua_to_simple_pointer_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_simple_pointer_protocol_to_lua(lua_State*L,EFI_SIMPLE_POINTER_PROTOCOL*proto){
	struct lua_uefi_simple_pointer_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_simple_pointer_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SIMPLE_POINTER);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSimplePointerProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SIMPLE_POINTER,
	.reg=(const luaL_Reg[]){
		{"Reset",        LuaUefiSimplePointerProtocolReset},
		{"GetState",     LuaUefiSimplePointerProtocolGetState},
		{"Mode",         LuaUefiSimplePointerProtocolMode},
		{"WaitForInput", LuaUefiSimplePointerProtocolWaitForInput},
		{NULL, NULL}
	},
	.tostring=LuaUefiSimplePointerProtocolToString,
	.gc=NULL,
};

#endif

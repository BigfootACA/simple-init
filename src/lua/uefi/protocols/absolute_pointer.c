/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_ABSOLUTE_POINTER "UEFI Absolute Pointer Protocol"
struct lua_uefi_absolute_pointer_proto{EFI_ABSOLUTE_POINTER_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_absolute_pointer_proto,LUA_UEFI_PROTO_ABSOLUTE_POINTER)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void load_state(lua_State*L,EFI_ABSOLUTE_POINTER_STATE*st){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"CurrentX");
	lua_pushinteger(L,st->CurrentX);
	lua_settable(L,-3);
	lua_pushliteral(L,"CurrentY");
	lua_pushinteger(L,st->CurrentY);
	lua_settable(L,-3);
	lua_pushliteral(L,"CurrentZ");
	lua_pushinteger(L,st->CurrentZ);
	lua_settable(L,-3);
	lua_pushliteral(L,"ActiveButtons");
	lua_pushinteger(L,st->ActiveButtons);
	lua_settable(L,-3);
}

static void load_mode(lua_State*L,EFI_ABSOLUTE_POINTER_MODE*mode){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"AbsoluteMinX");
	lua_pushinteger(L,mode->AbsoluteMinX);
	lua_settable(L,-3);
	lua_pushliteral(L,"AbsoluteMinY");
	lua_pushinteger(L,mode->AbsoluteMinY);
	lua_settable(L,-3);
	lua_pushliteral(L,"AbsoluteMinZ");
	lua_pushinteger(L,mode->AbsoluteMinZ);
	lua_settable(L,-3);
	lua_pushliteral(L,"AbsoluteMaxX");
	lua_pushinteger(L,mode->AbsoluteMaxX);
	lua_settable(L,-3);
	lua_pushliteral(L,"AbsoluteMaxY");
	lua_pushinteger(L,mode->AbsoluteMaxY);
	lua_settable(L,-3);
	lua_pushliteral(L,"AbsoluteMaxZ");
	lua_pushinteger(L,mode->AbsoluteMaxZ);
	lua_settable(L,-3);
	lua_pushliteral(L,"Attributes");
	lua_pushinteger(L,mode->Attributes);
	lua_settable(L,-3);
}

static int LuaUefiAbsolutePointerProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_ABSOLUTE_POINTER" %p",proto->proto);
	return 1;
}

static int LuaUefiAbsolutePointerProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN ext_verify=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->Reset(proto->proto,ext_verify);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiAbsolutePointerProtocolGetState(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_ABSOLUTE_POINTER_STATE st;
	EFI_STATUS status=proto->proto->GetState(proto->proto,&st);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))lua_pushnil(L);
	else load_state(L,&st);
	return 2;
}

static int LuaUefiAbsolutePointerProtocolMode(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Mode)lua_pushnil(L);
	else load_mode(L,proto->proto->Mode);
	return 1;
}

static int LuaUefiAbsolutePointerProtocolWaitForInput(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN ei=0;
	EFI_STATUS status=EFI_INVALID_PARAMETER;
	if(proto->proto->WaitForInput)
		status=gBS->WaitForEvent(1,&proto->proto->WaitForInput,&ei);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_ABSOLUTE_POINTER_PROTOCOL*uefi_lua_to_absolute_pointer_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_absolute_pointer_protocol_to_lua(lua_State*L,EFI_ABSOLUTE_POINTER_PROTOCOL*proto){
	struct lua_uefi_absolute_pointer_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_absolute_pointer_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_ABSOLUTE_POINTER);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiAbsolutePointerProtocolMetaTable={
	.name=LUA_UEFI_PROTO_ABSOLUTE_POINTER,
	.reg=(const luaL_Reg[]){
		{"Reset",        LuaUefiAbsolutePointerProtocolReset},
		{"GetState",     LuaUefiAbsolutePointerProtocolGetState},
		{"Mode",         LuaUefiAbsolutePointerProtocolMode},
		{"WaitForInput", LuaUefiAbsolutePointerProtocolWaitForInput},
		{NULL, NULL}
	},
	.tostring=LuaUefiAbsolutePointerProtocolToString,
	.gc=NULL,
};

#endif

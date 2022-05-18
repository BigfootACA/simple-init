/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SIMPLE_TEXT_OUTPUT "UEFI Simple Text Output Protocol"
struct lua_uefi_simple_text_output_proto{EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_simple_text_output_proto,LUA_UEFI_PROTO_SIMPLE_TEXT_OUTPUT)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void load_mode(lua_State*L,EFI_SIMPLE_TEXT_OUTPUT_MODE*mode){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"MaxMode");
	lua_pushinteger(L,mode->MaxMode);
	lua_settable(L,-3);
	lua_pushliteral(L,"Mode");
	lua_pushinteger(L,mode->Mode);
	lua_settable(L,-3);
	lua_pushliteral(L,"Attribute");
	lua_pushinteger(L,mode->Attribute);
	lua_settable(L,-3);
	lua_pushliteral(L,"CursorColumn");
	lua_pushinteger(L,mode->CursorColumn);
	lua_settable(L,-3);
	lua_pushliteral(L,"CursorRow");
	lua_pushinteger(L,mode->CursorRow);
	lua_settable(L,-3);
	lua_pushliteral(L,"CursorVisible");
	lua_pushboolean(L,mode->CursorVisible);
	lua_settable(L,-3);
}

static int LuaUefiSimpleTextOutProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SIMPLE_TEXT_OUTPUT" %p",proto->proto);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN ext_verify=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->Reset(proto->proto,ext_verify);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolOutputString(lua_State*L){
	CHAR16*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_char16(L,2,false,&data);
	if(!data)return luaL_argerror(L,2,"get argument failed");
	EFI_STATUS status=proto->proto->OutputString(proto->proto,data);
	uefi_status_to_lua(L,status);
	FreePool(data);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolTestString(lua_State*L){
	CHAR16*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_char16(L,2,false,&data);
	if(!data)return luaL_argerror(L,2,"get argument failed");
	EFI_STATUS status=proto->proto->TestString(proto->proto,data);
	uefi_status_to_lua(L,status);
	FreePool(data);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolClearScreen(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_STATUS status=proto->proto->ClearScreen(proto->proto);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolSetCursorPosition(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer col=luaL_checkinteger(L,2);
	lua_Integer row=luaL_checkinteger(L,3);
	EFI_STATUS status=proto->proto->SetCursorPosition(
		proto->proto,
		(UINTN)col,
		(UINTN)row
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolEnableCursor(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN visible=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->EnableCursor(proto->proto,visible);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolSetAttribute(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer attr=luaL_checkinteger(L,2);
	EFI_STATUS status=proto->proto->SetAttribute(proto->proto,(UINTN)attr);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolSetMode(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer mode=luaL_checkinteger(L,2);
	EFI_STATUS status=proto->proto->SetMode(proto->proto,(UINTN)mode);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSimpleTextOutProtocolQueryMode(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer mode=luaL_checkinteger(L,2);
	UINTN col=0,row=0;
	EFI_STATUS status=proto->proto->QueryMode(proto->proto,(UINTN)mode,&col,&row);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,col);
	lua_pushinteger(L,row);
	return 3;
}

static int LuaUefiSimpleTextOutProtocolMode(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Mode)lua_pushnil(L);
	else load_mode(L,proto->proto->Mode);
	return 1;
}

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*uefi_lua_to_simple_text_output_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_simple_text_output_protocol_to_lua(lua_State*L,EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*proto){
	struct lua_uefi_simple_text_output_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_simple_text_output_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SIMPLE_TEXT_OUTPUT);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSimpleTextOutProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SIMPLE_TEXT_OUTPUT,
	.reg=(const luaL_Reg[]){
		{"Reset",             LuaUefiSimpleTextOutProtocolReset},
		{"OutputString",      LuaUefiSimpleTextOutProtocolOutputString},
		{"TestString",        LuaUefiSimpleTextOutProtocolTestString},
		{"QueryMode",         LuaUefiSimpleTextOutProtocolQueryMode},
		{"SetMode",           LuaUefiSimpleTextOutProtocolSetMode},
		{"SetAttribute",      LuaUefiSimpleTextOutProtocolSetAttribute},
		{"ClearScreen",       LuaUefiSimpleTextOutProtocolClearScreen},
		{"SetCursorPosition", LuaUefiSimpleTextOutProtocolSetCursorPosition},
		{"EnableCursor",      LuaUefiSimpleTextOutProtocolEnableCursor},
		{"Mode",              LuaUefiSimpleTextOutProtocolMode},
		{NULL, NULL}
	},
	.tostring=LuaUefiSimpleTextOutProtocolToString,
	.gc=NULL,
};

#endif

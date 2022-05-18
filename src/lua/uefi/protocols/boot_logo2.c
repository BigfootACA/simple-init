/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_BOOT_LOGO2 "UEFI Boot Logo 2 Protocol"
struct lua_uefi_boot_logo2_proto{EDKII_BOOT_LOGO2_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_boot_logo2_proto,LUA_UEFI_PROTO_BOOT_LOGO2)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiBootLogo2ProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_BOOT_LOGO2" %p",proto->proto);
	return 1;
}

static int LuaUefiBootLogo2ProtocolSetBootLogo(lua_State*L){
	BOOLEAN alloc=FALSE;
	UINTN dstx=0,dsty=0,width=0,height=0;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*pixels=NULL;
	GET_PROTO(L,1,proto);
	uefi_graphics_output_get_pixels(L,2,&pixels,&alloc);
	dstx=luaL_optinteger(L,3,0);
	dsty=luaL_optinteger(L,4,0);
	width=luaL_optinteger(L,5,0);
	height=luaL_optinteger(L,6,0);
	EFI_STATUS status=proto->proto->SetBootLogo(
		proto->proto,pixels,
		dstx,dsty,
		width,height
	);
	uefi_status_to_lua(L,status);
	if(pixels&&alloc)FreePool(pixels);
	return 1;
}

static int LuaUefiBootLogo2ProtocolGetBootLogo(lua_State*L){
	UINTN dstx=0,dsty=0,width=0,height=0;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*pixels=NULL;
	GET_PROTO(L,1,proto);
	EFI_STATUS status=proto->proto->GetBootLogo(
		proto->proto,&pixels,
		&dstx,&dsty,&width,&height
	);
	uefi_status_to_lua(L,status);
	if(!pixels)lua_pushnil(L);
	else uefi_data_to_lua(L,FALSE,pixels,0);

	return 2;
}

EDKII_BOOT_LOGO2_PROTOCOL*uefi_lua_to_boot_logo2_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_boot_logo2_protocol_to_lua(lua_State*L,EDKII_BOOT_LOGO2_PROTOCOL*proto){
	struct lua_uefi_boot_logo2_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_boot_logo2_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_BOOT_LOGO2);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiBootLogo2ProtocolMetaTable={
	.name=LUA_UEFI_PROTO_BOOT_LOGO2,
	.reg=(const luaL_Reg[]){
		{"SetBootLogo", LuaUefiBootLogo2ProtocolSetBootLogo},
		{"GetBootLogo", LuaUefiBootLogo2ProtocolGetBootLogo},
		{NULL, NULL}
	},
	.tostring=LuaUefiBootLogo2ProtocolToString,
	.gc=NULL,
};

#endif

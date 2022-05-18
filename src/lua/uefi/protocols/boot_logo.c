/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_BOOT_LOGO "UEFI Boot Logo Protocol"
struct lua_uefi_boot_logo_proto{EFI_BOOT_LOGO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_boot_logo_proto,LUA_UEFI_PROTO_BOOT_LOGO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiBootLogoProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_BOOT_LOGO" %p",proto->proto);
	return 1;
}

static int LuaUefiBootLogoProtocolSetBootLogo(lua_State*L){
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

EFI_BOOT_LOGO_PROTOCOL*uefi_lua_to_boot_logo_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_boot_logo_protocol_to_lua(lua_State*L,EFI_BOOT_LOGO_PROTOCOL*proto){
	struct lua_uefi_boot_logo_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_boot_logo_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_BOOT_LOGO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiBootLogoProtocolMetaTable={
	.name=LUA_UEFI_PROTO_BOOT_LOGO,
	.reg=(const luaL_Reg[]){
		{"SetBootLogo", LuaUefiBootLogoProtocolSetBootLogo},
		{NULL, NULL}
	},
	.tostring=LuaUefiBootLogoProtocolToString,
	.gc=NULL,
};

#endif

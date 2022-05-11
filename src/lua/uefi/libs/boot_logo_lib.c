/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<Library/BootLogoLib.h>
#include"../lua_uefi.h"

int LuaBootLogoLibBootLogoEnableLogo(lua_State*L){
	uefi_status_to_lua(L,BootLogoEnableLogo());
	return 1;
}

int LuaBootLogoLibBootLogoDisableLogo(lua_State*L){
	uefi_status_to_lua(L,BootLogoDisableLogo());
	return 1;
}

int LuaBootLogoLibBootLogoUpdateProgress(lua_State*L){
	CHAR16*title=NULL;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL tfg,tbg,pc;
	uefi_graphics_output_get_pixel(L,1,&tfg);
	uefi_graphics_output_get_pixel(L,2,&tbg);
	lua_arg_get_char16(L,3,false,&title);
	if(!title)return luaL_argerror(L,3,"get argument failed");
	uefi_graphics_output_get_pixel(L,4,&pc);
	UINTN prog=luaL_checkinteger(L,5);
	UINTN prev=luaL_checkinteger(L,6);
	uefi_status_to_lua(L,BootLogoUpdateProgress(
		tfg,tbg,title,pc,prog,prev
	));
	FreePool(title);
	return 1;
}

const luaL_Reg LuaUefiLibraryBootLogoLib[]={
	{"BootLogoEnableLogo",     LuaBootLogoLibBootLogoEnableLogo},
	{"BootLogoDisableLogo",    LuaBootLogoLibBootLogoDisableLogo},
	{"BootLogoUpdateProgress", LuaBootLogoLibBootLogoUpdateProgress},
	{"EnableLogo",             LuaBootLogoLibBootLogoEnableLogo},
	{"DisableLogo",            LuaBootLogoLibBootLogoDisableLogo},
	{"UpdateProgress",         LuaBootLogoLibBootLogoUpdateProgress},
	{NULL,NULL}
};
#endif

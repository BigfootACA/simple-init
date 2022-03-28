/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<Library/UefiBootManagerLib.h>
#include"../lua_uefi.h"

static const char*type_to_str(EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type){
	switch(type){
		case LoadOptionTypeBoot:return "boot";
		case LoadOptionTypeDriver:return "driver";
		case LoadOptionTypeSysPrep:return "sysprep";
		case LoadOptionTypePlatformRecovery:return "platform-recovery";
		default:return NULL;
	}
}

static BOOLEAN str_to_type(const char*str,EFI_BOOT_MANAGER_LOAD_OPTION_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"boot")==0)return LoadOptionTypeBoot;
	else if(AsciiStriCmp(str,"driver")==0)return LoadOptionTypeDriver;
	else if(AsciiStriCmp(str,"sysprep")==0)return LoadOptionTypeSysPrep;
	else if(AsciiStriCmp(str,"platform-recovery")==0)return LoadOptionTypePlatformRecovery;
	else return FALSE;
	return TRUE;
}

int LuaBootLogoLibBootLogoEnableLogo(lua_State*L){
	uefi_status_to_lua(L,BootLogoEnableLogo());
	return 1;
}

int LuaBootLogoLibBootLogoDisableLogo(lua_State*L){
	uefi_status_to_lua(L,BootLogoDisableLogo());
	return 1;
}

int LuaBootLogoLibBootLogoUpdateProgress(lua_State*L){
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL tfg,tbg,pc;
	uefi_graphics_output_get_pixel(L,1,&tfg);
	uefi_graphics_output_get_pixel(L,2,&tbg);
	GET_CHAR16(L,3,title);
	uefi_graphics_output_get_pixel(L,4,&pc);
	UINTN prog=luaL_checkinteger(L,5);
	UINTN prev=luaL_checkinteger(L,6);
	uefi_status_to_lua(L,BootLogoUpdateProgress(
		tfg,tbg,title->string,pc,prog,prev
	));
	return 1;
}

const luaL_Reg LuaUefiLibraryBootLogoLib[]={
	{"BootLogoEnableLogo",     LuaBootLogoLibBootLogoEnableLogo},
	{"BootLogoDisableLogo",    LuaBootLogoLibBootLogoDisableLogo},
	{"BootLogoUpdateProgress", LuaBootLogoLibBootLogoUpdateProgress},
	{NULL,NULL}
};
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiBootManagerLibGetLoadOptions(lua_State*L){
	UINTN cnt=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*opts;
	EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type=0;
	if(!uefi_str_to_load_option_type(luaL_checkstring(L,1),&type))
		return luaL_argerror(L,1,"invalid load option type");
	opts=EfiBootManagerGetLoadOptions(&cnt,type);
	if(opts){
		lua_createtable(L,0,0);
		for(UINTN i=0;i<cnt;i++){
			uefi_bootopt_to_lua(L,&opts[i],TRUE);
			lua_rawseti(L,-2,i+1);
		}
		FreePool(opts);
	}else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootManagerLibNewLoadOption(lua_State*L){
	EFI_BOOT_MANAGER_LOAD_OPTION*lo;
	if(!(lo=AllocateZeroPool(sizeof(EFI_BOOT_MANAGER_LOAD_OPTION))))
		return luaL_error(L,"allocate load option failed");
	uefi_bootopt_to_lua(L,lo,TRUE);
	return 1;
}

static int LuaUefiBootManagerLibInitializeLoadOption(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	CHAR16*desc=NULL;
	GET_BOOT_OPTION(L,1,bo);
	UINTN bn=luaL_checkinteger(L,2);
	EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type=0;
	if(!uefi_str_to_load_option_type(luaL_checkstring(L,3),&type))
		return luaL_argerror(L,1,"invalid load option type");
	UINT32 attr=luaL_checkinteger(L,4);
	lua_arg_get_char16(L,5,false,&desc);
	GET_DEVICE_PATH(L,6,dp);
	if(!desc)return luaL_argerror(L,5,"get argument failed");
	lua_arg_get_data(L,7,true,&data,&ds);
	EFI_STATUS st=EfiBootManagerInitializeLoadOption(
		bo->bo,bn,type,attr,desc,dp->dp,data,data?ds:0
	);
	uefi_status_to_lua(L,st);
	FreePool(desc);
	return 1;
}

static int LuaUefiBootManagerLibVariableToLoadOption(lua_State*L){
	CHAR16*var=NULL;
	GET_BOOT_OPTION(L,1,bo);
	lua_arg_get_char16(L,2,false,&var);
	if(!var)return luaL_argerror(L,2,"get argument failed");
	EFI_STATUS st=EfiBootManagerVariableToLoadOption(var,bo->bo);
	uefi_status_to_lua(L,st);
	FreePool(var);
	return 1;
}

static int LuaUefiBootManagerLibLoadOptionToVariable(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	EFI_STATUS st=EfiBootManagerLoadOptionToVariable(bo->bo);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibAddLoadOptionVariable(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	UINTN pos=luaL_checkinteger(L,2);
	EFI_STATUS st=EfiBootManagerAddLoadOptionVariable(bo->bo,pos);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibDeleteLoadOptionVariable(lua_State*L){
	UINTN bn=luaL_checkinteger(L,1);
	EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type=0;
	if(!uefi_str_to_load_option_type(luaL_checkstring(L,2),&type))
		return luaL_argerror(L,1,"invalid load option type");
	EFI_STATUS st=EfiBootManagerDeleteLoadOptionVariable(bn,type);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibHotkeyBoot(lua_State*L __attribute__((unused))){
	EfiBootManagerHotkeyBoot();
	return 0;
}

static int LuaUefiBootManagerLibRefreshAllBootOption(lua_State*L __attribute__((unused))){
	EfiBootManagerRefreshAllBootOption();
	return 0;
}

static int LuaUefiBootManagerLibBoot(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	EfiBootManagerBoot(bo->bo);
	return 0;
}

static int LuaUefiBootManagerLibGetBootManagerMenu(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	EFI_STATUS st=EfiBootManagerGetBootManagerMenu(bo->bo);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibConnectAll(lua_State*L __attribute__((unused))){
	EfiBootManagerConnectAll();
	return 0;
}

static int LuaUefiBootManagerLibConnectDevicePath(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	GET_HANDLE(L,2,hand);
	EFI_STATUS st=EfiBootManagerConnectDevicePath(dp->dp,hand->hand);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibDisconnectAll(lua_State*L __attribute__((unused))){
	EfiBootManagerDisconnectAll();
	return 0;
}

static int LuaUefiBootManagerLibConnectAllDefaultConsoles(lua_State*L){
	EFI_STATUS st=EfiBootManagerConnectAllDefaultConsoles();
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibGetGopDevicePath(lua_State*L){
	GET_HANDLE(L,1,hand);
	EFI_DEVICE_PATH_PROTOCOL*dp=EfiBootManagerGetGopDevicePath(hand->hand);
	if(dp)uefi_device_path_to_lua(L,dp);
	else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootManagerLibConnectVideoController(lua_State*L){
	GET_HANDLE(L,1,hand);
	EFI_STATUS st=EfiBootManagerConnectVideoController(hand->hand);
	uefi_status_to_lua(L,st);
	return 0;
}

static int LuaUefiBootManagerLibProcessLoadOption(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	EFI_STATUS st=EfiBootManagerProcessLoadOption(bo->bo);
	uefi_status_to_lua(L,st);
	return 1;
}

static int LuaUefiBootManagerLibIsValidLoadOptionVariableName(lua_State*L){
	UINT16 bn=0;
	CHAR16*var=NULL;
	EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type=0;
	lua_arg_get_char16(L,1,false,&var);
	if(!var)return luaL_argerror(L,1,"get argument failed");
	lua_pushboolean(L,EfiBootManagerIsValidLoadOptionVariableName(
		var,&type,&bn
	));
	lua_pushstring(L,uefi_load_option_type_to_str(type));
	lua_pushinteger(L,bn);
	FreePool(var);
	return 3;
}

static int LuaUefiBootManagerLibDispatchDeferredImages(lua_State*L){
	EFI_STATUS st=EfiBootManagerDispatchDeferredImages();
	uefi_status_to_lua(L,st);
	return 1;
}

const luaL_Reg LuaUefiLibraryUefiBootManagerLib[]={
	{"GetLoadOptions",                LuaUefiBootManagerLibGetLoadOptions},
	{"NewLoadOption",                 LuaUefiBootManagerLibNewLoadOption},
	{"InitializeLoadOption",          LuaUefiBootManagerLibInitializeLoadOption},
	{"VariableToLoadOption",          LuaUefiBootManagerLibVariableToLoadOption},
	{"LoadOptionToVariable",          LuaUefiBootManagerLibLoadOptionToVariable},
	{"AddLoadOptionVariable",         LuaUefiBootManagerLibAddLoadOptionVariable},
	{"DeleteLoadOptionVariable",      LuaUefiBootManagerLibDeleteLoadOptionVariable},
	{"HotkeyBoot",                    LuaUefiBootManagerLibHotkeyBoot},
	{"RefreshAllBootOption",          LuaUefiBootManagerLibRefreshAllBootOption},
	{"Boot",                          LuaUefiBootManagerLibBoot},
	{"GetBootManagerMenu",            LuaUefiBootManagerLibGetBootManagerMenu},
	{"ConnectAll",                    LuaUefiBootManagerLibConnectAll},
	{"ConnectDevicePath",             LuaUefiBootManagerLibConnectDevicePath},
	{"DisconnectAll",                 LuaUefiBootManagerLibDisconnectAll},
	{"ConnectAllDefaultConsoles",     LuaUefiBootManagerLibConnectAllDefaultConsoles},
	{"GetGopDevicePath",              LuaUefiBootManagerLibGetGopDevicePath},
	{"ConnectVideoController",        LuaUefiBootManagerLibConnectVideoController},
	{"ProcessLoadOption",             LuaUefiBootManagerLibProcessLoadOption},
	{"IsValidLoadOptionVariableName", LuaUefiBootManagerLibIsValidLoadOptionVariableName},
	{"DispatchDeferredImages",        LuaUefiBootManagerLibDispatchDeferredImages},
	{NULL,NULL}
};
#endif

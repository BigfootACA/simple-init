/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<Library/UefiLib.h>
#include"../lua_uefi.h"

static int LuaUefiLibEfiGetSystemConfigurationTable(lua_State*L){
	EFI_GUID guid;
	VOID*table=NULL;
	EFI_STATUS status;
	lua_arg_get_guid(L,1,false,&guid);
	status=EfiGetSystemConfigurationTable(&guid,&table);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!table)lua_pushnil(L);
	else uefi_data_to_lua(L,FALSE,table,0);
	return 2;
}

static int LuaUefiLibEfiNamedEventSignal(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	lua_arg_get_guid(L,1,false,&guid);
	status=EfiNamedEventSignal(&guid);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLibEfiEventGroupSignal(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	lua_arg_get_guid(L,1,false,&guid);
	status=EfiEventGroupSignal(&guid);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLibEfiTestManagedDevice(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	GET_HANDLE(L,1,ctrl);
	GET_HANDLE(L,2,drv_bind);
	lua_arg_get_guid(L,3,false,&guid);
	status=EfiTestManagedDevice(
		ctrl->hand,
		drv_bind->hand,
		&guid
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLibEfiTestChildHandle(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	GET_HANDLE(L,1,ctrl);
	GET_HANDLE(L,2,child);
	lua_arg_get_guid(L,3,false,&guid);
	status=EfiTestChildHandle(
		ctrl->hand,
		child->hand,
		&guid
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLibIsLanguageSupported(lua_State*L){
	EFI_STATUS status;
	const char*supported=luaL_checkstring(L,1);
	const char*target=luaL_checkstring(L,1);
	status=IsLanguageSupported(supported,target);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiLibGetVariable2(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	VOID*data=NULL;
	UINTN size=0;
	CHAR16*name=NULL;
	lua_arg_get_char16(L,1,false,&name);
	lua_arg_get_guid(L,2,false,&guid);
	if(!name)return luaL_argerror(L,1,"get argument failed");
	status=GetVariable2(name,&guid,&data,&size);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!data)lua_pushnil(L);
	else uefi_data_to_lua(L,TRUE,data,size);
	FreePool(name);
	return 2;
}

static int LuaUefiLibGetEfiGlobalVariable2(lua_State*L){
	EFI_STATUS status;
	VOID*data=NULL;
	UINTN size=0;
	CHAR16*name=NULL;
	lua_arg_get_char16(L,1,false,&name);
	if(!name)return luaL_argerror(L,1,"get argument failed");
	status=GetEfiGlobalVariable2(name,&data,&size);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!data)lua_pushnil(L);
	else uefi_data_to_lua(L,TRUE,data,size);
	FreePool(name);
	return 2;
}

static int LuaUefiLibGetVariable3(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	VOID*data=NULL;
	UINTN size=0;
	UINT32 attr=0;
	CHAR16*name=NULL;
	lua_arg_get_char16(L,1,false,&name);
	lua_arg_get_guid(L,2,false,&guid);
	if(!name)return luaL_argerror(L,1,"get argument failed");
	status=GetVariable3(name,&guid,&data,&size,&attr);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!data)lua_pushnil(L);
	else uefi_data_to_lua(L,TRUE,data,size);
	lua_pushinteger(L,attr);
	FreePool(name);
	return 3;
}

static int LuaUefiLibGetBestLanguage(lua_State*L){
	if(!lua_isnoneornil(L,9))
		return luaL_argerror(L,9,"too many arguments");
	lua_pushstring(L,GetBestLanguage(
		luaL_checkstring(L,1),
		luaL_checkinteger(L,2),
		luaL_checkstring(L,3),
		luaL_optstring(L,4,NULL),
		luaL_optstring(L,5,NULL),
		luaL_optstring(L,6,NULL),
		luaL_optstring(L,7,NULL),
		luaL_optstring(L,8,NULL),
		NULL
	));
	return 1;
}

static int LuaUefiLibEfiSignalEventReadyToBoot(lua_State*L){
	EfiSignalEventReadyToBoot();
	return 0;
}

static int LuaUefiLibEfiSignalEventLegacyBoot(lua_State*L){
	EfiSignalEventLegacyBoot();
	return 0;
}

static int LuaUefiLibPrintXY(lua_State*L){
	CHAR16*str=NULL;
	UINTN px=0,py=0,res=0;
	BOOLEAN hfg=FALSE,hbg=FALSE;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL fg,bg;
	px=luaL_checkinteger(L,1);
	py=luaL_checkinteger(L,2);
	if((hfg=!lua_isnoneornil(L,3)))
		uefi_graphics_output_get_pixel(L,3,&fg);
	if((hbg=!lua_isnoneornil(L,4)))
		uefi_graphics_output_get_pixel(L,4,&bg);
	lua_arg_get_char16(L,5,false,&str);
	if(!str)return luaL_argerror(L,5,"get argument failed");
	res=PrintXY(
		px,py,
		hfg?&fg:NULL,
		hbg?&bg:NULL,
		L"%s",str
	);
	FreePool(str);
	lua_pushinteger(L,res);
	return 1;
}

static int LuaUefiLibAsciiPrintXY(lua_State*L){
	UINTN px=0,py=0,res=0;
	BOOLEAN hfg=FALSE,hbg=FALSE;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL fg,bg;
	px=luaL_checkinteger(L,1);
	py=luaL_checkinteger(L,2);
	if((hfg=!lua_isnoneornil(L,3)))
		uefi_graphics_output_get_pixel(L,3,&fg);
	if((hbg=!lua_isnoneornil(L,4)))
		uefi_graphics_output_get_pixel(L,4,&bg);
	const char*str=luaL_checkstring(L,5);
	res=AsciiPrintXY(
		px,py,
		hfg?&fg:NULL,
		hbg?&bg:NULL,
		"%a",str
	);
	lua_pushinteger(L,res);
	return 1;
}

static int LuaUefiLibEfiLocateProtocolBuffer(lua_State*L){
	EFI_GUID guid;
	EFI_STATUS status;
	VOID**data=NULL;
	UINTN size=0;
	lua_arg_get_guid(L,1,false,&guid);
	status=EfiLocateProtocolBuffer(&guid,&size,&data);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!data)lua_pushnil(L);
	else{
		lua_createtable(L,0,0);
		for(UINTN i=0;data[i];i++){
			uefi_data_to_protocol(L,&guid,data[i],FALSE);
			lua_rawseti(L,-2,i+1);
		}
	}
	return 2;
}

static int LuaUefiLibEfiOpenFileByDevicePath(lua_State*L){
	UINT64 mode=0,attr=0;
	EFI_STATUS status;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_DEVICE_PATH(L,1,dp);
	uefi_file_protocol_get_mode(L,2,&mode,FALSE);
	uefi_file_protocol_get_attr(L,3,&attr,TRUE);
	status=EfiOpenFileByDevicePath(&dp->dp,&fp,mode,attr);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	return 2;
}

const luaL_Reg LuaUefiLibraryUefiLib[]={
	{"EfiGetSystemConfigurationTable", LuaUefiLibEfiGetSystemConfigurationTable},
	{"EfiNamedEventSignal",            LuaUefiLibEfiNamedEventSignal},
	{"EfiEventGroupSignal",            LuaUefiLibEfiEventGroupSignal},
	{"EfiTestManagedDevice",           LuaUefiLibEfiTestManagedDevice},
	{"EfiTestChildHandle",             LuaUefiLibEfiTestChildHandle},
	{"IsLanguageSupported",            LuaUefiLibIsLanguageSupported},
	{"GetVariable2",                   LuaUefiLibGetVariable2},
	{"GetEfiGlobalVariable2",          LuaUefiLibGetEfiGlobalVariable2},
	{"GetVariable3",                   LuaUefiLibGetVariable3},
	{"GetBestLanguage",                LuaUefiLibGetBestLanguage},
	{"EfiSignalEventReadyToBoot",      LuaUefiLibEfiSignalEventReadyToBoot},
	{"EfiSignalEventLegacyBoot",       LuaUefiLibEfiSignalEventLegacyBoot},
	{"PrintXY",                        LuaUefiLibPrintXY},
	{"AsciiPrintXY",                   LuaUefiLibAsciiPrintXY},
	{"EfiLocateProtocolBuffer",        LuaUefiLibEfiLocateProtocolBuffer},
	{"EfiOpenFileByDevicePath",        LuaUefiLibEfiOpenFileByDevicePath},
	{NULL,NULL}
};
#endif

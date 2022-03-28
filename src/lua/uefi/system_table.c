/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"lua_uefi.h"

static void GetFirmwareVendor(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_char16_16_to_lua(L,FALSE,st->FirmwareVendor);
}

static void GetFirmwareRevision(lua_State*L,EFI_SYSTEM_TABLE*st){
	lua_pushinteger(L,st->FirmwareRevision);
}

static void GetConsoleInHandle(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_handle_to_lua(L,st->ConsoleInHandle);
}

static void GetConIn(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_simple_text_input_protocol_to_lua(L,st->ConIn);
}

static void GetConsoleOutHandle(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_handle_to_lua(L,st->ConsoleOutHandle);
}

static void GetConOut(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_simple_text_output_protocol_to_lua(L,st->ConOut);
}

static void GetStandardErrorHandle(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_handle_to_lua(L,st->StandardErrorHandle);
}

static void GetStdErr(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_simple_text_output_protocol_to_lua(L,st->StdErr);
}

static void GetRuntimeServices(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_rt_to_lua(L,st->RuntimeServices);
}

static void GetBootServices(lua_State*L,EFI_SYSTEM_TABLE*st){
	uefi_bs_to_lua(L,st->BootServices);
}

static void GetConfigurationTable(lua_State*L,EFI_SYSTEM_TABLE*st){
	lua_createtable(L,0,0);
	for(UINTN i=0;i<st->NumberOfTableEntries;i++){
		lua_createtable(L,0,0);
		lua_pushliteral(L,"VendorGuid");
		uefi_guid_to_lua(L,&st->ConfigurationTable[i].VendorGuid);
		lua_settable(L,-3);
		lua_pushliteral(L,"VendorTable");
		uefi_data_to_lua(L,FALSE,&st->ConfigurationTable[i].VendorTable,0);
		lua_settable(L,-3);
		lua_rawseti(L,-2,i+1);
	}
}

typedef void(*st_get)(lua_State*,EFI_SYSTEM_TABLE*);
static struct st_item{
	const char*name;
	st_get get;
}reg[]={
	{"FirmwareVendor",      GetFirmwareVendor},
	{"FirmwareRevision",    GetFirmwareRevision},
	{"ConsoleInHandle",     GetConsoleInHandle},
	{"ConIn",               GetConIn},
	{"ConsoleOutHandle",    GetConsoleOutHandle},
	{"ConOut",              GetConOut},
	{"StandardErrorHandle", GetStandardErrorHandle},
	{"StdErr",              GetStdErr},
	{"RuntimeServices",     GetRuntimeServices},
	{"BootServices",        GetBootServices},
	{"ConfigurationTable",  GetConfigurationTable},
	{NULL, NULL}
};

void uefi_st_to_lua(lua_State*L,EFI_SYSTEM_TABLE*st){
	if(!st){
		lua_pushnil(L);
		return;
	}
	lua_createtable(L,0,0);
	for(int i=0;reg[i].name;i++){
		lua_pushstring(L,reg[i].name);
		reg[i].get(L,st);
		lua_settable(L,-3);
	}
}

#endif

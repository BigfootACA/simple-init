/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"lua_uefi.h"
#include"system.h"
#define DECL_META_TABLE(name)extern struct lua_uefi_meta_table LuaUefi##name##MetaTable;
#define DECL_META_TABLE_PROTO(name)DECL_META_TABLE(name##Protocol)

DECL_META_TABLE(BS);
DECL_META_TABLE(Char16);
DECL_META_TABLE(DevicePath);
DECL_META_TABLE(Event);
DECL_META_TABLE(Guid);
DECL_META_TABLE(Hand);
DECL_META_TABLE(RT);
DECL_META_TABLE(Status);
DECL_META_TABLE(Time);
DECL_META_TABLE(InputKey);
DECL_META_TABLE(BootOption);
DECL_META_TABLE_PROTO(AbsolutePointer);
DECL_META_TABLE_PROTO(AcpiTable);
DECL_META_TABLE_PROTO(BlockIO);
DECL_META_TABLE_PROTO(BootLogo);
DECL_META_TABLE_PROTO(BootLogo2);
DECL_META_TABLE_PROTO(BootManagerPolicy);
DECL_META_TABLE_PROTO(ComponentName);
DECL_META_TABLE_PROTO(ComponentName2);
DECL_META_TABLE_PROTO(DeferredImageLoad);
DECL_META_TABLE_PROTO(DiskInfo);
DECL_META_TABLE_PROTO(DiskIO);
DECL_META_TABLE_PROTO(File);
DECL_META_TABLE_PROTO(GraphicsOutput);
DECL_META_TABLE_PROTO(LoadedImage);
DECL_META_TABLE_PROTO(PartitionInfo);
DECL_META_TABLE_PROTO(RamDisk);
DECL_META_TABLE_PROTO(SecurityArch);
DECL_META_TABLE_PROTO(Security2Arch);
DECL_META_TABLE_PROTO(SerialIO);
DECL_META_TABLE_PROTO(SimpleFileSystem);
DECL_META_TABLE_PROTO(SimplePointer);
DECL_META_TABLE_PROTO(SimpleTextIn);
DECL_META_TABLE_PROTO(SimpleTextOut);
DECL_META_TABLE_PROTO(TimeStamp);
static struct lua_uefi_meta_table*meta[]={
	&LuaUefiBSMetaTable,
	&LuaUefiChar16MetaTable,
	&LuaUefiDevicePathMetaTable,
	&LuaUefiEventMetaTable,
	&LuaUefiGuidMetaTable,
	&LuaUefiHandMetaTable,
	&LuaUefiRTMetaTable,
	&LuaUefiStatusMetaTable,
	&LuaUefiTimeMetaTable,
	&LuaUefiInputKeyMetaTable,
	&LuaUefiBootOptionMetaTable,
	&LuaUefiAbsolutePointerProtocolMetaTable,
	&LuaUefiAcpiTableProtocolMetaTable,
	&LuaUefiBlockIOProtocolMetaTable,
	&LuaUefiBootLogoProtocolMetaTable,
	&LuaUefiBootLogo2ProtocolMetaTable,
	&LuaUefiBootManagerPolicyProtocolMetaTable,
	&LuaUefiComponentNameProtocolMetaTable,
	&LuaUefiComponentName2ProtocolMetaTable,
	&LuaUefiDeferredImageLoadProtocolMetaTable,
	&LuaUefiDiskInfoProtocolMetaTable,
	&LuaUefiDiskIOProtocolMetaTable,
	&LuaUefiFileProtocolMetaTable,
	&LuaUefiGraphicsOutputProtocolMetaTable,
	&LuaUefiLoadedImageProtocolMetaTable,
	&LuaUefiPartitionInfoProtocolMetaTable,
	&LuaUefiRamDiskProtocolMetaTable,
	&LuaUefiSecurityArchProtocolMetaTable,
	&LuaUefiSecurity2ArchProtocolMetaTable,
	&LuaUefiSerialIOProtocolMetaTable,
	&LuaUefiSimpleFileSystemProtocolMetaTable,
	&LuaUefiSimplePointerProtocolMetaTable,
	&LuaUefiSimpleTextInProtocolMetaTable,
	&LuaUefiSimpleTextOutProtocolMetaTable,
	&LuaUefiTimeStampProtocolMetaTable,
	NULL
};

BOOLEAN uefi_str_to_tpl(IN CONST CHAR8*str,OUT EFI_TPL*tpl){
	if(!str||!tpl)return FALSE;
	if(AsciiStriCmp(str,"application")==0)
		*tpl=TPL_APPLICATION;
	else if(AsciiStriCmp(str,"callback")==0)
		*tpl=TPL_CALLBACK;
	else if(AsciiStriCmp(str,"notify")==0)
		*tpl=TPL_NOTIFY;
	else if(AsciiStriCmp(str,"high-level")==0)
		*tpl=TPL_HIGH_LEVEL;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_event_type(IN CONST CHAR8*str,OUT UINT32*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"signal-virtual-address-change")==0)
		*type=EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
	else if(AsciiStriCmp(str,"signal-exit-boot-services")==0)
		*type=EVT_SIGNAL_EXIT_BOOT_SERVICES;
	else if(AsciiStriCmp(str,"notify-signal")==0)
		*type=EVT_NOTIFY_SIGNAL;
	else if(AsciiStriCmp(str,"notify-wait")==0)
		*type=EVT_NOTIFY_WAIT;
	else if(AsciiStriCmp(str,"runtime")==0)
		*type=EVT_RUNTIME;
	else if(AsciiStriCmp(str,"timer")==0)
		*type=EVT_TIMER;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_reset_type(IN CONST CHAR8*str,OUT EFI_RESET_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"platform-specific")==0)
		*type=EfiResetPlatformSpecific;
	else if(AsciiStriCmp(str,"shutdown")==0)
		*type=EfiResetShutdown;
	else if(AsciiStriCmp(str,"warm")==0)
		*type=EfiResetWarm;
	else if(AsciiStriCmp(str,"cold")==0)
		*type=EfiResetCold;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_memory_type(IN CONST CHAR8*str,OUT EFI_MEMORY_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"reserved-memory-type")==0)
		*type=EfiReservedMemoryType;
	else if(AsciiStriCmp(str,"loader-code")==0)
		*type=EfiLoaderCode;
	else if(AsciiStriCmp(str,"loader-data")==0)
		*type=EfiLoaderData;
	else if(AsciiStriCmp(str,"boot-services-code")==0)
		*type=EfiBootServicesCode;
	else if(AsciiStriCmp(str,"boot-services-data")==0)
		*type=EfiBootServicesData;
	else if(AsciiStriCmp(str,"runtime-services-code")==0)
		*type=EfiRuntimeServicesCode;
	else if(AsciiStriCmp(str,"runtime-services-data")==0)
		*type=EfiRuntimeServicesData;
	else if(AsciiStriCmp(str,"conventional-memory")==0)
		*type=EfiConventionalMemory;
	else if(AsciiStriCmp(str,"unusable-memory")==0)
		*type=EfiUnusableMemory;
	else if(AsciiStriCmp(str,"acpi-reclaim-memory")==0)
		*type=EfiACPIReclaimMemory;
	else if(AsciiStriCmp(str,"acpi-memory-nvs")==0)
		*type=EfiACPIMemoryNVS;
	else if(AsciiStriCmp(str,"memory-mapped-io")==0)
		*type=EfiMemoryMappedIO;
	else if(AsciiStriCmp(str,"memory-mapped-io-port-space")==0)
		*type=EfiMemoryMappedIOPortSpace;
	else if(AsciiStriCmp(str,"pal-code")==0)
		*type=EfiPalCode;
	else if(AsciiStriCmp(str,"persistent-memory")==0)
		*type=EfiPersistentMemory;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_load_option_type(const char*str,EFI_BOOT_MANAGER_LOAD_OPTION_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"boot")==0)return LoadOptionTypeBoot;
	else if(AsciiStriCmp(str,"driver")==0)return LoadOptionTypeDriver;
	else if(AsciiStriCmp(str,"sysprep")==0)return LoadOptionTypeSysPrep;
	else if(AsciiStriCmp(str,"platform-recovery")==0)return LoadOptionTypePlatformRecovery;
	else return FALSE;
	return TRUE;
}

const char*uefi_memory_type_to_str(EFI_MEMORY_TYPE type){
	switch(type){
		case EfiReservedMemoryType:return "reserved-memory-type";
		case EfiLoaderCode:return "loader-code";
		case EfiLoaderData:return "loader-data";
		case EfiBootServicesCode:return "boot-services-code";
		case EfiBootServicesData:return "boot-services-data";
		case EfiRuntimeServicesCode:return "runtime-services-code";
		case EfiRuntimeServicesData:return "runtime-services-data";
		case EfiConventionalMemory:return "conventional-memory";
		case EfiUnusableMemory:return "unusable-memory";
		case EfiACPIReclaimMemory:return "acpi-reclaim-memory";
		case EfiACPIMemoryNVS:return "acpi-memory-nvs";
		case EfiMemoryMappedIO:return "memory-mapped-io";
		case EfiMemoryMappedIOPortSpace:return "memory-mapped-io-port-space";
		case EfiPalCode:return "pal-code";
		case EfiPersistentMemory:return "persistent-memory";
		default:return NULL;
	}
}

const char*uefi_load_option_type_to_str(EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type){
	switch(type){
		case LoadOptionTypeBoot:return "boot";
		case LoadOptionTypeDriver:return "driver";
		case LoadOptionTypeSysPrep:return "sysprep";
		case LoadOptionTypePlatformRecovery:return "platform-recovery";
		default:return NULL;
	}
}

void uefi_lua_check_status(lua_State*L,EFI_STATUS st){
	lua_getglobal(L,"uefi_error");
	bool err=lua_toboolean(L,-1);
	lua_pop(L,1);
	if(err&&EFI_ERROR(st))luaL_error(
		L,"uefi call failed: %s",
		efi_status_to_string(st)
	);
}

static int LuaUefiLibStringToGuid(lua_State*L){
	EFI_GUID guid;
	uefi_str_to_guid(luaL_checkstring(L,1),&guid);
	uefi_guid_to_lua(L,&guid);
	return 1;
}

static int LuaUefiLibStringToStatus(lua_State*L){
	uefi_str_status_to_lua(L,luaL_checkstring(L,1));
	return 1;
}

static int LuaUefiLibStringToChar16(lua_State*L){
	uefi_char16_a8_to_lua(L,(CHAR8*)luaL_checkstring(L,1));
	return 1;
}

static int LuaUefiLibStringToData(lua_State*L){
	const char*str=luaL_checkstring(L,1);
	uefi_data_dup_to_lua(L,(CHAR8*)str,AsciiStrSize(str));
	return 1;
}

static int LuaUefiLibGetLib(lua_State*L){
	const luaL_Reg*lib=uefi_get_lib(luaL_checkstring(L,1));
	if(!lib)lua_pushnil(L);
	else{
		lua_createtable(L,0,0);
		luaL_setfuncs(L,lib,0);
	}
	return 1;
}

static int LuaUefiLibReboot(lua_State*L){
	enum reboot_cmd cmd;
	const char*str=luaL_checkstring(L,1);
	const char*data=luaL_optstring(L,2,NULL);
	if(AsciiStriCmp(str,"halt")==0)cmd=REBOOT_HALT;
	else if(AsciiStriCmp(str,"kexec")==0)cmd=REBOOT_KEXEC;
	else if(AsciiStriCmp(str,"poweroff")==0)cmd=REBOOT_POWEROFF;
	else if(AsciiStriCmp(str,"shutdown")==0)cmd=REBOOT_POWEROFF;
	else if(AsciiStriCmp(str,"suspend")==0)cmd=REBOOT_SUSPEND;
	else if(AsciiStriCmp(str,"restart")==0)cmd=REBOOT_RESTART;
	else if(AsciiStriCmp(str,"reboot")==0)cmd=REBOOT_RESTART;
	else if(AsciiStriCmp(str,"warm")==0)cmd=REBOOT_WARM;
	else if(AsciiStriCmp(str,"cold")==0)cmd=REBOOT_COLD;
	else if(AsciiStriCmp(str,"data")==0)cmd=REBOOT_DATA;
	else if(AsciiStriCmp(str,"recovery")==0)cmd=REBOOT_RECOVERY;
	else if(AsciiStriCmp(str,"fastboot")==0)cmd=REBOOT_FASTBOOT;
	else if(AsciiStriCmp(str,"bootloader")==0)cmd=REBOOT_FASTBOOT;
	else if(AsciiStriCmp(str,"edl")==0)cmd=REBOOT_EDL;
	else return luaL_argerror(L,1,"invalid reboot command");
	lua_pushinteger(L,adv_reboot(cmd,(char*)data));
	return 1;
}

static const luaL_Reg uefi_lib[]={
	{"Reboot",         LuaUefiLibReboot},
	{"Lib",            LuaUefiLibGetLib},
	{"Library",        LuaUefiLibGetLib},
	{"GetLib",         LuaUefiLibGetLib},
	{"GetLibrary",     LuaUefiLibGetLib},
	{"Guid",           LuaUefiLibStringToGuid},
	{"GetGuid",        LuaUefiLibStringToGuid},
	{"StringToGuid",   LuaUefiLibStringToGuid},
	{"StringToStatus", LuaUefiLibStringToStatus},
	{"Status",         LuaUefiLibStringToStatus},
	{"StringToChar16", LuaUefiLibStringToChar16},
	{"Char16",         LuaUefiLibStringToChar16},
	{"StringToData",   LuaUefiLibStringToData},
	{"Data",           LuaUefiLibStringToData},
	{NULL,NULL}
};

LUAMOD_API int luaopen_uefi(lua_State*L){
	luaL_newlib(L,uefi_lib);
	for(size_t i=0;meta[i];i++){
		xlua_create_metatable(
			L,
			meta[i]->name,
			meta[i]->reg,
			meta[i]->tostring,
			meta[i]->gc
		);
	}
	lua_pushliteral(L,"gBS");
	uefi_bs_to_lua(L,gBS);
	lua_settable(L,-3);
	lua_pushliteral(L,"gRT");
	uefi_rt_to_lua(L,gRT);
	lua_settable(L,-3);
	lua_pushliteral(L,"gST");
	uefi_st_to_lua(L,gST);
	lua_settable(L,-3);
	lua_pushliteral(L,"gImageHandle");
	uefi_handle_to_lua(L,gImageHandle);
	lua_settable(L,-3);
	return 1;
}
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"lua_uefi.h"

typedef void(*to_protocol)(lua_State*,VOID*);
#define DECL_PROT(_name,_guid,_hand) {.name=(_name),.guid=&(_guid),.hand=(const to_protocol)(_hand)},
#define DECL_GUID(_name,_guid)DECL_PROT(_name,_guid,NULL)
struct guid_info{
	const char*name;
	const EFI_GUID*guid;
	const to_protocol hand;
}infos[]={
	DECL_PROT("AbsolutePointerProtocol",     gEfiAbsolutePointerProtocolGuid,     uefi_absolute_pointer_protocol_to_lua)
	DECL_PROT("AcpiTableProtocol",           gEfiAcpiTableProtocolGuid,           uefi_acpi_table_protocol_to_lua)
	DECL_PROT("BlockIoProtocol",             gEfiBlockIoProtocolGuid,             uefi_block_io_protocol_to_lua)
	DECL_PROT("BootLogoProtocol",            gEfiBootLogoProtocolGuid,            uefi_boot_logo_protocol_to_lua)
	DECL_PROT("BootLogo2Protocol",           gEdkiiBootLogo2ProtocolGuid,         uefi_boot_logo2_protocol_to_lua)
	DECL_PROT("BootManagerPolicyProtocol",   gEfiBootManagerPolicyProtocolGuid,   uefi_boot_manager_policy_protocol_to_lua)
	DECL_PROT("ComponentNameProtocol",       gEfiComponentNameProtocolGuid,       uefi_component_name2_protocol_to_lua)
	DECL_PROT("ComponentName2Protocol",      gEfiComponentName2ProtocolGuid,      uefi_component_name_protocol_to_lua)
	DECL_PROT("DeferredImageLoadProtocol",   gEfiDeferredImageLoadProtocolGuid,   uefi_deferred_image_load_protocol_to_lua)
	DECL_PROT("DevicePathProtocol",          gEfiDevicePathProtocolGuid,          uefi_device_path_to_lua)
	DECL_PROT("DiskInfoProtocol",            gEfiDiskInfoProtocolGuid,            uefi_disk_info_protocol_to_lua)
	DECL_PROT("DiskIoProtocol",              gEfiDiskIoProtocolGuid,              uefi_disk_io_protocol_to_lua)
	DECL_PROT("GraphicsOutputProtocol",      gEfiGraphicsOutputProtocolGuid,      uefi_graphics_output_protocol_to_lua)
	DECL_PROT("LoadedImage",                 gEfiLoadedImageProtocolGuid,         uefi_loaded_image_protocol_to_lua)
	DECL_PROT("PartitionInfoProtocol",       gEfiPartitionInfoProtocolGuid,       uefi_partition_info_protocol_to_lua)
	DECL_PROT("RamDiskProtocol",             gEfiRamDiskProtocolGuid,             uefi_ramdisk_protocol_to_lua)
	DECL_PROT("SecurityArchProtocol",        gEfiSecurityArchProtocolGuid,        uefi_security_arch_protocol_to_lua)
	DECL_PROT("Security2ArchProtocol",       gEfiSecurity2ArchProtocolGuid,       uefi_security2_arch_protocol_to_lua)
	DECL_PROT("SerialIoProtocol",            gEfiSerialIoProtocolGuid,            uefi_serial_io_protocol_to_lua)
	DECL_PROT("SimpleFileSystemProtocol",    gEfiSimpleFileSystemProtocolGuid,    uefi_simple_file_system_protocol_to_lua)
	DECL_PROT("SimplePointerProtocol",       gEfiSimplePointerProtocolGuid,       uefi_simple_pointer_protocol_to_lua)
	DECL_PROT("SimpleTextInProtocol",        gEfiSimpleTextInProtocolGuid,        uefi_simple_text_input_protocol_to_lua)
	DECL_PROT("SimpleTextOutProtocol",       gEfiSimpleTextOutProtocolGuid,       uefi_simple_text_output_protocol_to_lua)
	DECL_PROT("TimestampProtocol",           gEfiTimestampProtocolGuid,           uefi_timestamp_protocol_to_lua)
	DECL_GUID("DiskInfoIdeInterface",        gEfiDiskInfoIdeInterfaceGuid)
	DECL_GUID("DiskInfoScsiInterface",       gEfiDiskInfoScsiInterfaceGuid)
	DECL_GUID("DiskInfoUsbInterface",        gEfiDiskInfoUsbInterfaceGuid)
	DECL_GUID("DiskInfoAhciInterface",       gEfiDiskInfoAhciInterfaceGuid)
	DECL_GUID("DiskInfoNvmeInterface",       gEfiDiskInfoNvmeInterfaceGuid)
	DECL_GUID("DiskInfoUfsInterface",        gEfiDiskInfoUfsInterfaceGuid)
	DECL_GUID("DiskInfoSdMmcInterface",      gEfiDiskInfoSdMmcInterfaceGuid)
	DECL_GUID("FileInfo",                    gEfiFileInfoGuid)
	DECL_GUID("FileSystemInfo",              gEfiFileSystemInfoGuid)
	DECL_GUID("FileSystemVolumeLabelInfoId", gEfiFileSystemVolumeLabelInfoIdGuid)
	DECL_GUID("BootManagerPolicyConsole",    gEfiBootManagerPolicyConsoleGuid)
	DECL_GUID("BootManagerPolicyNetwork",    gEfiBootManagerPolicyNetworkGuid)
	DECL_GUID("BootManagerPolicyConnectAll", gEfiBootManagerPolicyConnectAllGuid)
	DECL_GUID("VirtualDisk",                 gEfiVirtualDiskGuid)
	DECL_GUID("VirtualCd",                   gEfiVirtualCdGuid)
	DECL_GUID("PersistentVirtualDisk",       gEfiPersistentVirtualDiskGuid)
	DECL_GUID("PersistentVirtualCd",         gEfiPersistentVirtualCdGuid)
	DECL_GUID("FdtTable",                    gFdtTableGuid)
	{NULL,NULL,NULL}
};

void uefi_data_to_protocol(lua_State*L,EFI_GUID*guid,void*proto,BOOLEAN nil){
	for(size_t i=0;infos[i].name;i++){
		if(!infos[i].name)continue;
		if(!infos[i].guid)continue;
		if(!infos[i].hand)continue;
		if(!CompareGuid(guid,infos[i].guid))continue;
		infos[i].hand(L,proto);
		return;
	}
	if(nil)lua_pushnil(L);
	else uefi_data_to_lua(L,FALSE,proto,0);
}

const char*uefi_guid_to_str(const EFI_GUID*guid){
	for(size_t i=0;infos[i].name;i++){
		if(!infos[i].name)continue;
		if(!infos[i].guid)continue;
		if(!CompareGuid(guid,infos[i].guid))continue;
		return infos[i].name;
	}
	return NULL;
}

BOOLEAN uefi_str_to_guid(const char*str,EFI_GUID*out){
	ZeroMem(out,sizeof(EFI_GUID));
	for(size_t i=0;infos[i].name;i++){
		if(!infos[i].name)continue;
		if(!infos[i].guid)continue;
		if(AsciiStriCmp(str,infos[i].name)!=0)continue;
		CopyGuid(out,infos[i].guid);
		return TRUE;
	}
	return !EFI_ERROR(AsciiStrToGuid(str,out));
}

#endif

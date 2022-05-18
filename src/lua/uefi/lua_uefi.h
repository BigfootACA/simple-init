/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#ifndef _LUA_UEFI_H
#define _LUA_UEFI_H
#include"list.h"
#include"xlua.h"
#include"uefi.h"
#include"logger.h"
#include"string.h"
#include"compatible.h"
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/UefiBootManagerLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Protocol/AbsolutePointer.h>
#include<Protocol/AcpiTable.h>
#include<Protocol/BlockIo.h>
#include<Protocol/BootLogo.h>
#include<Protocol/BootLogo2.h>
#include<Protocol/BootManagerPolicy.h>
#include<Protocol/ComponentName.h>
#include<Protocol/ComponentName2.h>
#include<Protocol/DeferredImageLoad.h>
#include<Protocol/DevicePath.h>
#include<Protocol/DiskInfo.h>
#include<Protocol/DiskIo.h>
#include<Protocol/GraphicsOutput.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/RamDisk.h>
#include<Protocol/Security.h>
#include<Protocol/Security2.h>
#include<Protocol/SerialIo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Protocol/SimplePointer.h>
#include<Protocol/SimpleTextIn.h>
#include<Protocol/SimpleTextOut.h>
#include<Protocol/Timestamp.h>
#include<Guid/FileInfo.h>
#include<Guid/FileSystemInfo.h>
#include<Guid/FileSystemVolumeLabelInfo.h>
#define TAG "uefi"

#define LUA_UEFI_BS                       "UEFI Boot Service Table"
#define LUA_UEFI_RT                       "UEFI Runtime Service Table"
#define LUA_UEFI_TIME                     "UEFI Time"
#define LUA_UEFI_HANDLE                   "UEFI Handle"
#define LUA_UEFI_STATUS                   "UEFI Status"
#define LUA_UEFI_DEVICE_PATH              "UEFI Device Path"
#define LUA_UEFI_CHAR16                   "UEFI 16-bit String"
#define LUA_UEFI_GUID                     "UEFI GUID"
#define LUA_UEFI_EVENT                    "UEFI Event"
#define LUA_UEFI_INPUT_KEY                "UEFI Input Key"
#define LUA_UEFI_BOOT_OPTION              "UEFI Boot Option"

#define OPT_BS(L,n,var)          OPT_UDATA(L,n,var,lua_uefi_bs_data,LUA_UEFI_BS)
#define OPT_RT(L,n,var)          OPT_UDATA(L,n,var,lua_uefi_rt_data,LUA_UEFI_RT)
#define OPT_TIME(L,n,var)        OPT_UDATA(L,n,var,lua_uefi_time_data,LUA_UEFI_TIME)
#define OPT_HANDLE(L,n,var)      OPT_UDATA(L,n,var,lua_uefi_hand_data,LUA_UEFI_HANDLE)
#define OPT_STATUS(L,n,var)      OPT_UDATA(L,n,var,lua_uefi_status_data,LUA_UEFI_STATUS)
#define OPT_DEVICE_PATH(L,n,var) OPT_UDATA(L,n,var,lua_uefi_device_path_data,LUA_UEFI_DEVICE_PATH)
#define OPT_EVENT(L,n,var)       OPT_UDATA(L,n,var,lua_uefi_event_data,LUA_UEFI_EVENT)
#define OPT_INPUT_KEY(L,n,var)   OPT_UDATA(L,n,var,lua_uefi_input_key_data,LUA_UEFI_INPUT_KEY)
#define OPT_BOOT_OPTION(L,n,var) OPT_UDATA(L,n,var,lua_uefi_bootopt_data,LUA_UEFI_BOOT_OPTION)

#define GET_BS(L,n,var)          OPT_BS(L,n,var);          CHECK_NULL(L,n,var)
#define GET_RT(L,n,var)          OPT_RT(L,n,var);          CHECK_NULL(L,n,var)
#define GET_TIME(L,n,var)        OPT_TIME(L,n,var);        CHECK_NULL(L,n,var)
#define GET_HANDLE(L,n,var)      OPT_HANDLE(L,n,var);      CHECK_NULL(L,n,var)
#define GET_STATUS(L,n,var)      OPT_STATUS(L,n,var);      CHECK_NULL(L,n,var)
#define GET_DEVICE_PATH(L,n,var) OPT_DEVICE_PATH(L,n,var); CHECK_NULL(L,n,var)
#define GET_EVENT(L,n,var)       OPT_EVENT(L,n,var);       CHECK_NULL(L,n,var)
#define GET_INPUT_KEY(L,n,var)   OPT_INPUT_KEY(L,n,var);   CHECK_NULL(L,n,var)
#define GET_BOOT_OPTION(L,n,var) OPT_BOOT_OPTION(L,n,var); CHECK_NULL(L,n,var)

#define add_str_arr(L,i,str) lua_pushstring((L),(str));lua_rawseti((L),-2,(i))
#define nadd_str_arr(L,i,str) (i)++;add_str_arr(L,i,str)

struct lua_uefi_meta_table{
	char*name;
	const luaL_Reg*reg;
	const lua_CFunction tostring;
	const lua_CFunction gc;
};
struct lua_uefi_bs_data{EFI_BOOT_SERVICES*bs;};
struct lua_uefi_rt_data{EFI_RUNTIME_SERVICES*rt;};
struct lua_uefi_time_data{EFI_TIME time;};
struct lua_uefi_input_key_data{EFI_INPUT_KEY key;};
struct lua_uefi_hand_data{EFI_HANDLE hand;};
struct lua_uefi_status_data{EFI_STATUS st;};
struct lua_uefi_device_path_data{EFI_DEVICE_PATH_PROTOCOL*dp;};
struct lua_uefi_guid_data{EFI_GUID guid;};
struct lua_uefi_bootopt_data{
	BOOLEAN allocated;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
};
struct lua_uefi_char16_data{
	BOOLEAN allocated;
	CHAR16*string;
};
struct lua_uefi_event_extra{
	struct lua_uefi_event_data*data;
	lua_State*st;
	int func_ref;
	int data_ref;
};
struct lua_uefi_event_data{
	EFI_EVENT event;
	EFI_BOOT_SERVICES*bs;
	struct lua_uefi_event_extra*data;
};

extern void uefi_lua_check_status(lua_State*L,EFI_STATUS st);
extern BOOLEAN uefi_str_to_tpl(IN CONST CHAR8*str,OUT EFI_TPL*tpl);
extern BOOLEAN uefi_str_to_event_type(IN CONST CHAR8*str,OUT UINT32*type);
extern BOOLEAN uefi_str_to_reset_type(IN CONST CHAR8*str,OUT EFI_RESET_TYPE*type);
extern BOOLEAN uefi_str_to_memory_type(IN CONST CHAR8*str,OUT EFI_MEMORY_TYPE*type);
extern BOOLEAN uefi_str_to_load_option_type(IN CONST CHAR8*str,EFI_BOOT_MANAGER_LOAD_OPTION_TYPE*type);
extern const char*uefi_memory_type_to_str(EFI_MEMORY_TYPE type);
extern const char*uefi_load_option_type_to_str(EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type);

extern void uefi_absolute_pointer_protocol_to_lua(lua_State*L,EFI_ABSOLUTE_POINTER_PROTOCOL*proto);
extern void uefi_acpi_table_protocol_to_lua(lua_State*L,EFI_ACPI_TABLE_PROTOCOL*proto);
extern void uefi_block_io_protocol_to_lua(lua_State*L,EFI_BLOCK_IO_PROTOCOL*proto);
extern void uefi_boot_logo_protocol_to_lua(lua_State*L,EFI_BOOT_LOGO_PROTOCOL*proto);
extern void uefi_boot_logo2_protocol_to_lua(lua_State*L,EDKII_BOOT_LOGO2_PROTOCOL*proto);
extern void uefi_boot_manager_policy_protocol_to_lua(lua_State*L,EFI_BOOT_MANAGER_POLICY_PROTOCOL*proto);
extern void uefi_component_name_protocol_to_lua(lua_State*L,EFI_COMPONENT_NAME_PROTOCOL*proto);
extern void uefi_component_name2_protocol_to_lua(lua_State*L,EFI_COMPONENT_NAME2_PROTOCOL*proto);
extern void uefi_deferred_image_load_protocol_to_lua(lua_State*L,EFI_DEFERRED_IMAGE_LOAD_PROTOCOL*proto);
extern void uefi_disk_io_protocol_to_lua(lua_State*L,EFI_DISK_IO_PROTOCOL*proto);
extern void uefi_disk_info_protocol_to_lua(lua_State*L,EFI_DISK_INFO_PROTOCOL*proto);
extern void uefi_file_protocol_to_lua(lua_State*L,EFI_FILE_PROTOCOL*proto);
extern void uefi_graphics_output_protocol_to_lua(lua_State*L,EFI_GRAPHICS_OUTPUT_PROTOCOL*proto);
extern void uefi_loaded_image_protocol_to_lua(lua_State*L,EFI_LOADED_IMAGE_PROTOCOL*proto);
extern void uefi_partition_info_protocol_to_lua(lua_State*L,EFI_PARTITION_INFO_PROTOCOL*proto);
extern void uefi_ramdisk_protocol_to_lua(lua_State*L,EFI_RAM_DISK_PROTOCOL*proto);
extern void uefi_security_arch_protocol_to_lua(lua_State*L,EFI_SECURITY_ARCH_PROTOCOL*proto);
extern void uefi_security2_arch_protocol_to_lua(lua_State*L,EFI_SECURITY2_ARCH_PROTOCOL*proto);
extern void uefi_serial_io_protocol_to_lua(lua_State*L,EFI_SERIAL_IO_PROTOCOL*proto);
extern void uefi_simple_file_system_protocol_to_lua(lua_State*L,EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*proto);
extern void uefi_simple_pointer_protocol_to_lua(lua_State*L,EFI_SIMPLE_POINTER_PROTOCOL*proto);
extern void uefi_simple_text_input_protocol_to_lua(lua_State*L,EFI_SIMPLE_TEXT_INPUT_PROTOCOL*proto);
extern void uefi_simple_text_output_protocol_to_lua(lua_State*L,EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*proto);
extern void uefi_timestamp_protocol_to_lua(lua_State*L,EFI_TIMESTAMP_PROTOCOL*proto);

extern EFI_ABSOLUTE_POINTER_PROTOCOL*uefi_lua_to_absolute_pointer_protocol(lua_State*L,int n);
extern EFI_ACPI_TABLE_PROTOCOL*uefi_lua_to_acpi_table_protocol(lua_State*L,int n);
extern EFI_BLOCK_IO_PROTOCOL*uefi_lua_to_block_io_protocol(lua_State*L,int n);
extern EDKII_BOOT_LOGO2_PROTOCOL*uefi_lua_to_boot_logo2_protocol(lua_State*L,int n);
extern EFI_BOOT_LOGO_PROTOCOL*uefi_lua_to_boot_logo_protocol(lua_State*L,int n);
extern EFI_BOOT_MANAGER_POLICY_PROTOCOL*uefi_lua_to_boot_manager_policy_protocol(lua_State*L,int n);
extern EFI_COMPONENT_NAME2_PROTOCOL*uefi_lua_to_component_name2_protocol(lua_State*L,int n);
extern EFI_COMPONENT_NAME_PROTOCOL*uefi_lua_to_component_name_protocol(lua_State*L,int n);
extern EFI_DEFERRED_IMAGE_LOAD_PROTOCOL*uefi_lua_to_deferred_image_load_protocol(lua_State*L,int n);
extern EFI_DISK_INFO_PROTOCOL*uefi_lua_to_disk_info_protocol(lua_State*L,int n);
extern EFI_DISK_IO_PROTOCOL*uefi_lua_to_disk_io_protocol(lua_State*L,int n);
extern EFI_FILE_PROTOCOL*uefi_lua_to_file_protocol(lua_State*L,int n);
extern EFI_GRAPHICS_OUTPUT_PROTOCOL*uefi_lua_to_graphics_output_protocol(lua_State*L,int n);
extern EFI_LOADED_IMAGE_PROTOCOL*uefi_lua_to_loaded_image_protocol(lua_State*L,int n);
extern EFI_PARTITION_INFO_PROTOCOL*uefi_lua_to_partition_info_protocol(lua_State*L,int n);
extern EFI_RAM_DISK_PROTOCOL*uefi_lua_to_ramdisk_protocol(lua_State*L,int n);
extern EFI_SECURITY2_ARCH_PROTOCOL*uefi_lua_to_security2_arch_protocol(lua_State*L,int n);
extern EFI_SECURITY_ARCH_PROTOCOL*uefi_lua_to_security_arch_protocol(lua_State*L,int n);
extern EFI_SERIAL_IO_PROTOCOL*uefi_lua_to_serial_io_protocol(lua_State*L,int n);
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*uefi_lua_to_simple_file_system_protocol(lua_State*L,int n);
extern EFI_SIMPLE_POINTER_PROTOCOL*uefi_lua_to_simple_pointer_protocol(lua_State*L,int n);
extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL*uefi_lua_to_simple_text_input_protocol(lua_State*L,int n);
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*uefi_lua_to_simple_text_output_protocol(lua_State*L,int n);
extern EFI_TIMESTAMP_PROTOCOL*uefi_lua_to_timestamp_protocol(lua_State*L,int n);

extern bool lua_arg_get_guid(lua_State*L,int idx,bool nil,EFI_GUID*guid);
extern void lua_arg_get_char16(lua_State*L,int idx,bool nil,CHAR16**data);
extern void uefi_file_protocol_get_mode(lua_State*L,int n,UINT64*mode,BOOLEAN nil);
extern void uefi_file_protocol_get_attr(lua_State*L,int n,UINT64*attr,BOOLEAN nil);
extern void uefi_graphics_output_get_pixel(lua_State*L,int t,EFI_GRAPHICS_OUTPUT_BLT_PIXEL*pixel);
extern void uefi_graphics_output_get_pixels(lua_State*L,int t,EFI_GRAPHICS_OUTPUT_BLT_PIXEL**pixels,BOOLEAN*alloc);
extern void*uefi_raw_file_info_from_lua(lua_State*L,EFI_GUID*guid,UINTN*size,int n);
extern void uefi_raw_file_info_to_lua(lua_State*L,EFI_GUID*guid,void*data);
extern void uefi_data_to_protocol(lua_State*L,EFI_GUID*guid,void*proto,BOOLEAN nil);
extern BOOLEAN uefi_str_to_guid(const char*str,EFI_GUID*out);
extern const char*uefi_guid_to_str(CONST EFI_GUID*guid);
extern const luaL_Reg*uefi_get_lib(const char*name);

extern void uefi_bs_to_lua(lua_State*L,EFI_BOOT_SERVICES*bs);
extern void uefi_rt_to_lua(lua_State*L,EFI_RUNTIME_SERVICES*rt);
extern void uefi_st_to_lua(lua_State*L,EFI_SYSTEM_TABLE*st);
extern void uefi_data_to_lua(lua_State*L,BOOLEAN allocated,VOID*data,UINTN size);
extern void uefi_data_dup_to_lua(lua_State*L,VOID*data,UINTN size);
extern void uefi_handle_to_lua(lua_State*L,EFI_HANDLE hand);
extern void uefi_str_status_to_lua(lua_State*L,const char*st);
extern void uefi_status_to_lua(lua_State*L,EFI_STATUS st);
extern void uefi_device_path_to_lua(lua_State*L,EFI_DEVICE_PATH_PROTOCOL*dp);
extern void uefi_guid_to_lua(lua_State*L,CONST EFI_GUID*guid);
extern void uefi_time_to_lua(lua_State*L,CONST EFI_TIME*time);
extern void uefi_str_guid_to_lua(lua_State*L,CHAR8*str);
extern void uefi_char16_guid_to_lua(lua_State*L,CHAR16*str);
extern void uefi_char16_16_to_lua(lua_State*L,BOOLEAN allocated,CHAR16*string);
extern void uefi_char16_an16_to_lua(lua_State*L,CHAR16*string,UINTN buff_len);
extern void uefi_char16_an8_to_lua(lua_State*L,CHAR8*string,UINTN buff_len);
extern void uefi_char16_a16_to_lua(lua_State*L,CHAR16*string);
extern void uefi_char16_a8_to_lua(lua_State*L,CHAR8*string);
extern void uefi_input_key_to_lua(lua_State*L,EFI_INPUT_KEY*key);
extern void uefi_bootopt_to_lua(lua_State*L,EFI_BOOT_MANAGER_LOAD_OPTION*opt,BOOLEAN allocated);

extern EFI_STATUS uefi_event_clean(lua_State*L,struct lua_uefi_event_data*ev);
extern struct lua_uefi_event_data*uefi_create_event(
	lua_State*L,
	EFI_BOOT_SERVICES*bs,
	UINT32 type,
	EFI_TPL tpl,
	int data,
	int func
);
extern struct lua_uefi_event_data*uefi_create_event_ex(
	lua_State*L,
	EFI_BOOT_SERVICES*bs,
	UINT32 type,
	EFI_TPL tpl,
	EFI_GUID*guid,
	int data,
	int func
);
#endif
#endif

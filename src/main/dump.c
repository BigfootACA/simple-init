/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/DevicePath.h>
#include<Guid/MemoryTypeInformation.h>
#include<stdio.h>
#include<string.h>
#include"str.h"
#include"uefi.h"
#include"logger.h"
#include"compatible.h"
#define TAG "info"

#define DECL_FMT \
	char prefix[64];\
	va_list va;\
	va_start(va,fmt);\
	memset(prefix,0,sizeof(prefix));\
	vsnprintf(prefix,sizeof(prefix)-1,fmt,va);\
	va_end(va);

static CHAR8*String16ToString8(CHAR16*str){
	CHAR8*pstr;
	UINTN size;
	EFI_STATUS st;
	if(!str)return NULL;
	size=StrLen(str)+1;
	pstr=AllocateZeroPool(size);
	if(!pstr)return NULL;
	st=UnicodeStrToAsciiStrS(str,pstr,size);
	if(EFI_ERROR(st))FreePool(pstr),pstr=NULL;
	return pstr;
}

static void dump_cfg_memory_type_info(void*data,const char*fmt,...){
	char buff[64];
	DECL_FMT
	if(!data)return;
	EFI_MEMORY_TYPE_INFORMATION*info=data;
	for(int i=0;i!=EfiMaxMemoryType;i++){
		EFI_MEMORY_TYPE_INFORMATION*m=&info[i];
		EFI_MEMORY_TYPE type=m->Type;
		UINT32 pages=m->NumberOfPages;
		UINT64 size=EFI_PAGES_TO_SIZE(pages);
		const char*str=uefi_memory_type_to_str(type);
		make_readable_str_buf(buff,sizeof(buff),size,1,0);
		tlog_debug(
			"%stype %s (%d) pages %d (0x%X, %lld bytes, %s)",
			prefix,str,type,pages,pages,size,str
		);
	}
}

static struct cfg_table_dump{
	EFI_GUID*guid;
	char*type;
	void(*dump)(void*data,const char*fmt,...);
}cfg_table_dumps[]={
	{&gEfiMemoryTypeInformationGuid,        "MemoryTypeInformation",        dump_cfg_memory_type_info},
	{&gEfiDebugImageInfoTableGuid,          "DebugImageInfo",               NULL},
	{&gEfiAcpiTableGuid,                    "ACPI",                         NULL},
	{&gEfiGlobalVariableGuid,               "GlobalVariable",               NULL},
	{&gEfiFileSystemInfoGuid,               "FileSystemInfo",               NULL},
	{&gEfiHobListGuid,                      "HobList",                      NULL},
	{&gEfiMemoryAttributesTableGuid,        "MemoryAttributes",             NULL},
	{&gEfiSystemResourceTableGuid,          "SystemResource",               NULL},
	{&gEfiMdePkgTokenSpaceGuid,             "MdePkgTokenSpace",             NULL},
	{&gEfiSmbiosTableGuid,                  "SMBIOS",                       NULL},
	{&gEfiVariableIndexTableGuid,           "VariableIndex",                NULL},
	{&gEfiDxeServicesTableGuid,             "DxeServices",                  NULL},
	{&gEfiCrc32GuidedSectionExtractionGuid, "Crc32GuidedSectionExtraction", NULL},
	{NULL,NULL,NULL}
};

static void dump_size(UINT64 size,const char*fmt,...){
	char buff[64];
	DECL_FMT
	tlog_debug(
		"%s: 0x%04llX (%lld bytes, %s)",prefix,size,size,
		make_readable_str_buf(buff,sizeof(buff),size,1,0)
	);
}

static void dump_table_header(EFI_TABLE_HEADER*hdr,const char*fmt,...){
	char buff[64];
	DECL_FMT
	if(!hdr)return;
	memset(buff,0,sizeof(buff));
	memcpy(buff,&hdr->Signature,sizeof(hdr->Signature));
	tlog_debug(
		"%ssignature: %s (0x%016llX)",
		prefix,buff,hdr->Signature
	);
	tlog_debug(
		"%srevision: %d.%d (0x%08X)",
		prefix,
		(hdr->Revision&0xffff0000)>>16,
		(hdr->Revision&0x0000ffff),
		hdr->Revision
	);
	dump_size(hdr->HeaderSize,"%sheader size",prefix);
	tlog_debug(
		"%scrc32: 0x%08X",
		prefix,hdr->CRC32
	);
}

static void dump_device_path(EFI_DEVICE_PATH_PROTOCOL*dp,const char*fmt,...){
	CHAR8*pstr;
	CHAR16*str;
	DECL_FMT
	tlog_debug("%sdevice path: %p",prefix,dp);
	if(!dp||!(str=ConvertDevicePathToText(dp,TRUE,FALSE)))return;
	if((pstr=String16ToString8(str))){
		tlog_debug("%sdevice path text: %s",prefix,pstr);
		FreePool(pstr);
	}
	FreePool(str);
}

static void dump_handle(EFI_HANDLE hand,const char*fmt,...){
	EFI_STATUS st;
	EFI_DEVICE_PATH_PROTOCOL*dp;
	DECL_FMT
	tlog_debug("%s: %p",prefix,hand);
	if(!hand)return;
	st=gBS->HandleProtocol(hand,&gEfiDevicePathProtocolGuid,(VOID*)&dp);
	if(!EFI_ERROR(st)&&dp)dump_device_path(dp,"%s ",prefix);
	else tlog_debug("%s no device path: %s",prefix,efi_status_to_string(st));
}

static void dump_runtime_services(EFI_RUNTIME_SERVICES*rt,bool g,const char*fmt,...){
	DECL_FMT
	char*tg="";
	if(g)g=rt==gRT,tg=g?" (gRT)":" (not gRT)";
	tlog_debug("%sruntime services table: %p%s",prefix,rt,tg);
	if(!rt||g)return;
	dump_table_header(&rt->Hdr,"%s  ",prefix);
}

static void dump_boot_services(EFI_BOOT_SERVICES*bs,bool g,const char*fmt,...){
	DECL_FMT
	char*tg="";
	if(g)g=bs==gBS,tg=g?" (gBS)":" (not gBS)";
	tlog_debug("%sboot services table: %p%s",prefix,bs,tg);
	if(!bs||g)return;
	dump_table_header(&bs->Hdr,"%s  ",prefix);
}

static void dump_configuration_table(EFI_CONFIGURATION_TABLE*ct,const char*fmt,...){
	char buff[64];
	DECL_FMT
	tlog_debug("%sconfiguration table: %p",prefix,ct);
	if(!ct)return;
	AsciiSPrint(buff,sizeof(buff)-1,"%g",&ct->VendorGuid);
	tlog_debug("%s  vendor guid: %s",prefix,buff);
	tlog_debug("%s  vendor table: %p",prefix,ct->VendorTable);
	for(int i=0;cfg_table_dumps[i].guid;i++){
		struct cfg_table_dump*d=&cfg_table_dumps[i];
		if(!CompareGuid(&ct->VendorGuid,d->guid))continue;
		if(d->type)tlog_debug("%s  table type: %s",prefix,d->type);
		if(d->dump)d->dump(ct->VendorTable,"%s  ",prefix);
	}
}

static void dump_system_table(EFI_SYSTEM_TABLE*st,bool g,const char*fmt,...){
	DECL_FMT
	char*tg="";
	CHAR8*pstr;
	if(g)g=st==gST,tg=g?" (gST)":" (not gST)";
	tlog_debug("%ssystem table: %p%s",prefix,st,tg);
	if(!st||g)return;
	dump_table_header(&st->Hdr,"%s  ",prefix);
	if(st->FirmwareVendor&&(pstr=String16ToString8(st->FirmwareVendor))){
		tlog_debug("%s  firmware vendor: %s",prefix,pstr);
		FreePool(pstr);
	}
	tlog_debug("%s  firmware revision: 0x%08X",prefix,st->FirmwareRevision);
	dump_handle(st->ConsoleInHandle,"%s  console input handle",prefix);
	dump_handle(st->ConsoleOutHandle,"%s  console output handle",prefix);
	dump_handle(st->StandardErrorHandle,"%s  standard error handle",prefix);
	tlog_debug("%s  console input: %p",prefix,st->ConIn);
	tlog_debug("%s  console output: %p",prefix,st->ConOut);
	tlog_debug("%s  standard error: %p",prefix,st->StdErr);
	dump_runtime_services(st->RuntimeServices,true,"%s  ",prefix);
	dump_boot_services(st->BootServices,true,"%s  ",prefix);
	tlog_debug(
		"%s  configuration table count: %llu",
		prefix,(unsigned long long)st->NumberOfTableEntries
	);
	for(UINTN i=0;i<st->NumberOfTableEntries;i++)
		dump_configuration_table(&st->ConfigurationTable[i],"%s    ",prefix);
}

static void dump_loaded_image(EFI_LOADED_IMAGE_PROTOCOL*li,const char*fmt,...){
	DECL_FMT
	tlog_debug("%sloaded image: %p",prefix,li);
	if(!li)return;
	tlog_debug("%s  revision: 0x%X",prefix,li->Revision);
	dump_handle(li->ParentHandle,"%s  parent handle",prefix);
	dump_system_table(li->SystemTable,true,"%s  ",prefix);
	dump_handle(li->DeviceHandle,"%s  device handle",prefix);
	dump_device_path(li->FilePath,"%s  file path ",prefix);
	dump_size(li->LoadOptionsSize,"%s  load options size",prefix);
	tlog_debug("%s  load options: %p",prefix,li->LoadOptions);
	if(li->LoadOptions&&li->LoadOptionsSize>0&&li->LoadOptionsSize<4096){
		UINTN size=0;
		char buff[4096];
		memset(buff,0,sizeof(buff));
		memcpy(buff,li->LoadOptions,MIN(li->LoadOptionsSize,sizeof(buff)-1));
		tlog_debug("%s  load options UTF-8: %s",prefix,(CHAR8*)buff);
		memset(buff,0,sizeof(buff));
		UnicodeStrnToAsciiStrS(
			li->LoadOptions,
			li->LoadOptionsSize,
			buff,sizeof(buff)-1,&size
		);
		tlog_debug("%s  load options UTF-16: %s",prefix,buff);
	}
	tlog_debug("%s  image base: %p",prefix,li->ImageBase);
	dump_size(li->ImageSize,"%s  image size");
	tlog_debug(
		"%s  image code memory type: %s",prefix,
		uefi_memory_type_to_str(li->ImageCodeType)
	);
	tlog_debug(
		"%s  image data memory type: %s",prefix,
		uefi_memory_type_to_str(li->ImageDataType)
	);
}

void uefi_dump_info(){
	EFI_STATUS st;
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	dump_handle(gImageHandle,"image handle");
	dump_runtime_services(gRT,false,"");
	dump_boot_services(gBS,false,"");
	dump_system_table(gST,false,"");
	st=gBS->HandleProtocol(
		gImageHandle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&li
	);
	if(!EFI_ERROR(st)&&li)dump_loaded_image(li,"");
	else tlog_debug(
		"cannot handle loaded image protocol: %s",
		efi_status_to_string(st)
	);
}

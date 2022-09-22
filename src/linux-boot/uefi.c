/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/ReportStatusCodeLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include<comp_libfdt.h>
#include"confd.h"
#include"logger.h"
#include"internal.h"
#define TAG "linux-uefi"

int boot_linux_uefi(linux_boot*boot){
	EFI_STATUS st;
	EFI_HANDLE ih;
	CHAR16 cmdline[512];
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	fill_kernel_device_path(&boot->kernel);
	tlog_info("load kernel image...");
	st=gBS->LoadImage(
		FALSE,
		gImageHandle,
		boot->kernel.device,
		boot->kernel.address,
		boot->kernel.size,
		&ih
	);
	if(EFI_ERROR(st)){
		tlog_warn(
			"load image failed: %s",
			efi_status_to_string(st)
		);
		return -1;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"open protocol failed: %s",
			efi_status_to_string(st)
		);
		return -1;
	}
	if(li->ImageCodeType!=EfiLoaderCode){
		tlog_error("not a UEFI Application");
		return -1;
	}
	ZeroMem(cmdline,sizeof(cmdline));
	AsciiStrToUnicodeStrS(
		boot->cmdline,
		cmdline,
		sizeof(cmdline)/sizeof(CHAR16)
	);
	confd_save_file(NULL);
	li->LoadOptions=cmdline;
	li->LoadOptionsSize=(StrLen(cmdline)+1)*sizeof(CHAR16);
	gBS->SetWatchdogTimer(60,0x10000,0,NULL);
	EfiSignalEventReadyToBoot();
	REPORT_STATUS_CODE(
		EFI_PROGRESS_CODE,(
			EFI_SOFTWARE_DXE_BS_DRIVER|
			EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT
		)
	);
	tlog_info("start kernel image...");
	st=gBS->StartImage(ih,NULL,NULL);
	if(EFI_ERROR(st))tlog_error(
		"start image failed: %s",
		efi_status_to_string(st)
	);
	else tlog_error("kernel image unexpected return");
	gBS->SetWatchdogTimer(0,0x10000,0,NULL);
	return -1;
}

int linux_boot_update_uefi(linux_boot*lb){
	int off,r;
	UINT32 dv=0;
	VOID*rsv=NULL;
	EFI_STATUS st;
	UINTN ms=0,mk=0,ds=0,l;
	EFI_MEMORY_DESCRIPTOR*mm=NULL,*md;
	if(!lb->dtb.address||!lb->initrd.address)return 0;
	if(!lb->config||!lb->config->add_uefi_runtime)return 0;

	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_warn(
		-1,"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;
        st=gBS->AllocatePool(EfiLoaderData,64,&rsv);
        if(!EFI_ERROR(st)){
		ZeroMem(rsv,64);
		EFI_GUID guid={0x888eb0c6,0x8ede,0x4ff5,{0xa8,0xf0,0x9a,0xee,0x5c,0xb9,0x77,0xc2}};
		gBS->InstallConfigurationTable(&guid,rsv);
	}
	fdt_setprop_u64(
		lb->dtb.address,off,
		"linux,uefi-system-table",
		(UINT64)(UINTN)gST
	);

	st=gBS->GetMemoryMap(&ms,mm,&mk,&ds,&dv);
	if(st==EFI_BUFFER_TOO_SMALL){
		st=gBS->AllocatePool(EfiLoaderData,ms+(2*ds),(VOID**)&mm);
		if(mm&&!EFI_ERROR(st))st=gBS->GetMemoryMap(&ms,mm,&mk,&ds,&dv);
	}
	if(!EFI_ERROR(st)){
		for(l=0;l<ms;l+=ds){
			md=(void*)mm+l;
			if((md->Attribute&EFI_MEMORY_RUNTIME))
				md->VirtualStart=md->PhysicalStart;
		}
		fdt_setprop_u64(
			lb->dtb.address,off,
			"linux,uefi-mmap-start",
			(UINT64)(UINTN)mm
		);
		fdt_setprop_u32(
			lb->dtb.address,off,
			"linux,uefi-mmap-size",
			(UINT32)(UINTN)ms
		);
		fdt_setprop_u32(
			lb->dtb.address,off,
			"linux,uefi-mmap-desc-size",
			(UINT32)(UINTN)ds
		);
		fdt_setprop_u32(
			lb->dtb.address,off,
			"linux,uefi-mmap-desc-ver",
			(UINT32)(UINTN)dv
		);
	}

	return 0;
}

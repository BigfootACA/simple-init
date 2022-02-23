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
	confd_save_file(NULL,NULL);
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

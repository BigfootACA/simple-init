/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"locate.h"
#include"compatible.h"
#define TAG "drivers"

bool boot_load_driver(EFI_DEVICE_PATH_PROTOCOL*p){
	EFI_STATUS st;
	EFI_HANDLE ih;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		tlog_warn("load image failed: %s",efi_status_to_string(st));
		return false;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		tlog_warn("open protocol failed: %s",efi_status_to_string(st));
		return false;
	}
	if(
		li->ImageCodeType!=EfiBootServicesCode&&
		li->ImageCodeType!=EfiRuntimeServicesCode
	){
		if(ih)gBS->UnloadImage(ih);
		tlog_warn("not a UEFI Driver");
		return false;
	}
	st=gBS->StartImage(ih,NULL,NULL);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		tlog_error("start dxe failed: %s",efi_status_to_string(st));
	}
	return true;
}

void boot_load_drivers(){
	locate_ret loc;
	char*d,**ds,*b="uefi.drivers";
	if(!(ds=confd_ls(b)))return;
	for(int i=0;ds[i];i++){
		if(!(d=confd_get_string_base(b,ds[i],NULL)))continue;
		tlog_verbose("try to load dxe driver from %s",d);
		if(!boot_locate(&loc,d)){
			tlog_warn("resolve locate %s failed",d);
			free(d);
			continue;
		}
		if(loc.type!=LOCATE_FILE){
			tlog_warn("only support load dxe driver from file");
			free(d);
			continue;
		}
		if(boot_load_driver(loc.device))
			tlog_debug("loaded dxe driver %s",d);
		loc.file->Close(loc.file);
		free(d);
	}
	if(ds[0])free(ds[0]);
	free(ds);
}
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"locate.h"
#include"compatible.h"
#include"filesystem.h"
#include"gui/splash.h"
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
	fsh*f=NULL;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	char*d,**ds=NULL,*b="uefi.drivers";
	if(!(ds=confd_ls(b)))return;
	for(int i=0;ds[i];i++){
		if(!(d=confd_get_string_base(b,ds[i],NULL)))continue;
		gui_splash_set_text(true,_("Loading UEFI driver %s..."),d);
		tlog_verbose("try to load dxe driver from %s",d);
		if(fs_open(NULL,&f,d,FILE_FLAG_READ)!=0)
			EDONE(telog_warn("open %s failed",d));
		if(fs_ioctl(f,FS_IOCTL_UEFI_GET_DEVICE_PATH,&dp)!=0)
			EDONE(telog_warn("get device path of %s failed",d));
		if(boot_load_driver(dp))
			tlog_debug("loaded dxe driver %s",d);
		done:
		if(f)fs_close(&f);
		if(d)free(d);
		d=NULL,d=NULL,dp=NULL;
	}
	if(ds[0])free(ds[0]);
	free(ds);
}
#endif

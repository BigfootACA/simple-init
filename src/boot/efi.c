/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include<Guid/FileInfo.h>
#include"boot.h"
#include"uefi.h"
#include"confd.h"
#include"logger.h"
#include"filesystem.h"
#define TAG "efi"

typedef EFI_DEVICE_PATH_PROTOCOL*image_locator(boot_config*boot);

static EFI_DEVICE_PATH_PROTOCOL*locate_fv_guid(boot_config*boot){
	EFI_STATUS st;
	EFI_GUID efi_guid={0};
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	MEDIA_FW_VOL_FILEPATH_DEVICE_PATH fn;
	char*guid=confd_get_string_base(boot->key,"efi_fv_guid",NULL);
	if(!guid)return NULL;
	ZeroMem(&efi_guid,sizeof(EFI_GUID));
	st=AsciiStrToGuid(guid,&efi_guid);
	free(guid);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"invalid guid %s: %s",
		guid,efi_status_to_string(st)
	));
	EfiInitializeFwVolDevicepathNode(&fn,&efi_guid);
	st=gBS->HandleProtocol(
		gImageHandle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&li
	);
	if(EFI_ERROR(st))EDONE(tlog_error(
		"handle loaded image protocol failed: %s",
		efi_status_to_string(st)
	));
	if(!(dp=AppendDevicePathNode(
		DevicePathFromHandle(li->DeviceHandle),
		(EFI_DEVICE_PATH_PROTOCOL*)&fn
	)))EDONE(tlog_error("AppendDevicePathNode failed"));
	done:
	return DuplicateDevicePath(dp);
}

static EFI_DEVICE_PATH_PROTOCOL*locate_file(boot_config*boot){
	fsh*f=NULL;
	EFI_DEVICE_PATH_PROTOCOL*ret=NULL,*dp=NULL;
	char*efi=confd_get_string_base(boot->key,"efi_file",NULL);
	if(!efi||!*efi)goto done;
	if(fs_open(NULL,&f,efi,FILE_FLAG_READ)!=0)
		EDONE(telog_error("open efi %s failed",efi));
	if(fs_ioctl(f,FS_IOCTL_UEFI_GET_DEVICE_PATH,&dp)!=0)
		EDONE(telog_error("get device path of %s failed",efi));
	ret=DuplicateDevicePath(dp);
	done:
	if(f)fs_close(&f);
	if(efi)free(efi);
	return ret;
}

static image_locator*locators[]={
	locate_fv_guid,
	locate_file,
	NULL
};

int run_boot_efi(boot_config*boot){
	static char img[PATH_MAX];
	int r=-1;
	EFI_STATUS st;
	EFI_HANDLE ih=NULL;
	UINTN size=0,os,len;
	CHAR16*dpt,*buf=NULL;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	char*options=confd_get_string_base(boot->key,"options",NULL);
	bool wc=confd_get_boolean_base(boot->key,"options_widechar",true);
	if(!boot)ERET(EINVAL);
	if(boot->mode!=BOOT_EFI)ERET(EINVAL);
	for(int i=0;locators[i];i++)
		if((dp=locators[i](boot)))break;
	if(!dp){
		tlog_error("efi file not found, abort");
		ERET(ENOENT);
	}
	st=gBS->LoadImage(
		FALSE,
		gImageHandle,
		dp,
		NULL,
		0,
		&ih
	);
	if(EFI_ERROR(st))EDONE(tlog_error(
		"load image failed: %s",
		efi_status_to_string(st)
	));
	st=gBS->OpenProtocol(
		ih,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,
		gImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st))EDONE(tlog_error(
		"open protocol failed: %s",
		efi_status_to_string(st)
	));
	if(li->ImageCodeType!=EfiLoaderCode)
		EDONE(tlog_error("not a UEFI Application"));
	if(options){
		os=AsciiStrLen(options);
		tlog_info("options: '%s'",options);
		if(wc){
			len=(os+1)*sizeof(CHAR16);
			if(!(buf=AllocateZeroPool(len)))
				EDONE(tlog_error("allocate memory failed"));
			AsciiStrToUnicodeStrS(options,buf,len);
			li->LoadOptions=buf;
			li->LoadOptionsSize=len;
			free(options);
			options=NULL;
		}else{
			li->LoadOptions=options;
			li->LoadOptionsSize=os;
		}
	}
	ZeroMem(img,sizeof(img));
	dpt=ConvertDevicePathToText(dp,TRUE,FALSE);
	if(dpt)UnicodeStrToAsciiStrS(dpt,img,sizeof(img));
	else AsciiStrCpyS(img,sizeof(img),"(Unknown)");
	confd_save_file(NULL);
	tlog_info("start image '%s' ...",img);
	st=gBS->StartImage(ih,&size,NULL);
	if(EFI_ERROR(st)){
		tlog_error(
			"start image failed: %s",
			efi_status_to_string(st)
		);
		r=-1;
	}
	r=0;
	done:
	if(dp)FreePool(dp);
	if(buf)FreePool(buf);
	if(options)free(options);
	if(ih)gBS->UnloadImage(ih);
	return r;
}

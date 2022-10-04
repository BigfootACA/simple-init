/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdlib.h>
#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"confd.h"
#include"locate.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"filesystem.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "dxe-load"

static bool auto_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	int did=0;
	fsh*f=user_data;
	char*path=NULL,drv[32];
	if(!f)return true;
	if(id==0&&fs_get_path_alloc(f,&path)==0&&path){
		do{
			memset(drv,0,sizeof(drv));
			snprintf(drv,sizeof(drv)-1,"driver-%d",did++);
		}while(confd_get_type_base("uefi.drivers",drv)==TYPE_STRING);
		confd_set_string_base("uefi.drivers",drv,path);
		free(path);
	}
	fs_close(&f);
	return false;
}

static void load_cb(void*d){
	int r;
	fsh*f=d;
	EFI_STATUS st;
	EFI_HANDLE ih=NULL;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*p=NULL;
	if((r=fs_ioctl(f,FS_IOCTL_UEFI_GET_DEVICE_PATH,&p))!=0||!p){
		msgbox_alert("get device path failed");
		tlog_warn("get device path failed: %s",strerror(r));
		goto done;
	}
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
	if(EFI_ERROR(st)){
		msgbox_alert("load image failed: %s",efi_status_to_string(st));
		tlog_warn("load image failed: %s",efi_status_to_string(st));
		goto done;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		msgbox_alert("open protocol failed: %s",_(efi_status_to_string(st)));
		tlog_warn("open protocol failed: %s",efi_status_to_string(st));
		goto done;
	}
	if(
		li->ImageCodeType!=EfiBootServicesCode&&
		li->ImageCodeType!=EfiRuntimeServicesCode
	){
		msgbox_alert("not a UEFI Driver");
		tlog_warn("not a UEFI Driver");
		goto done;
	}
	st=gBS->StartImage(ih,NULL,NULL);
	if(EFI_ERROR(st)){
		msgbox_alert("start dxe failed: %s",_(efi_status_to_string(st)));
		tlog_error("start dxe failed: %s",efi_status_to_string(st));
		goto done;
	}else msgbox_set_user_data(msgbox_create_yesno(
		auto_cb,
		"auto load this dxe driver on next boot?"
	),f);
	return;
	done:
	if(f)fs_close(&f);
	if(ih)gBS->UnloadImage(ih);
}

static bool confirm_click(
	uint16_t id,
	const char*text __attribute__((unused)),
	void*user_data
){
	fsh*f=user_data;
	if(id==0)lv_async_call(load_cb,f);
	else fs_close(&f);
	return false;
}

static int uefi_dxe_load_draw(struct gui_activity*d){
	int r;
	fsh*f=NULL;
	if(!d||!d->args)return -1;
	if((r=fs_open(NULL,&f,d->args,FILE_FLAG_READ))!=0){
		msgbox_alert("Open file failed: %s",strerror(r));
		tlog_error("open file failed: %s",strerror(r));
		return -r;
	}
	msgbox_set_user_data(msgbox_create_yesno(
		confirm_click,
		"Load UEFI Driver (DXE) '%s'?",
		(char*)d->args
	),f);
	return -10;
}

struct gui_register guireg_uefi_dxe_load={
	.name="uefi-dxe-load",
	.title="UEFI Driver (DXE) Load",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.efi$",
		NULL
	},
	.draw=uefi_dxe_load_draw,
};
#endif
#endif

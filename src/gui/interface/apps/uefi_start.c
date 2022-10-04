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
#include<Library/UefiLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"logger.h"
#include"language.h"
#include"filesystem.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "start"

static EFI_HANDLE ih;
static int after_exit(void*d __attribute__((unused))){
	if(!ih)return -1;
	UINTN size;
	int r=0;
	gST->ConOut->ClearScreen(gST->ConOut);
	if(EFI_ERROR(gBS->StartImage(ih,&size,NULL))){
		tlog_error("start image failed");
		r=1;
	}
	if(ih)gBS->UnloadImage(ih);
	return r;
}

static void start_cb(void*d){
	EFI_STATUS st;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*p=d;
	if(!p)return;
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
	FreePool(p);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("load image failed: %s",efi_status_to_string(st));
		tlog_warn("load image failed: %s",efi_status_to_string(st));
		return;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		msgbox_alert("open protocol failed: %s",_(efi_status_to_string(st)));
		tlog_warn("open protocol failed: %s",efi_status_to_string(st));
		return;
	}
	if(li->ImageCodeType!=EfiLoaderCode){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("not a UEFI Application");
		tlog_warn("not a UEFI Application");
		return;
	}
	gui_run_and_exit(after_exit);
}

static bool confirm_click(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	if(id==0)lv_async_call(start_cb,user_data);
	return false;
}

static int uefi_start_draw(struct gui_activity*d){
	int r;
	fsh*f=NULL;
	EFI_DEVICE_PATH_PROTOCOL*p=NULL;
	if(!d||!d->args)return -1;
	if((r=fs_open(NULL,&f,d->args,FILE_FLAG_READ))!=0){
		msgbox_alert("Open file failed: %s",strerror(r));
		tlog_error("open file failed: %s",strerror(r));
		return -r;
	}
	if((r=fs_ioctl(f,FS_IOCTL_UEFI_GET_DEVICE_PATH,&p))!=0||!p){
		fs_close(&f);
		msgbox_alert("get device path failed");
		tlog_warn("get device path failed: %s",strerror(r));
		return -r;
	}
	msgbox_set_user_data(msgbox_create_yesno(
		confirm_click,
		"Start UEFI Application '%s'?",
		(char*)d->args
	),DuplicateDevicePath(p));
	fs_close(&f);
	return -10;
}

struct gui_register guireg_uefi_start={
	.name="uefi-app-start",
	.title="UEFI Application Start",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.efi$",
		NULL
	},
	.draw=uefi_start_draw,
};
#endif
#endif

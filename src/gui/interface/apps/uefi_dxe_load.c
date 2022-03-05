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
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "dxe-load"

static void load_cb(lv_task_t*t){
	char*full_path=t->user_data;
	EFI_STATUS st;
	EFI_HANDLE ih;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*p=fs_get_device_path(full_path);
	if(!p){
		msgbox_alert("get device path failed");
		tlog_warn("get device path failed");
		return;
	}
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
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
	if(
		li->ImageCodeType!=EfiBootServicesCode&&
		li->ImageCodeType!=EfiRuntimeServicesCode
	){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("not a UEFI Driver");
		tlog_warn("not a UEFI Driver");
		return;
	}
	st=gBS->StartImage(ih,NULL,NULL);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("start dxe failed: %s",_(efi_status_to_string(st)));
		tlog_error("start dxe failed: %s",efi_status_to_string(st));
	}
}

static bool confirm_click(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	if(id==0)lv_task_once(lv_task_create(load_cb,1,LV_TASK_PRIO_LOWEST,user_data));
	return false;
}

static int uefi_dxe_load_draw(struct gui_activity*d){
	if(!d)return -1;
	static char full_path[PATH_MAX];
	memset(full_path,0,sizeof(full_path));
	strncpy(full_path,(char*)d->args,sizeof(full_path)-1);
	msgbox_set_user_data(msgbox_create_yesno(
		confirm_click,
		"Load UEFI Driver (DXE) '%s'?",
		full_path
	),full_path);
	return -10;
}

struct gui_register guireg_uefi_dxe_load={
	.name="uefi-dxe-load",
	.title="UEFI Driver (DXE) Load",
	.icon="efi.svg",
	.show_app=false,
	.open_file=true,
	.draw=uefi_dxe_load_draw,
};
#endif
#endif

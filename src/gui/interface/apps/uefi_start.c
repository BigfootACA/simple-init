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
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadedImage.h>
#include"gui.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "start"

static EFI_HANDLE ih;
static int after_exit(void*d __attribute__((unused))){
	if(!ih)return -1;
	UINTN size;
	int r=0;
	if(EFI_ERROR(gBS->StartImage(ih,&size,NULL))){
		tlog_error("StartImage failed");
		r=1;
	}
	if(ih)gBS->UnloadImage(ih);
	return r;
}

extern EFI_DEVICE_PATH_PROTOCOL*fs_get_device_path(const char*path);
static void start_cb(lv_task_t*t){
	char*full_path=t->user_data;
	EFI_STATUS st;
	EFI_LOADED_IMAGE_PROTOCOL*li;
	EFI_DEVICE_PATH_PROTOCOL*p=fs_get_device_path(full_path);
	if(!p){
		msgbox_alert("get DevicePath failed");
		tlog_warn("get DevicePath failed");
		return;
	}
	st=gBS->LoadImage(FALSE,gImageHandle,p,NULL,0,&ih);
	if(EFI_ERROR(st)){
		if(ih)gBS->UnloadImage(ih);
		msgbox_alert("LoadImage failed: %llx",st);
		tlog_warn("LoadImage failed: %llx",st);
		return;
	}
	st=gBS->OpenProtocol(
		ih,&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,gImageHandle,NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		msgbox_alert("OpenProtocol failed: %llx",st);
		tlog_warn("OpenProtocol failed: %llx",st);
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
	if(id==0)lv_task_once(lv_task_create(start_cb,1,LV_TASK_PRIO_LOWEST,user_data));
	return false;
}

static int uefi_start_draw(struct gui_activity*d){
	if(!d)return -1;
	static char full_path[PATH_MAX];
	memset(full_path,0,PATH_MAX-1);
	strncpy(full_path,(char*)d->args,PATH_MAX-1);
	msgbox_set_user_data(msgbox_create_yesno(
		confirm_click,
		"Start UEFI Application '%s'?",
		full_path
	),full_path);
	return -10;
}

struct gui_register guireg_uefi_start={
	.name="uefi-app-start",
	.title="UEFI Application Start",
	.icon="efi.png",
	.show_app=false,
	.open_file=true,
	.draw=uefi_start_draw,
};
#endif
#endif

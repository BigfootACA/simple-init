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
#define TAG "shell"
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

static bool msg_click(uint16_t id,const char*text,void*user_data __attribute__((unused))){
	if(id==0){
		MEDIA_FW_VOL_FILEPATH_DEVICE_PATH fn;
		EFI_LOADED_IMAGE_PROTOCOL*li;
		EFI_DEVICE_PATH_PROTOCOL*dp;
		EFI_STATUS st;
		EfiInitializeFwVolDevicepathNode(&fn,&gUefiShellFileGuid);
		st=gBS->HandleProtocol(
			gImageHandle,
			&gEfiLoadedImageProtocolGuid,
			(VOID**)&li
		);
		if(EFI_ERROR(st)){
			tlog_error("handle protocol failed: %s",efi_status_to_string(st));
			msgbox_alert("handle protocol failed: %s",_(efi_status_to_string(st)));
			return false;
		}
		if(!(dp=AppendDevicePathNode(
			DevicePathFromHandle(li->DeviceHandle),
			(EFI_DEVICE_PATH_PROTOCOL*)&fn
		))){
			tlog_error("append device path node failed");
			msgbox_alert("append device path node failed");
			return false;
		}
		st=gBS->LoadImage(FALSE,gImageHandle,dp,NULL,0,&ih);
		if(EFI_ERROR(st)){
			if(ih)gBS->UnloadImage(ih);
			tlog_error("load image failed: %s",efi_status_to_string(st));
			msgbox_alert("load image failed: %s",_(efi_status_to_string(st)));
			return false;
		}
		gui_run_and_exit(after_exit);
	}
	return false;
}

static int uefi_shell_draw(struct gui_activity*act){
	msgbox_create_yesno(msg_click,"Exit GUI Application and enter UEFI Shell?");
	return -10;
}

struct gui_register guireg_uefi_shell={
	.name="uefi-shell",
	.title="UEFI Shell",
	.show_app=true,
	.draw=uefi_shell_draw,
};
#endif
#endif

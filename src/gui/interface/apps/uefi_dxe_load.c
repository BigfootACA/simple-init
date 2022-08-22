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
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "dxe-load"

static bool auto_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	EFI_DEVICE_PATH_PROTOCOL*dp;
	char*buffer=AllocateZeroPool(PATH_MAX*4);
	if(!buffer)return true;
	char*path=buffer;
	char*buff=path+PATH_MAX;
	char*loc=buff+PATH_MAX;
	char*drv=loc+PATH_MAX;
	char*p=user_data,*cp=path;
	int did=0;
	if(id==0){
		if(!p||p[1]!=':'||(p[2]!='/'&&p[2]!='\\'))return true;
		if(!(dp=fs_drv_get_device_path(p[0])))return true;
		if(!locate_auto_add_by_device_path(loc,PATH_MAX,dp))return true;
		ZeroMem(path,PATH_MAX);
		AsciiStrCpyS(path,PATH_MAX,p);
		do{if(*cp=='/')*cp='\\';}while(*cp++);
		ZeroMem(buff,PATH_MAX);
		AsciiSPrint(buff,PATH_MAX,"@%a:%a",loc,p+2);
		do{
			ZeroMem(drv,PATH_MAX);
			AsciiSPrint(drv,PATH_MAX,"driver-%d",did);
			did++;
		}while(confd_get_type_base("uefi.drivers",drv)==TYPE_STRING);
		confd_set_string_base("uefi.drivers",drv,buff);
	}
	return false;
}

static void load_cb(void*d){
	char*full_path=d;
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
	}else msgbox_set_user_data(msgbox_create_yesno(
		auto_cb,
		"auto load this dxe driver on next boot?"
	),full_path);
}

static bool confirm_click(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	if(id==0)lv_async_call(load_cb,user_data);
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
	.open_regex=(char*[]){
		".*\\.efi$",
		NULL
	},
	.draw=uefi_dxe_load_draw,
};
#endif
#endif

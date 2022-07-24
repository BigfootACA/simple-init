/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<Library/UefiLib.h>
#include<Library/DebugLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleTextIn.h>
#include<Protocol/SimpleTextInEx.h>
#define TAG "uefikeyboard"
#include"gui.h"
#include"list.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
#include"gui/activity.h"

static list*kbds;
static void*event_reg=NULL;
static bool has_lr=false;
static lv_indev_drv_t drv;
static lv_indev_t*dev;
struct keyboard_data{
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL*kbd;
	void*pd;
};
static void keyboard_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	list*l,*n;
	EFI_INPUT_KEY p;
	data->state=LV_INDEV_STATE_REL;
	if(indev_drv!=&drv||!dev||dev->driver!=indev_drv)return;
	if(!(l=list_first(kbds))){
		tlog_warn("no available keyboard devices");
		lv_indev_enable(dev,false);
		return;
	}else do{
		n=l->next;
		LIST_DATA_DECLARE(kd,l,struct keyboard_data*);
		if(!kd->pd)kd->pd=kd->kbd->ReadKeyStroke;
		else if(kd->kbd->ReadKeyStroke!=kd->pd){
			tlog_warn("ReadKeyStroke changed, disable keyboard device");
			list_obj_del(&kbds,l,list_default_free);
			continue;
		}
		if(EFI_ERROR(kd->kbd->ReadKeyStroke(kd->kbd,&p)))continue;
		data->state=0;
		if(p.ScanCode!=0){
			if(lv_group_get_editing(gui_grp))switch(p.ScanCode){
				// why UP and DOWN map to LEFT and RIGHT?
				// because volume keys only have UP and DOWN.
				case SCAN_VOLUME_UP:
				case SCAN_UP:data->key=has_lr?LV_KEY_UP:LV_KEY_LEFT;break;
				case SCAN_BRIGHTNESS_UP:
				case SCAN_LEFT:data->key=LV_KEY_LEFT;has_lr=true;break;
				case SCAN_PAGE_UP:data->key=LV_KEY_UP;break;
				case SCAN_VOLUME_DOWN:
				case SCAN_DOWN:data->key=has_lr?LV_KEY_DOWN:LV_KEY_RIGHT;break;
				case SCAN_BRIGHTNESS_DOWN:
				case SCAN_RIGHT:data->key=LV_KEY_RIGHT;has_lr=true;break;
				case SCAN_PAGE_DOWN:data->key=LV_KEY_DOWN;break;
				case SCAN_SUSPEND:data->key=LV_KEY_ENTER;break;
				case SCAN_ESC:
					if(guiact_get_activities())
						guiact_do_back();
					data->key=0;
			}else switch(p.ScanCode){
				case SCAN_UP:
				case SCAN_LEFT:
				case SCAN_VOLUME_UP:
				case SCAN_BRIGHTNESS_UP:
				case SCAN_PAGE_UP:data->key=LV_KEY_PREV;break;
				case SCAN_DOWN:
				case SCAN_RIGHT:
				case SCAN_VOLUME_DOWN:
				case SCAN_BRIGHTNESS_DOWN:
				case SCAN_PAGE_DOWN:data->key=LV_KEY_NEXT;break;
				case SCAN_SUSPEND:data->key=LV_KEY_ENTER;break;
				case SCAN_ESC:
					if(guiact_get_activities())
						guiact_do_back();
					data->key=0;
			}
		}else if(p.UnicodeChar!=0)switch(p.UnicodeChar){
			case ' ':case '\n':case '\r':data->key=LV_KEY_ENTER;break;
			default:data->key=p.UnicodeChar;
		}else continue;
		data->state=LV_INDEV_STATE_PR;
		data->continue_reading=true;
		return;
	}while((l=n));
	data->continue_reading=false;
}
static bool proto_cmp(list*f,void*data){
	LIST_DATA_DECLARE(d,f,struct keyboard_data*);
	return d->kbd==(EFI_SIMPLE_TEXT_INPUT_PROTOCOL*)data;
}
static int _keyboard_register(EFI_SIMPLE_TEXT_INPUT_PROTOCOL*k){
	struct keyboard_data*kbd=NULL;
	if(list_search_one(kbds,proto_cmp,k))return 0;
	if(!(kbd=malloc(sizeof(struct keyboard_data))))return -1;
	memset(kbd,0,sizeof(struct keyboard_data));
	kbd->kbd=k;
	tlog_debug("found new uefi keyboard %p",kbd->kbd);
	list_obj_add_new(&kbds,kbd);
	if(dev)lv_indev_enable(dev,true);
	return 0;
}
STATIC EFIAPI VOID keyboard_event(IN EFI_EVENT ev,IN VOID*ctx){
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL*kbd=NULL;
	EFI_STATUS st=gBS->LocateProtocol(
		&gEfiSimpleTextInProtocolGuid,
		event_reg,(VOID**)&kbd
	);
	tlog_notice("receive keyboard device hot-plug event");
	if(!EFI_ERROR(st)&&kbd)_keyboard_register(kbd);
}
static int keyboard_register(){
	bool found=false;
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimpleTextInProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"locate keyboard failed: %s",
		efi_status_to_string(st)
	);
	for(UINTN i=0;i<cnt;i++){
		EFI_SIMPLE_TEXT_INPUT_PROTOCOL*kbd=NULL;
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiSimpleTextInProtocolGuid,
			(VOID**)&kbd
		))||!kbd)continue;
		if(_keyboard_register(kbd)==0)found=true;
	}
	EfiCreateProtocolNotifyEvent(
		&gEfiSimpleTextInProtocolGuid,
		TPL_CALLBACK,
		(EFI_EVENT_NOTIFY)keyboard_event,
		NULL,&event_reg
	);
	if(found){
		lv_indev_drv_init(&drv);
		drv.type=LV_INDEV_TYPE_KEYPAD;
		drv.read_cb=keyboard_read;
		dev=lv_indev_drv_register(&drv);
		lv_indev_set_group(dev,gui_grp);
		return 0;
	}
	return trlog_warn(-1,"no uefi keyboard found");
}

struct input_driver indrv_uefi_kbd={
	.name="uefi-keyboard",
	.compatible={
		"uefigop",
		"uefiuga",
		"dummy",
		NULL
	},
	.drv_register=keyboard_register,
};
#endif
#endif

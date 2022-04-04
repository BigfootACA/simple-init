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
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/AbsolutePointer.h>
#define TAG "uefitouch"
#include"gui.h"
#include"list.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
static void*event_reg=NULL;
static list*touchs;
struct input_data{
	bool lp,oldp;
	INT64 rx,lx,oldx;
	INT64 ry,ly,oldy;
	EFI_ABSOLUTE_POINTER_PROTOCOL*touch;
	lv_indev_drv_t drv;
	lv_indev_t*dev;
	void*pd;
};
static bool touch_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	EFI_ABSOLUTE_POINTER_STATE p;
	struct input_data*d=indev_drv->user_data;
	if(!d)return false;
	if(!d->pd)d->pd=d->touch->GetState;
	else if(d->touch->GetState!=d->pd){
		tlog_warn("GetState changed, disable touch device");
		indev_drv->user_data=NULL;
		list_obj_del_data(&touchs,d,NULL);
		memset(d,0,sizeof(struct input_data));
		free(d);
		return false;
	}
	if(!EFI_ERROR(d->touch->GetState(d->touch,&p))){
		d->lx=(p.CurrentX*gui_w/d->rx);
		d->ly=(p.CurrentY*gui_h/d->ry);
		d->lp=p.ActiveButtons==1;
		if(
			d->oldx==d->lx&&
			d->oldy==d->ly&&
			d->oldp==d->lp
		)return true;
		d->oldx=d->lx,d->oldy=d->ly,d->oldp=d->lp;
	}
	data->point.x=d->lx;
	data->point.y=d->ly;
	data->state=d->lp?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
	return false;
}
static bool proto_cmp(list*f,void*data){
	LIST_DATA_DECLARE(d,f,struct input_data*);
	return d->touch==(EFI_ABSOLUTE_POINTER_PROTOCOL*)data;
}
static int _touch_register(EFI_ABSOLUTE_POINTER_PROTOCOL*touch){
	struct input_data*data=NULL;
	if(list_search_one(touchs,proto_cmp,touch))return 0;
	if(!(data=malloc(sizeof(struct input_data))))return -1;
	memset(data,0,sizeof(struct input_data));
	data->touch=touch;
	lv_indev_drv_init(&data->drv);
	data->drv.type=LV_INDEV_TYPE_POINTER;
	data->drv.read_cb=touch_read;
	data->drv.user_data=data;
	data->dev=lv_indev_drv_register(&data->drv);
	data->rx=data->touch->Mode->AbsoluteMaxX;
	data->ry=data->touch->Mode->AbsoluteMaxY;
	tlog_debug("found new uefi absolute %p",data->touch);
	list_obj_add_new(&touchs,data);
	return 0;
}
STATIC EFIAPI VOID touch_event(IN EFI_EVENT ev,IN VOID*ctx){
	EFI_ABSOLUTE_POINTER_PROTOCOL*touch=NULL;
	EFI_STATUS st=gBS->LocateProtocol(
		&gEfiAbsolutePointerProtocolGuid,
		event_reg,(VOID**)&touch
	);
	tlog_notice("receive touch device hot-plug event");
	if(!EFI_ERROR(st)&&touch)_touch_register(touch);
}
int touch_register(){
	bool found=false;
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiAbsolutePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"locate touch failed: %s",
		efi_status_to_string(st)
	);
	for(UINTN i=0;i<cnt;i++){
		EFI_ABSOLUTE_POINTER_PROTOCOL*touch=NULL;
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiAbsolutePointerProtocolGuid,
			(VOID**)&touch
		))||!touch)continue;
		if(_touch_register(touch)==0)found=true;
	}
	EfiCreateProtocolNotifyEvent(
		&gEfiAbsolutePointerProtocolGuid,
		TPL_CALLBACK,
		(EFI_EVENT_NOTIFY)touch_event,
		NULL,&event_reg
	);
	return found?0:trlog_warn(-1,"no uefi touch found");
}
#endif
#endif

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
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/AbsolutePointer.h>
#define TAG "uefitouch"
#include"gui.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
struct input_data{
	bool lp,oldp;
	INT64 rx,lx,oldx;
	INT64 ry,ly,oldy;
	EFI_ABSOLUTE_POINTER_PROTOCOL*touch;
	lv_indev_drv_t drv;
	lv_indev_t*dev;
};
static bool touch_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	EFI_ABSOLUTE_POINTER_STATE p;
	struct input_data*d=indev_drv->user_data;
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
int touch_register(){
	bool found=false;
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiAbsolutePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,"locate absolute failed: %lld",st);
	for(UINTN i=0;i<cnt;i++){
		struct input_data*data=malloc(sizeof(struct input_data));
		if(!data)break;
		memset(data,0,sizeof(struct input_data));
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiAbsolutePointerProtocolGuid,
			(VOID**)&data->touch
		)))continue;
		lv_indev_drv_init(&data->drv);
		data->drv.type=LV_INDEV_TYPE_POINTER;
		data->drv.read_cb=touch_read;
		data->drv.user_data=data;
		data->dev=lv_indev_drv_register(&data->drv);
		data->rx=data->touch->Mode->AbsoluteMaxX;
		data->ry=data->touch->Mode->AbsoluteMaxY;
		tlog_debug("found uefi absolute %p",data->touch);
		found=true;
	}
	return found?0:trlog_warn(-1,"no uefi absolute found");
}
#endif
#endif

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
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
static void*event_reg=NULL;
static list*touchs;
struct input_data{
	INT64 lx,ly,rx,ry;
	EFI_ABSOLUTE_POINTER_PROTOCOL*touch;
	lv_indev_drv_t drv;
	lv_indev_t*dev;
	void*pd;
};
static bool touch_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	bool r=false;
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
	if(EFI_ERROR(d->touch->GetState(d->touch,&p)))return false;
	if(p.CurrentX==d->lx&&p.CurrentY==d->ly)return false;
	data->point.x=((double)p.CurrentX/(double)d->rx)*gui_w;
	data->point.y=((double)p.CurrentY/(double)d->ry)*gui_h;
	d->lx=p.CurrentX,d->ly=p.CurrentY;
	data->state=LV_INDEV_STATE_PR;
	return r;
}
static bool proto_cmp(list*f,void*data){
	LIST_DATA_DECLARE(d,f,struct input_data*);
	return d->touch==(EFI_ABSOLUTE_POINTER_PROTOCOL*)data;
}
static int _touch_register(EFI_ABSOLUTE_POINTER_PROTOCOL*touch){
	struct input_data*data=NULL;
	if(list_search_one(touchs,proto_cmp,touch))return 0;
	if(confd_get_boolean(
		"gui.driver.pointer.use_first",
		(bool)PcdGetBool(PcdGuiDefaultMouseOnlyFirst)
	)&&list_count(touchs)>=1){
		tlog_debug("skip uefi touch %p",touch);
		return 0;
	}
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
static int touch_register(){
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
struct input_driver indrv_uefi_touch={
	.name="uefi-touch",
	.compatible={
		"uefigop",
		"uefiuga",
		"dummy",
		NULL
	},
	.drv_register=touch_register,
};
#endif
#endif

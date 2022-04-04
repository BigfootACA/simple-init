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
#include<Library/PcdLib.h>
#include<Library/UefiLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimplePointer.h>
#define TAG "uefipointer"
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
int gui_mouse_scale=0;
static void*event_reg=NULL;
static list*mouses;
struct input_data{
	bool lp;
	INT64 rx,lx;
	INT64 ry,ly;
	EFI_SIMPLE_POINTER_PROTOCOL*mouse;
	lv_indev_drv_t drv;
	lv_indev_t*dev;
	void*pd;
};
static bool pointer_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	EFI_SIMPLE_POINTER_STATE p;
	struct input_data*d=indev_drv->user_data;
	if(!d)return false;
	if(
		!d->dev->cursor&&
		(d->lx>0||d->ly>0)
	)lv_indev_set_cursor(d->dev,gui_cursor);
	if(!d->pd)d->pd=d->mouse->GetState;
	else if(d->mouse->GetState!=d->pd){
		tlog_warn("GetState changed, disable mouse device");
		indev_drv->user_data=NULL;
		list_obj_del_data(&mouses,d,NULL);
		memset(d,0,sizeof(struct input_data));
		free(d);
		return false;
	}
	if(!EFI_ERROR(d->mouse->GetState(d->mouse,&p))){
		INT64 ix=d->lx+(p.RelativeMovementX*gui_mouse_scale/d->rx);
		INT64 iy=d->ly+(p.RelativeMovementY*gui_mouse_scale/d->ry);
		d->lx=MIN(MAX(ix,0),gui_w);
		d->ly=MIN(MAX(iy,0),gui_h);
		d->lp=p.LeftButton==1;
	}
	data->point.x=d->lx;
	data->point.y=d->ly;
	data->state=d->lp?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
	return false;
}
static bool proto_cmp(list*f,void*data){
	LIST_DATA_DECLARE(d,f,struct input_data*);
	return d->mouse==(EFI_SIMPLE_POINTER_PROTOCOL*)data;
}
static int _pointer_register(EFI_SIMPLE_POINTER_PROTOCOL*mouse){
	struct input_data*data=NULL;
	if(list_search_one(mouses,proto_cmp,mouse))return 0;
	if(confd_get_boolean(
		"gui.driver.pointer.use_first",
		(bool)PcdGetBool(PcdGuiDefaultMouseOnlyFirst)
	)&&list_count(mouses)>=1){
		tlog_debug("skip uefi pointer %p",mouse);
		return 0;
	}
	if(!(data=malloc(sizeof(struct input_data))))return -1;
	memset(data,0,sizeof(struct input_data));
	data->mouse=mouse;
	lv_indev_drv_init(&data->drv);
	data->drv.type=LV_INDEV_TYPE_POINTER;
	data->drv.read_cb=pointer_read;
	data->drv.user_data=data;
	data->dev=lv_indev_drv_register(&data->drv);
	data->rx=data->mouse->Mode->ResolutionX;
	data->ry=data->mouse->Mode->ResolutionY;
	tlog_debug("found new uefi pointer %p",data->mouse);
	list_obj_add_new(&mouses,data);
	return 0;
}
STATIC EFIAPI VOID pointer_event(IN EFI_EVENT ev,IN VOID*ctx){
	EFI_SIMPLE_POINTER_PROTOCOL*mouse=NULL;
	EFI_STATUS st=gBS->LocateProtocol(
		&gEfiSimplePointerProtocolGuid,
		event_reg,(VOID**)&mouse
	);
	tlog_notice("receive mouse device hot-plug event");
	if(!EFI_ERROR(st)&&mouse)_pointer_register(mouse);
}
int pointer_register(){
	bool found=false;
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	gui_mouse_scale=confd_get_integer(
		"gui.mouse_scale",
		(int)PcdGet8(PcdGuiDefaultMouseScale)
	);
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimplePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"locate pointer failed: %s",
		efi_status_to_string(st)
	);
	for(UINTN i=0;i<cnt;i++){
		EFI_SIMPLE_POINTER_PROTOCOL*mouse=NULL;
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiSimplePointerProtocolGuid,
			(VOID**)&mouse
		))||!mouse)continue;
		if(_pointer_register(mouse)==0)found=true;
	}
	EfiCreateProtocolNotifyEvent(
		&gEfiSimplePointerProtocolGuid,
		TPL_CALLBACK,
		(EFI_EVENT_NOTIFY)pointer_event,
		NULL,&event_reg
	);
	return found?0:trlog_warn(-1,"no uefi pointer found");
}
#endif
#endif

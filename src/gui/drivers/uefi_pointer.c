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
#include"gui/tools.h"
#include"gui/guidrv.h"
int gui_mouse_scale=0;
static void*event_reg=NULL;
static list*mouses;
static lv_indev_drv_t drv;
static lv_indev_t*dev;
static bool lp=false;
static INT64 lx=0,ly=0;
struct input_data{
	INT64 rx,ry;
	EFI_SIMPLE_POINTER_PROTOCOL*mouse;
	void*pd;
};
static void pointer_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	list*l,*n;
	EFI_SIMPLE_POINTER_STATE p;
	if(indev_drv!=&drv||!dev||dev->driver!=indev_drv)return;
	if(!dev->cursor&&(lx>0||ly>0))
		lv_indev_set_cursor(dev,gui_cursor);
	if(!(l=list_first(mouses))){
		tlog_warn("no available mouse devices");
		lv_indev_enable(dev,false);
		return;
	}else do{
		n=l->next;
		LIST_DATA_DECLARE(d,l,struct input_data*);
		if(!d->pd)d->pd=d->mouse->GetState;
		else if(d->mouse->GetState!=d->pd){
			tlog_warn("GetState changed, disable mouse device");
			list_obj_del(&mouses,l,list_default_free);
			continue;
		}
		if(EFI_ERROR(d->mouse->GetState(d->mouse,&p)))continue;
		INT64 ix=lx+(p.RelativeMovementX*gui_mouse_scale/d->rx);
		INT64 iy=ly+(p.RelativeMovementY*gui_mouse_scale/d->ry);
		lx=lv_coord_border(ix,gui_w-1,0);
		ly=lv_coord_border(iy,gui_h-1,0);
		lp=p.LeftButton==1;
		if(ix<0||iy<0||ix>=gui_w||iy>=gui_h)lp=false;
		data->continue_reading=true;
		break;
	}while((l=n));
	data->point.x=lx,data->point.y=ly;
	data->state=lp?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
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
	data->rx=data->mouse->Mode->ResolutionX;
	data->ry=data->mouse->Mode->ResolutionY;
	tlog_debug("found new uefi pointer %p",data->mouse);
	list_obj_add_new(&mouses,data);
	if(dev)lv_indev_enable(dev,true);
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
static int pointer_register(){
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
	if(found){
		lv_indev_drv_init(&drv);
		drv.type=LV_INDEV_TYPE_POINTER;
		drv.read_cb=pointer_read;
		dev=lv_indev_drv_register(&drv);
		return 0;
	}
	return trlog_warn(-1,"no uefi pointer found");
}

struct input_driver indrv_uefi_pointer={
	.name="uefi-pointer",
	.compatible={
		"uefigop",
		"uefiuga",
		"dummy",
		NULL
	},
	.drv_register=pointer_register,
};
#endif
#endif

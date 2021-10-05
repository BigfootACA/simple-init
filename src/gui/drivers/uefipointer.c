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
#include<Protocol/SimplePointer.h>
#define TAG "uefipointer"
#include"gui.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
#ifndef MOUSE_SCALE
#define MOUSE_SCALE 1
#endif
#ifndef DISPLAY_DPI
#define DISPLAY_DPI 200
#endif
int gui_mouse_scale=(DISPLAY_DPI/200)*MOUSE_SCALE;
struct input_data{
	bool lp;
	INT64 rx,lx;
	INT64 ry,ly;
	EFI_SIMPLE_POINTER_PROTOCOL*mouse;
	lv_indev_drv_t drv;
	lv_indev_t*dev;
};
static bool pointer_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	EFI_SIMPLE_POINTER_STATE p;
	struct input_data*d=indev_drv->user_data;
	if(!d->dev->cursor)lv_indev_set_cursor(d->dev,gui_cursor);
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
int pointer_register(){
	bool found=false;
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimplePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,"locate pointer failed: %lld",st);
	for(UINTN i=0;i<cnt;i++){
		struct input_data*data=malloc(sizeof(struct input_data));
		if(!data)break;
		memset(data,0,sizeof(struct input_data));
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiSimplePointerProtocolGuid,
			(VOID**)&data->mouse
		)))continue;
		lv_indev_drv_init(&data->drv);
		data->drv.type=LV_INDEV_TYPE_POINTER;
		data->drv.read_cb=pointer_read;
		data->drv.user_data=data;
		data->dev=lv_indev_drv_register(&data->drv);
		data->rx=data->mouse->Mode->ResolutionX;
		data->ry=data->mouse->Mode->ResolutionY;
		tlog_debug("found uefi pointer %p",data->mouse);
		found=true;
	}
	return found?0:trlog_warn(-1,"no uefi pointer found");
}
#endif
#endif

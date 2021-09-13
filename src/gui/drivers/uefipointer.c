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
#include"defines.h"
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
#ifndef MOUSE_SCALE
#define MOUSE_SCALE 1
#endif
static int scale=(DISPLAY_DPI/200)*MOUSE_SCALE;
static lv_indev_t*dev=NULL;
static EFI_SIMPLE_POINTER_PROTOCOL*mouse=NULL;
static bool pointer_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	static INT64 lx=0,ly=0;
	static bool lp=false;
	if(!dev->cursor)lv_indev_set_cursor(dev,gui_cursor);
	EFI_SIMPLE_POINTER_STATE p;
	if(!EFI_ERROR(mouse->GetState(mouse,&p))){
		INT64 ix=lx+(p.RelativeMovementX*scale/(INT64)mouse->Mode->ResolutionX);
		INT64 iy=ly+(p.RelativeMovementY*scale/(INT64)mouse->Mode->ResolutionY);
		lx=MIN(MAX(ix,0),gui_w);
		ly=MIN(MAX(iy,0),gui_h);
		lp=p.LeftButton==1;
	}
	data->point.x=lx;
	data->point.y=ly;
	data->state=lp?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
	return false;
}
static int pointer_init(){
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimplePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,"locate pointer failed: %lld",st);
	for(UINTN i=0;i<cnt;i++){
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiSimplePointerProtocolGuid,
			(VOID**)&mouse
		)))continue;
		if(mouse)return trlog_debug(0,"found uefi pointer %p",mouse);
	}
	return trlog_warn(-1,"no uefi pointer found");
}
int pointer_register(){
	if(pointer_init()<0)return -1;
	static lv_indev_drv_t drv;
	lv_indev_drv_init(&drv);
	drv.type=LV_INDEV_TYPE_POINTER;
	drv.read_cb=pointer_read;
	dev=lv_indev_drv_register(&drv);
	return 0;
}
#endif
#endif

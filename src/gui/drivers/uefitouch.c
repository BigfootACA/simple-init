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
#include"defines.h"
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
static lv_indev_t*dev=NULL;
static EFI_ABSOLUTE_POINTER_PROTOCOL*touch=NULL;
static bool touch_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	static bool lp=false,oldp=false;
	static INT64 lx=0,oldx=0;
	static INT64 ly=0,oldy=0;
	EFI_ABSOLUTE_POINTER_STATE p;
	if(!EFI_ERROR(touch->GetState(touch,&p))){
		lx=(p.CurrentX*gui_w/(INT64)touch->Mode->AbsoluteMaxX);
		ly=(p.CurrentY*gui_h/(INT64)touch->Mode->AbsoluteMaxY);
		lp=p.ActiveButtons==1;
		if(oldx==lx&&oldy==ly&&oldp==lp)return true;
		oldx=lx,oldy=ly,oldp=lp;
	}
	data->point.x=lx;
	data->point.y=ly;
	data->state=lp?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
	return false;
}
static int touch_init(){
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiAbsolutePointerProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,"locate absolute failed: %lld",st);
	for(UINTN i=0;i<cnt;i++){
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiAbsolutePointerProtocolGuid,
			(VOID**)&touch
		)))continue;
		if(touch)return trlog_debug(0,"found uefi absolute %p",touch);
	}
	return trlog_warn(-1,"no uefi absolute found");
}
int touch_register(){
	if(touch_init()<0)return -1;
	static lv_indev_drv_t drv;
	lv_indev_drv_init(&drv);
	drv.type=LV_INDEV_TYPE_POINTER;
	drv.read_cb=touch_read;
	dev=lv_indev_drv_register(&drv);
	return 0;
}
#endif
#endif

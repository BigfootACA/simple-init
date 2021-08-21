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
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
static lv_indev_t dev;
static EFI_SIMPLE_POINTER_PROTOCOL*mouse=NULL;
static bool pointer_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	static UINT64 lx=0,ly=0;
	static bool lp=false,init=false;
	if(!init){
		init=true;
		lv_indev_set_cursor(&dev,gui_cursor);
	}
	EFI_SIMPLE_POINTER_STATE p;
	if(!EFI_ERROR(mouse->GetState(mouse,&p))){
		lx+=p.RelativeMovementX/mouse->Mode->ResolutionX;
		ly+=p.RelativeMovementY/mouse->Mode->ResolutionY;
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
	lv_indev_drv_register(&drv);
	return 0;
}
#endif
#endif

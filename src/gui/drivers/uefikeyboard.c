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
#include<Protocol/SimpleTextIn.h>
#define TAG "uefikeyboard"
#include"defines.h"
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL*keyboard;
static bool keyboard_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	EFI_INPUT_KEY p;
	data->state=LV_INDEV_STATE_REL;
	if(!EFI_ERROR(keyboard->ReadKeyStroke(keyboard,&p))){
		data->state=0;
		if(p.ScanCode!=0)switch(p.ScanCode){
			case SCAN_UP:case SCAN_LEFT:case SCAN_PAGE_UP:data->key=LV_KEY_PREV;break;
			case SCAN_DOWN:case SCAN_RIGHT:case SCAN_PAGE_DOWN:data->key=LV_KEY_NEXT;break;
		}else if(p.UnicodeChar!=0)switch(p.UnicodeChar){
			case ' ':case '\n':case '\r':data->key=LV_KEY_ENTER;break;
		}else return false;
		data->state=LV_INDEV_STATE_PR;
	}
	return false;
}
static int keyboard_init(){
	UINTN cnt=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimpleTextInProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))return trlog_warn(-1,"locate keyboard failed: %lld",st);
	for(UINTN i=0;i<cnt;i++){
		if(EFI_ERROR(gBS->HandleProtocol(
			hands[i],
			&gEfiSimpleTextInProtocolGuid,
			(VOID**)&keyboard
		)))continue;
		if(keyboard)return trlog_debug(0,"found uefi keyboard %p",keyboard);
	}
	return trlog_warn(-1,"no uefi keyboard found");
}
int keyboard_register(){
	if(keyboard_init()<0)return -1;
	static lv_indev_drv_t drv;
	lv_indev_drv_init(&drv);
	drv.type=LV_INDEV_TYPE_KEYPAD;
	drv.read_cb=keyboard_read;
	lv_indev_set_group(lv_indev_drv_register(&drv),gui_grp);
	return 0;
}
#endif
#endif

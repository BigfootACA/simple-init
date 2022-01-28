/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/GraphicsOutput.h>
#include"gui.h"
#include"logger.h"
#include"version.h"
#include"gui/guidrv.h"
#define TAG "uefigop"
#ifndef DISPLAY_DPI
#define DISPLAY_DPI 200
#endif

static int ww=-1,hh=-1;
static EFI_GRAPHICS_OUTPUT_PROTOCOL*gop;

static void uefigop_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	gop->Blt(
		gop,
		(EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)color_p,
		EfiBltBufferToVideo,
		0,0,
		area->x1,area->y1,
		area->x2-area->x1+1,
		area->y2-area->y1+1,
		0
	);
	lv_disp_flush_ready(disp_drv);
}

static int uefigop_init(){
	if(!gBS)return -1;
	if(EFI_ERROR(gBS->LocateProtocol(
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		(VOID**)&gop
	))||!gop)return -1;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE*mode=gop->Mode;
	if(!mode)
		return trlog_error(-1,"cannot get uefi graphics mode");
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info=mode->Info;
	if(!info)
		return trlog_error(-1,"cannot get uefi graphics mode info");
	ww=info->HorizontalResolution;
	hh=info->VerticalResolution;
	if(ww<=0||hh<=0)
		return trlog_error(-1,"invalid uefi graphics mode");
	tlog_debug("uefigop resolution %dx%d",ww,hh);
	return 0;
}

extern int keyboard_register();
extern int pointer_register();
extern int touch_register();
static int uefigop_register(){
	if(uefigop_init()!=0)return -1;
	size_t s=ww*hh;
	static lv_color_t*buf=NULL;
	static lv_disp_buf_t disp_buf;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_buf_init(&disp_buf,buf,NULL,s);
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.buffer=&disp_buf;
	disp_drv.flush_cb=uefigop_flush;
	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
	lv_disp_drv_register(&disp_drv);
	logger_set_console(false);
	keyboard_register();
	pointer_register();
	touch_register();
	return 0;
}
static void uefigop_get_sizes(uint32_t*width,uint32_t*height){
	if(width)*width=ww;
	if(height)*height=hh;
}
static void uefigop_get_dpi(int*dpi){
	if(dpi)*dpi=(int)(DISPLAY_DPI);
}
static bool uefigop_can_sleep(){
	return false;
}
static void uefigop_exit(){
	logger_set_console(true);
	gST->ConOut->ClearScreen(gST->ConOut);
}
struct gui_driver guidrv_uefigop={
	.name="uefigop",
	.drv_register=uefigop_register,
	.drv_getsize=uefigop_get_sizes,
	.drv_getdpi=uefigop_get_dpi,
	.drv_cansleep=uefigop_can_sleep,
	.drv_exit=uefigop_exit
};
#endif
#endif

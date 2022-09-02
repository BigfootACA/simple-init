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
#include<Library/BaseMemoryLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/UgaDraw.h>
#include"gui.h"
#include"logger.h"
#include"version.h"
#include"gui/guidrv.h"
#define TAG "uefiuga"

static int ww=-1,hh=-1;
static EFI_UGA_DRAW_PROTOCOL*uga;

static void uefiuga_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	uga->Blt(
		uga,
		(EFI_UGA_PIXEL*)color_p,
		EfiUgaBltBufferToVideo,
		0,0,
		area->x1,area->y1,
		area->x2-area->x1+1,
		area->y2-area->y1+1,
		0
	);
	lv_disp_flush_ready(disp_drv);
}

static int uefiuga_init(){
	if(!gBS)return -1;
	if(EFI_ERROR(gBS->LocateProtocol(
		&gEfiUgaDrawProtocolGuid,
		NULL,
		(VOID**)&uga
	))||!uga)return -1;
	UINT32 xw=0,xh=0,dep=0,rr=0;
	if(EFI_ERROR(uga->GetMode(uga,&xw,&xh,&dep,&rr)))
		return trlog_error(-1,"cannot get uefi graphics mode");
	ww=xw,hh=xh;
	if(ww<=0||hh<=0)
		return trlog_error(-1,"invalid uefi graphics mode");
	tlog_debug("uefiuga resolution %dx%d",ww,hh);
	return 0;
}

static int uefiuga_register(){
	if(uefiuga_init()!=0)return -1;
	size_t s=ww*hh;
	static lv_color_t*buf=NULL;
	static lv_disp_draw_buf_t disp_buf;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,s);
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=uefiuga_flush;
	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
	disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}
	lv_disp_drv_register(&disp_drv);
	logger_set_console(false);
	return 0;
}

static void uefiuga_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}

static bool uefiuga_can_sleep(){
	return false;
}

static void uefiuga_exit(){
	logger_set_console(true);
	gST->ConOut->ClearScreen(gST->ConOut);
	if(uga){
		EFI_UGA_PIXEL c;
		ZeroMem(&c,sizeof(c));
		uga->Blt(
			uga,&c,EfiUgaVideoFill,
			0,0,0,0,ww,hh,0
		);
	}
}

struct gui_driver guidrv_uefiuga={
	.name="uefiuga",
	.drv_register=uefiuga_register,
	.drv_getsize=uefiuga_get_sizes,
	.drv_cansleep=uefiuga_can_sleep,
	.drv_exit=uefiuga_exit
};
#endif
#endif

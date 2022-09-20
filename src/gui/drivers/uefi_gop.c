/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/GraphicsOutput.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"version.h"
#include"compatible.h"
#include"gui/guidrv.h"
#define TAG "uefigop"

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

static int uefigop_get_modes(int*cnt,struct display_mode**modes){
	UINTN s;
	EFI_STATUS st;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info;
	if(!cnt||!modes)ERET(EINVAL);
	if(!gop)ERET(ENODEV);
	*cnt=gop->Mode->MaxMode;
	if(*cnt<=0)return 0;
	size_t size=(gop->Mode->MaxMode+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memset(*modes,0,size);
	for(UINT32 i=0;i<(UINT32)(*cnt);i++){
		s=sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),info=NULL;
		st=gop->QueryMode(gop,i,&s,&info);
		if(EFI_ERROR(st)||!info)continue;
		(*modes)[i].width=info->HorizontalResolution;
		(*modes)[i].height=info->VerticalResolution;
		AsciiSPrint(
			(*modes)[i].name,
			sizeof((*modes)[i].name),
			"%dx%d",
			info->HorizontalResolution,
			info->VerticalResolution
		);
		FreePool(info);
	}
	return 0;
}

static void uefigop_apply_mode(){
	UINTN s;
	int cnt=0;
	EFI_STATUS st;
	char*name=NULL;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info;
	struct display_mode*mode=NULL,*modes=NULL;
	if(uefigop_get_modes(&cnt,&modes)!=0)
		EDONE(telog_warn("get modes failed"));
	if(!(name=confd_get_string("gui.mode",NULL)))goto done;
	if(!modes||cnt<=0)
		EDONE(tlog_warn("no any modes found"));
	for(int i=0;i<cnt;i++)
		if(AsciiStriCmp(name,modes[i].name)==0)
			mode=&modes[i];
	if(!mode)EDONE(tlog_warn("mode %s not found",name));
	for(UINT32 i=0;i<gop->Mode->MaxMode;i++){
		s=sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),info=NULL;
		st=gop->QueryMode(gop,i,&s,&info);
		if(EFI_ERROR(st)||!info)continue;
		if(
			info->VerticalResolution==mode->height&&
			info->HorizontalResolution==mode->width
		){
			st=gop->SetMode(gop,i);
			if(!EFI_ERROR(st))tlog_info("set mode to %s",name);
			else tlog_warn("set mode failed: %s",efi_status_to_string(st));
		}
		FreePool(info);
	}
	done:
	if(name)free(name);
	if(modes)free(modes);
}

static int uefigop_init(){
	EFI_STATUS st;
	if(!gBS)return -1;
	st=gBS->LocateProtocol(
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		(VOID**)&gop
	);
	if(EFI_ERROR(st)||!gop)return -1;
	uefigop_apply_mode();
	ww=gop->Mode->Info->HorizontalResolution;
	hh=gop->Mode->Info->VerticalResolution;
	if(ww<=0||hh<=0)
		return trlog_error(-1,"invalid uefi graphics mode");
	tlog_debug("use uefigop resolution %dx%d",ww,hh);
	return 0;
}

static int uefigop_register(){
	if(uefigop_init()!=0)return -1;
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
	disp_drv.flush_cb=uefigop_flush;
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

static void uefigop_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}

static bool uefigop_can_sleep(){
	return false;
}

static void uefigop_exit(){
	logger_set_console(true);
	gST->ConOut->ClearScreen(gST->ConOut);
	if(gop){
		EFI_GRAPHICS_OUTPUT_BLT_PIXEL c;
		ZeroMem(&c,sizeof(c));
		gop->Blt(
			gop,&c,EfiBltVideoFill,
			0,0,0,0,ww,hh,0
		);
	}
}

struct gui_driver guidrv_uefigop={
	.name="uefigop",
	.drv_register=uefigop_register,
	.drv_getsize=uefigop_get_sizes,
	.drv_cansleep=uefigop_can_sleep,
	.drv_get_modes=uefigop_get_modes,
	.drv_exit=uefigop_exit,
};
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define STBI_NO_SIMD
#define STBI_NO_STDIO
#include<Library/UefiLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/GraphicsOutput.h>
#include<unistd.h>
#include"uefi.h"
#include"boot.h"
#include"logger.h"
#include"compatible.h"
#include"filesystem.h"
#include"stb_image.h"
#include"stb_image_resize.h"
#define TAG "splash"

int boot_draw_splash(boot_config*cfg){
	UINTN ds=0;
	size_t bs=0;
	EFI_STATUS st;
	uint8_t*disp=NULL,*p,s;
	int sw=0,sh=0,iw=0,ih=0;
	int dw=0,dh=0,dx=0,dy=0,r=-1;
	stbi_uc*buffer=NULL,*image=NULL;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL c;
	EFI_GRAPHICS_OUTPUT_PROTOCOL*gop=NULL;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE*mode;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info;
	if(!cfg->splash[0])return 0;
	st=gBS->LocateProtocol(
		&gEfiGraphicsOutputProtocolGuid,
		NULL,(VOID**)&gop
	);
	if(EFI_ERROR(st)||!gop)return trlog_warn(
		-1,"locate graphics output protocol failed: %s",
		efi_status_to_string(st)
	);
	if(!(mode=gop->Mode))return trlog_warn(-1,"cannot get gop mode");
	if(!(info=mode->Info))return trlog_warn(-1,"cannot get gop mode info");
	sw=info->HorizontalResolution,sh=info->VerticalResolution;
	if(sw<=0||sh<=0)return trlog_warn(-1,"invalid gop resolution");
	tlog_debug("screen resolution: %dx%d",sw,sh);
	if(fs_read_whole_file(NULL,cfg->splash,(VOID**)&buffer,&bs)!=0||!buffer)
		EDONE(telog_warn("read splash file %s failed",cfg->splash));
	if(EFI_ERROR(st)||!buffer||bs<=0)EDONE(tlog_warn(
		"read splash file failed: %s",
		efi_status_to_string(st)
	));
	tlog_debug(
		"parse splash image %s size %llu bytes",
		cfg->splash,(unsigned long long)bs
	);
	image=stbi_load_from_memory(
		buffer,(int)bs,&iw,&ih,NULL,sizeof(c)
	);
	if(!image||iw<=0||ih<=0)
		EDONE(tlog_warn("parse splash image failed"));
	tlog_debug("splash image resolution: %dx%d",iw,ih);
	if(iw>sw||ih>sh){
		float sc=MIN(sw/(float)iw,sh/(float)ih);
		tlog_debug("scale image %0.2f%%",sc*100);
		dw=MIN(sw,iw*sc),dh=MIN(sh,ih*sc);
	}else dw=iw,dh=ih;
	ds=dw*dh*sizeof(c);
	if(!(disp=AllocateZeroPool(ds)))
		EDONE(tlog_warn("alloc image failed"));
	if(dw==iw&&dh==ih)CopyMem(disp,image,ds);
	else if(!stbir_resize_uint8(
		image,iw,ih,0,
		disp,dw,dh,0,
		sizeof(c)
	))EDONE(tlog_warn("resize image failed"));
	tlog_debug("splash display resolution: %dx%d",dw,dh);
	for(p=disp;p<disp+ds;p+=sizeof(c))s=p[0],p[0]=p[2],p[2]=s;
	dx=(sw-dw)/2,dy=(sh-dh)/2;
	tlog_debug("splash display position: %dx%d",dx,dy);
	ZeroMem(&c,sizeof(c));
	gop->Blt(gop,&c,EfiBltVideoFill,0,0,sw,sh,sw,sh,0);
	st=gop->Blt(gop,
		(EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)disp,
		EfiBltBufferToVideo,
		0,0,dx,dy,dw,dh,0
	);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"draw splash to screen failed: %s",
		efi_status_to_string(st)
	));
	r=0;done:
	if(buffer)free(buffer);
	if(disp)FreePool(disp);
	return r;
}

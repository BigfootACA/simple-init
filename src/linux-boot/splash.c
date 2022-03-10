/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/GraphicsOutput.h>
#include<comp_libfdt.h>
#include"str.h"
#include"logger.h"
#include"internal.h"
#include"fdtparser.h"
#include"KernelFdt.h"
#define TAG "splash"

static int get_splash(linux_boot*lb,uint64_t*base,uint64_t*size){
	int r,off;
	EFI_STATUS st;
	KERNEL_FDT_PROTOCOL*fdt;
	st=gBS->LocateProtocol(
		&gKernelFdtProtocolGuid,
		NULL,
		(VOID**)&fdt
	);
	if(EFI_ERROR(st)||!fdt||!fdt->Fdt)return 0;

	r=fdt_path_offset(fdt->Fdt,"/reserved-memory/splash_region");
	if(r<0)r=fdt_path_offset(fdt->Fdt,"/reserved-memory/cont_splash_region");
	if(r<0)return trlog_warn(
		0,"get splash_region node from KernelFdtDxe failed: %s",
		fdt_strerror(r)
	);
	off=r;

	return fdt_get_reg(fdt->Fdt,off,0,base,size)?0:-1;
}

int linux_boot_update_splash(linux_boot*lb){
	int r,off;
	char buf[64];
	EFI_STATUS st;
	char*format="a8r8g8b8";
	uint64_t base=0,size=0;
	uint32_t width=0,height=0,stride=0;
	EFI_GRAPHICS_OUTPUT_PROTOCOL*gop=NULL;
	if(!lb->dtb.address)return 0;

	st=gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID**)&gop);
	if(!EFI_ERROR(st)){
		base=gop->Mode->FrameBufferBase;
		size=gop->Mode->FrameBufferSize;
		width=gop->Mode->Info->HorizontalResolution;
		height=gop->Mode->Info->VerticalResolution;
		stride=width*4;
		switch(gop->Mode->Info->PixelFormat){
			case PixelBlueGreenRedReserved8BitPerColor:format="a8r8g8b8";break;
			case PixelRedGreenBlueReserved8BitPerColor:format="a8b8g8r8";break;
			default:;
		}
	}else trlog_warn(
		0,"query graphics output protocol failed: %s",
		efi_status_to_string(st)
	);

	if(lb->config){
		linux_screen_info*scr=&lb->config->screen;
		if(!scr->update_splash)return 0;
		if(scr->splash.start>0)base=scr->splash.start;
		if(scr->splash.end>0&&base>0)size=scr->splash.end-base;
		if(scr->width>0)width=scr->width;
		if(scr->height>0)height=scr->height;
		if(scr->stride>0)stride=scr->stride;
	}
	if(
		base<=0&&size<=0&&
		(r=get_splash(lb,&base,&size))!=0
	)return trlog_warn(
		0,"get splash_region from KernelFdtDxe failed: %s",
		fdt_strerror(r)
	);
	if(base<=0||size<=0)return 0;
	if(stride<=0&&width>0)stride=width*4;
	size=ALIGN_VALUE(size,EFI_PAGE_SIZE);

	tlog_debug(
		"splash memory: 0x%llx - 0x%llx (0x%llx %s)",
		(unsigned long long)base,
		(unsigned long long)base+size,
		(unsigned long long)size,
		make_readable_str_buf(buf,sizeof(buf),size,1,0)
	);

	r=fdt_path_offset(lb->dtb.address,"/reserved-memory/splash_region");
	if(r<0)r=fdt_path_offset(lb->dtb.address,"/reserved-memory/cont_splash_region");
	if(r<0){
		r=fdt_path_offset(lb->dtb.address,"/reserved-memory");
		if(r<0)r=fdt_add_subnode(lb->dtb.address,0,"reserved-memory");
		if(r<0)return trlog_warn(
			0,"create reserved-memory node failed: %s",
			fdt_strerror(r)
		);
		off=r;
		r=fdt_add_subnode(lb->dtb.address,off,"splash_region");
		if(r<0)return trlog_warn(
			0,"create splash_region node failed: %s",
			fdt_strerror(r)
		);
	}
	off=r;

	fdt_setprop_u64(lb->dtb.address,off,"reg",base);
	fdt_appendprop_u64(lb->dtb.address,off,"reg",size);

	if(lb->config&&!lb->config->screen.add_simplefb)return 0;
	if(fdt_path_offset(lb->dtb.address,"/chosen/framebuffer")>=0)
		return trlog_warn(0,"skip add simplefb because it exists");
	if(width<=0||height<=0||stride<=0||!format)
		return trlog_warn(0,"skip add simplefb because no width/height/stride");

	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_warn(
		0,"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;
	r=fdt_add_subnode(lb->dtb.address,off,"framebuffer");
	if(r<0)return trlog_warn(
		0,"create framebuffer node failed: %s",
		fdt_strerror(r)
	);
	off=r;

	fdt_setprop_string(lb->dtb.address,off,"compatible","simple-framebuffer");
	fdt_setprop_u64(lb->dtb.address,off,"reg",base);
	fdt_appendprop_u64(lb->dtb.address,off,"reg",size);
	fdt_setprop_u32(lb->dtb.address,off,"width",width);
	fdt_setprop_u32(lb->dtb.address,off,"height",width);
	fdt_setprop_u32(lb->dtb.address,off,"stride",width);
	fdt_setprop_string(lb->dtb.address,off,"format",format);
	fdt_setprop_string(lb->dtb.address,off,"status","okay");

	return 0;
}

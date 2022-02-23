/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiBootServicesTableLib.h>
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
	uint64_t base=0,size=0;
	if(!lb->dtb.address)return 0;

	if(lb->config&&lb->config->splash.start>0){
		base=lb->config->splash.start;
		size=lb->config->splash.end-base;
	}else if((r=get_splash(lb,&base,&size))!=0)return trlog_warn(
		0,"get splash_region from KernelFdtDxe failed: %s",
		fdt_strerror(r)
	);
	if(base<=0||size<=0)return 0;

	tlog_debug(
		"splash memory: 0x%llx - 0x%llx (0x%llx %s)",
		(unsigned long long)base,
		(unsigned long long)base+size,
		(unsigned long long)size,
		make_readable_str_buf(buf,sizeof(buf),size,1,0)
	);

	r=fdt_path_offset(lb->dtb.address,"/reserved-memory/splash_region");
	if(r<0)r=fdt_path_offset(lb->dtb.address,"/reserved-memory/cont_splash_region");
	if(r<0)return trlog_warn(
		0,"get splash_region node from dtb failed: %s",
		fdt_strerror(r)
	);
	off=r;

	fdt_setprop_u64(lb->dtb.address,off,"reg",base);
	fdt_appendprop_u64(lb->dtb.address,off,"reg",size);

	return 0;
}

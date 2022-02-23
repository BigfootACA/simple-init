/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<comp_libfdt.h>
#include"str.h"
#include"version.h"
#include"logger.h"
#include"internal.h"
#include"fdtparser.h"
#define TAG "info"

int linux_boot_update_info(linux_boot*lb){
	int r,off;
	char buff[BUFSIZ];
	if(!lb->dtb.address||!lb->initrd.address)return 0;

	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_warn(
		-1,"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;

	fdt_setprop_string(
		lb->dtb.address,off,
		"simpleinit,build-time",
		__DATE__
	);

	fdt_setprop_string(
		lb->dtb.address,off,
		"simpleinit,version",
		PRODUCT
	);

	ZeroMem(buff,sizeof(buff));
	UnicodeStrToAsciiStrS(gST->FirmwareVendor,buff,sizeof(buff)-1);
	fdt_setprop_string(
		lb->dtb.address,off,
		"simpleinit,firmware-vendor",
		buff
	);

	fdt_setprop_u32(
		lb->dtb.address,off,
		"simpleinit,firmware-revision",
		gST->FirmwareRevision
	);

	if(lb->config&&lb->config->tag[0])fdt_setprop_string(
		lb->dtb.address,off,
		"simpleinit,config",
		lb->config->tag
	);

	if(lb->status.dtb_id>=0)fdt_setprop_u64(
		lb->dtb.address,off,
		"simpleinit,dtb-id",
		(uint64_t)lb->status.dtb_id
	);

	if(lb->status.dtbo_id>=0)fdt_setprop_u64(
		lb->dtb.address,off,
		"simpleinit,dtbo-id",
		(uint64_t)lb->status.dtbo_id
	);

	return 0;
}

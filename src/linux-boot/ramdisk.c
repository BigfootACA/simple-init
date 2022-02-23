/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<comp_libfdt.h>
#include<stdint.h>
#include"logger.h"
#include"internal.h"
#define TAG "fdt"

int linux_boot_update_initrd(linux_boot*lb){
	int r,off;
	uint64_t start,end;
	if(!lb->dtb.address||!lb->initrd.address)return 0;

	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_warn(
		-1,"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;

	if(lb->config&&lb->config->skip_initrd){
		fdt_delprop(lb->dtb.address,off,"linux,initrd-start");
		fdt_delprop(lb->dtb.address,off,"linux,initrd-end");
		return 0;
	}

	start=(uint64_t)lb->initrd.address;
	end=(uint64_t)(start+lb->initrd.size);

	r=fdt_setprop_u64(
		lb->dtb.address,off,
		"linux,initrd-start",start
	);
	if(r<0)return trlog_warn(
		-1,"update initramfs start failed: %s",
		fdt_strerror(r)
	);

	r=fdt_setprop_u64(
		lb->dtb.address,off,
		"linux,initrd-end",end
	);
	if(r<0)return trlog_warn(
		-1,"update initramfs end failed: %s",
		fdt_strerror(r)
	);

	tlog_debug(
		"update fdt initramfs address done (0x%llx - 0x%llx)",
		(unsigned long long)start,
		(unsigned long long)end
	);
	return 0;
}

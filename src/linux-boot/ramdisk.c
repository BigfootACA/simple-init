/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/LoadFile.h>
#include<Protocol/LoadFile2.h>
#include<comp_libfdt.h>
#include<stdint.h>
#include"logger.h"
#include"internal.h"
#define TAG "fdt"

struct initrd_loader{
        EFI_LOAD_FILE_PROTOCOL load_file;
        linux_boot*lb;
};

static struct{
        VENDOR_DEVICE_PATH vendor;
        EFI_DEVICE_PATH end;
}efi_initrd_device_path={
	.vendor={
		.Header={
			.Type=MEDIA_DEVICE_PATH,
			.SubType=MEDIA_VENDOR_DP,
			.Length={sizeof(efi_initrd_device_path.vendor),0}
		},
		.Guid={0x5568e427,0x68fc,0x4f3d,{0xac,0x74,0xca,0x55,0x52,0x31,0xcc,0x68}}
	},
	.end={
		.Type=END_DEVICE_PATH_TYPE,
		.SubType=END_ENTIRE_DEVICE_PATH_SUBTYPE,
		.Length={sizeof(efi_initrd_device_path.end),0}
	}
};

static EFIAPI EFI_STATUS initrd_load_file(
	EFI_LOAD_FILE_PROTOCOL*this,
	EFI_DEVICE_PATH*fp,
	BOOLEAN bp,
	UINTN*bs,
	void*buf
){
	struct initrd_loader*loader;
	if(!this||!bs||!fp)return EFI_INVALID_PARAMETER;
	if(bp)return EFI_UNSUPPORTED;
	loader=(struct initrd_loader*)this;
	if(!loader->lb||!loader->lb->initrd.address)return EFI_NOT_FOUND;
	if(!buf||*bs<loader->lb->initrd.size){
		*bs=loader->lb->initrd.size;
		return EFI_BUFFER_TOO_SMALL;
	}
	CopyMem(buf,loader->lb->initrd.address,loader->lb->initrd.size);
	*bs=loader->lb->initrd.size;
	return EFI_SUCCESS;
}

int linux_boot_install_initrd(linux_boot*lb){
	EFI_STATUS st;
	EFI_HANDLE handle=NULL;
	struct initrd_loader*loader;
	if(!lb->initrd.address)return 0;
	if(!(loader=AllocatePool(sizeof(struct initrd_loader))))
		return trlog_warn(-1,"allocate initrd loader failed");
	loader->load_file.LoadFile=initrd_load_file;
	loader->lb=lb;
	st=gBS->InstallMultipleProtocolInterfaces(
		&handle,
		&gEfiDevicePathProtocolGuid,
		&efi_initrd_device_path,
		&gEfiLoadFile2ProtocolGuid,
		loader,
		NULL
	);
	if(EFI_ERROR(st))tlog_warn(
		"install initrd loader failed: %p",
		efi_status_to_string(st)
	);
	else tlog_debug(
		"installed initrd 0x%llx",
		(long long)(UINTN)loader
	);
	return 0;
}

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

	start=(uint64_t)(UINTN)lb->initrd.address;
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

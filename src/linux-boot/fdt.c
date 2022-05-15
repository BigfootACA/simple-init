/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/Fdt.h>
#include<comp_libfdt.h>
#include<stdint.h>
#include"str.h"
#include"list.h"
#include"qcom.h"
#include"logger.h"
#include"fdtparser.h"
#include"KernelFdt.h"
#include"internal.h"
#define TAG "fdt"

int linux_boot_set_fdt(linux_boot*lb,void*data,size_t size){
	char buff[64];
	if(CompareMem(&fdt_magic,data,4)!=0)return 0;

	lb->dtb.size=size;
	lb->dtb.mem_pages=EFI_SIZE_TO_PAGES(ALIGN_VALUE(lb->dtb.size,MEM_ALIGN));
	lb->dtb.mem_size=EFI_PAGES_TO_SIZE(lb->dtb.mem_pages);
	if(!(lb->dtb.address=AllocateAlignedPages(lb->dtb.mem_pages,MEM_ALIGN)))
		EDONE(tlog_error("allocate memory for dtb failed"));
	lb->dtb.allocated=true;

	ZeroMem(lb->dtb.address,lb->dtb.mem_size);
	CopyMem(lb->dtb.address,data,lb->dtb.size);

	if(fdt_check_header(data)!=0)
		EDONE(tlog_error("invalid dtb"));

	tlog_info(
		"loaded new dtb %zu bytes (%s)",size,
		make_readable_str_buf(buff,sizeof(buff),size,1,0)
	);
	linux_file_dump("new dtb",&lb->dtb);
	return 0;
	done:
	if(lb->dtb.allocated)FreePages(
		lb->dtb.address-lb->dtb.offset,
		lb->dtb.mem_pages
	);
	ZeroMem(&lb->dtb,sizeof(linux_file_info));
	return -1;
}


int linux_boot_generate_fdt(linux_boot*lb){
	int r=0;
	EFI_STATUS st;
	KERNEL_FDT_PROTOCOL*fdt=NULL;
	if(!lb||!lb->config)return -1;
	if(lb->config->pass_kfdt_dtb){
		st=gBS->LocateProtocol(
			&gKernelFdtProtocolGuid,
			NULL,
			(VOID**)&fdt
		);
		if(!EFI_ERROR(st)&&fdt&&fdt->Fdt){
			tlog_debug("use kernel fdt as dtb");
			linux_file_clean(&lb->dtb);
			if(!linux_file_allocate(&lb->dtb,fdt->FdtSize)){
				tlog_error("failed to allocate for copy kernel fdt");
				return -1;
			}else{
				CopyMem(lb->dtb.address,fdt->Fdt,fdt->FdtSize);
				linux_file_dump("kernel fdt",&lb->dtb);
				return 0;
			}
		}else tlog_warn(
			"failed to locate KernelFdtProtocol: %s",
			efi_status_to_string(st)
		);
	}
	if(!lb->dtb.address){
		tlog_debug("generating empty device tree...");
		lb->dtb.size=MAX_DTB_SIZE/2;
		linux_file_allocate(&lb->dtb,lb->dtb.size);
		r=fdt_create_empty_tree(lb->dtb.address,(int)lb->dtb.size);
		if(r!=0)return trlog_warn(
			-1,"create empty device tree failed: %s",
			fdt_strerror(r)
		);
		fdt_add_subnode(lb->dtb.address,0,"chosen");
		fdt_add_subnode(lb->dtb.address,0,"memory");
	}
	return 0;
}

int linux_boot_update_fdt(linux_boot*lb){
	int r;
	EFI_STATUS st;
	char buf[64],*model;
	if(!lb||!lb->dtb.address)return 0;
	r=fdt_check_header(lb->dtb.address);
	if(r!=0){
		tlog_warn("invalid dtb head: %s",fdt_strerror(r));
		linux_file_clean(&lb->dtb);
		return -1;
	}

	lb->dtb.size=fdt_totalsize(lb->dtb.address);
	if(lb->dtb.size<lb->dtb.mem_size/2){
		tlog_debug(
			"current fdt size %zu bytes (%s)",lb->dtb.size,
			make_readable_str_buf(buf,sizeof(buf),lb->dtb.size,1,0)
		);
		lb->dtb.size=lb->dtb.mem_size/2;
		fdt_set_totalsize(lb->dtb.address,lb->dtb.size);
		tlog_debug(
			"expand fdt size to %zu bytes (%s)",lb->dtb.size,
			make_readable_str_buf(buf,sizeof(buf),lb->dtb.size,1,0)
		);
	}

	if((model=(char*)fdt_getprop(lb->dtb.address,0,"model",NULL)))
		tlog_info("device model: %s",model);

	linux_boot_update_cmdline(lb);
	linux_boot_update_initrd(lb);
	linux_boot_update_memory(lb);
	linux_boot_update_random(lb);
	linux_boot_update_splash(lb);
	linux_boot_update_info(lb);
	linux_boot_update_uefi(lb);

	lb->dtb.size=fdt_totalsize(lb->dtb.address);
	tlog_debug(
		"update fdt done, size %zu bytes (%s)",lb->dtb.size,
		make_readable_str_buf(buf,sizeof(buf),lb->dtb.size,1,0)
	);
	tlog_info(
		"install fdt configuration table 0x%llx",
		(unsigned long long)(UINTN)lb->dtb.address
	);
	st=gBS->InstallConfigurationTable(
		&gFdtTableGuid,
		lb->dtb.address
	);
	if(EFI_ERROR(st))tlog_warn(
		"install fdt table failed: %s",
		efi_status_to_string(st)
	);
	return 0;
}

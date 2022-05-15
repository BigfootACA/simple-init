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
#include"str.h"
#include"logger.h"
#include"internal.h"
#include"KernelFdt.h"
#define TAG "linux"

int fill_kernel_device_path(linux_file_info*fi){
	if(!fi)return -1;
	if(fi->device)return 0;
	ZeroMem(&fi->mem_device,sizeof(fi->mem_device));
	fi->mem_device.dp.Header.Type=HARDWARE_DEVICE_PATH;
	fi->mem_device.dp.Header.SubType=HW_MEMMAP_DP;
	fi->mem_device.dp.Header.Length[0]=(UINT8)(sizeof(MEMMAP_DEVICE_PATH));
	fi->mem_device.dp.Header.Length[1]=(UINT8)(sizeof(MEMMAP_DEVICE_PATH)>>8);
	fi->mem_device.dp.StartingAddress=(EFI_PHYSICAL_ADDRESS)(UINTN)fi->address;
	fi->mem_device.dp.EndingAddress=(EFI_PHYSICAL_ADDRESS)(UINTN)fi->address+fi->size;
	fi->mem_device.proto.Type=END_DEVICE_PATH_TYPE;
	fi->mem_device.proto.SubType=END_ENTIRE_DEVICE_PATH_SUBTYPE;
	fi->mem_device.proto.Length[0]=sizeof(EFI_DEVICE_PATH_PROTOCOL);
	fi->mem_device.proto.Length[1]=0;
	fi->device=(EFI_DEVICE_PATH*)&fi->mem_device;
	return 0;
}

bool is_kernel_magic(linux_file_info*fi,UINTN off,UINT32 require){
	if(!fi||!fi->address||fi->size<=64)return false;
	UINT32 magic=*(UINT32*)(fi->address+off);
	bool match=(require==magic);
	if(!match)tlog_warn(
		"kernel magic at offset 0x%llx not match "
		"(require '%lx', has '%lx')",
		(unsigned long long)off,
		(unsigned long)require,
		(unsigned long)magic
	);
	return match;
}

int linux_file_clean(linux_file_info*fi){
	if(!fi)return -1;
	if(fi->allocated)FreePages(fi->address-fi->offset,fi->mem_pages);
	ZeroMem(fi,sizeof(linux_file_info));
	return 0;
}

static int check_boot(linux_boot*lb){
	if(lb->config&&lb->config->skip_initrd){
		tlog_debug("skip load initramfs");
		linux_file_clean(&lb->initrd);
	}
	if(lb->config&&lb->config->skip_dtb){
		tlog_debug("skip load dtb");
		linux_file_clean(&lb->dtb);
	}
	linux_boot_dump(lb);
	return 0;
}

int linux_boot_prepare(linux_boot*lb){
	if(linux_load_from_config(lb)!=0)
		return trlog_error(-1,"load linux failed");
	if(linux_boot_install_random_seed()!=0)
		return trlog_error(-1,"install random seed failed");
	if(linux_boot_uncompress_kernel(lb)!=0)
		return trlog_error(-1,"decompress kernel failed");
	if(linux_boot_select_fdt(lb)!=0)
		return trlog_error(-1,"select fdt failed");
	if(linux_boot_apply_dtbo(lb)!=0)
		return trlog_error(-1,"apply dtbo failed");
	if(linux_boot_generate_fdt(lb)!=0)
		return trlog_error(-1,"generate fdt failed");
	if(linux_boot_move(lb)!=0)
		return trlog_error(-1,"move load failed");
	if(linux_boot_install_initrd(lb)!=0)
		return trlog_error(-1,"install initrd failed");
	if(linux_boot_update_fdt(lb)!=0)
		return trlog_error(-1,"update fdt failed");
	if(check_boot(lb)!=0)
		return trlog_error(-1,"check boot failed");
	return 0;
}

int linux_boot_execute(linux_boot*lb){
	if(!lb->kernel.address){
		tlog_error("kernel not loaded, abort boot...");
		return -1;
	}
	switch(lb->arch){
		case ARCH_UEFI:boot_linux_uefi(lb);break;
		#if defined(__arm__)||defined(__aarch64__)
		case ARCH_ARM32:case ARCH_ARM64:boot_linux_arm(lb);break;
		#endif
		default:tlog_error("unsupported boot architecture");
	}
	return -1;
}

linux_boot*linux_boot_new(linux_config*cfg){
	if(!cfg)return NULL;
	linux_boot*boot=AllocateZeroPool(sizeof(linux_boot));
	if(!boot)return NULL;
	linux_boot_init(boot);
	boot->config=cfg;
	boot->status.dtb_id=-1;
	boot->status.dtbo_id=-1;
	return boot;
}

bool linux_boot_match_kfdt_model(linux_boot*lb,const char*model){
	int len=0;
	EFI_STATUS st;
	bool ret=false;
	KERNEL_FDT_PROTOCOL*fdt;
	static bool initialize=false;
	static char*last_model=NULL;
	if(!lb->config->match_kfdt_model)return false;
	if(!initialize){
		initialize=true;
		st=gBS->LocateProtocol(
			&gKernelFdtProtocolGuid,
			NULL,
			(VOID**)&fdt
		);
		if(EFI_ERROR(st)||!fdt||!fdt->Fdt){
			tlog_warn(
				"Kernel Fdt Protocol failed: %s",
				efi_status_to_string(st)
			);
			return false;
		}
		last_model=(char*)fdt_getprop(fdt->Fdt,0,"model",&len);
		if(len<=0)last_model=NULL;
	}
	if(last_model){
		tlog_debug("kernel fdt model: %s",last_model);
		ret=strcmp(last_model,model)==0;
	}
	return ret;
}

void linux_boot_free(linux_boot*lb){
	if(!lb)return;
	if(lb->kernel.address)linux_file_clean(&lb->kernel);
	if(lb->initrd.address)linux_file_clean(&lb->initrd);
	if(lb->dtb.address)linux_file_clean(&lb->dtb);
	FreePool(lb);
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LINUX_BOOT_H
#define _LINUX_BOOT_H
#include<Uefi.h>
#include<Protocol/BlockIo.h>
#include<Protocol/DevicePath.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<stdint.h>
#include"boot.h"
#include"aboot.h"
#include"limits.h"
#include"filesystem.h"

// linux kernel boot arch
typedef enum linux_boot_arch{
	ARCH_UNKNOWN=0,
	ARCH_UEFI,
	ARCH_ARM32,
	ARCH_ARM64,
	ARCH_X86,
	ARCH_X64,
}linux_boot_arch;

// linux file source load from type
typedef enum linux_load_from_type{
	FROM_NONE=0,
	FROM_LOCATE,
	FROM_FILE_PROTOCOL,
	FROM_FILE_SYSTEM_HANDLE,
	FROM_BLOCKIO_PROTOCOL,
	FROM_POINTER,
}linux_load_from_type;

// linux memory region include start and end
typedef struct linux_mem_region{
	uint64_t start;
	uint64_t end;
}linux_mem_region;

// linux load files to specified addresses
typedef struct linux_boot_addresses{
	linux_mem_region load;
	linux_mem_region kernel;
	linux_mem_region initrd;
	linux_mem_region fdt;
}linux_boot_addresses;

// linux file source load from
typedef struct linux_load_from{
	linux_load_from_type type;
	bool enabled;
	size_t size;
	union{
		char locate[PATH_MAX-sizeof(void*)-sizeof(size_t)];
		EFI_FILE_PROTOCOL*file_proto;
		EFI_BLOCK_IO_PROTOCOL*blk_proto;
		void*pointer;
		fsh*fsh;
	};
}linux_load_from;

// linux file source load from
typedef struct qcom_chip_info{
	uint32_t soc_id;
	uint32_t soc_rev;
	uint32_t foundry_id;
	uint32_t variant_major;
	uint32_t variant_minor;
	uint32_t variant_id;
	uint32_t subtype_id;
	uint32_t subtype_ddr;
}qcom_chip_info;

// linux screen info
typedef struct linux_screen_info{
	linux_mem_region splash;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	bool update_splash;
	bool add_simplefb;
}linux_screen_info;

// linux boot config
typedef struct linux_config{
	linux_boot_arch arch;
	bool use_uefi:1;
	bool skip_dtb:1;
	bool skip_dtbo:1;
	bool skip_initrd:1;
	bool skip_efi_memory_map:1;
	bool skip_kfdt_memory:1;
	bool skip_kfdt_cmdline:1;
	bool skip_abootimg_kernel:1;
	bool skip_abootimg_cmdline:1;
	bool skip_abootimg_initrd:1;
	bool skip_dtb_after_kernel:1;
	bool force_dtb_after_kernel:1;
	bool load_custom_address:1;
	bool match_kfdt_model:1;
	bool ignore_dtbo_error:1;
	bool add_uefi_runtime:1;
	bool pass_kfdt_dtb:1;
	bool use_kfdt_ramdisk_kernel:1;
	bool use_kfdt_ramdisk_abootimg:1;
	linux_load_from kernel;
	linux_load_from dtb;
	linux_load_from abootimg;
	list*initrd;
	list*dtbo;
	linux_mem_region memory[8];
	linux_screen_info screen;
	linux_boot_addresses load_address;
	qcom_chip_info info;
	int64_t dtb_id;
	int64_t dtbo_id;
	list*dtb_model;
	list*dtb_compatible;
	list*dtbo_model;
	list*dtbo_compatible;
	char tag[256];
	char cmdline[
		PATH_MAX*2-
		(sizeof(void*)*15)-
		(sizeof(int64_t)*2)-
		(sizeof(char)*256)-
		(sizeof(linux_screen_info))-
		(sizeof(linux_mem_region)*8)-
		(sizeof(qcom_chip_info))
	];
}linux_config;

typedef struct linux_boot linux_boot;

extern void linux_config_free(linux_config*cfg);
extern linux_config*linux_config_new();
extern linux_config*linux_config_new_from_confd(const char*key);
extern linux_boot*linux_boot_new(linux_config*cfg);
extern int linux_boot_prepare(linux_boot*lb);
extern int linux_boot_execute(linux_boot*lb);
extern void linux_boot_free(linux_boot*lb);

#endif

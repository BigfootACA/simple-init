/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LINUX_BOOT_INTERNAL_H
#define _LINUX_BOOT_INTERNAL_H
#include<Uefi.h>
#include<Protocol/BlockIo.h>
#include<Protocol/DevicePath.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<stdint.h>
#include"aboot.h"
#include"locate.h"
#include"linux_boot.h"

// linux kernel arm64 boot main
typedef void(*linux_kernel_arm64_main)(
	UINTN fdt_address,
	UINT64 unknown0,
	UINT64 unknown1,
	UINT64 unknown2
);

// linux kernel arm32 boot main
typedef void(*linux_kernel_arm32_main)(
	UINT32 zero,
	UINT32 arch,
	UINTN fdt_address
);

// kernel file device path
typedef struct kernel_device_path{
	MEMMAP_DEVICE_PATH dp;
	EFI_DEVICE_PATH_PROTOCOL proto;
	char p0[
		32-
		sizeof(EFI_DEVICE_PATH_PROTOCOL)-
		sizeof(MEMMAP_DEVICE_PATH)
	];
}kernel_device_path;

// linux load file info
typedef struct linux_file_info{
	bool allocated;
	bool decompressed;
	EFI_DEVICE_PATH_PROTOCOL*device;
	kernel_device_path mem_device;
	void*address;
	size_t size;
	size_t mem_size;
	size_t mem_pages;
	size_t offset;
}linux_file_info;

// linux boot status
typedef struct linux_boot_status{
	int64_t dtb_id;
	int64_t dtbo_id;
	bool compressed;
	bool qualcomm;
}linux_boot_status;

// linux boot info
typedef struct linux_boot{
	linux_file_info kernel;
	linux_file_info initrd;
	linux_file_info dtb;
	list*initrd_buf;
	list*dtbo;
	linux_config*config;
	linux_boot_status status;
	linux_boot_arch arch;
	char cmdline[
		(PATH_MAX*2)-
		sizeof(linux_file_info)*3-
		sizeof(linux_boot_arch)-
		sizeof(linux_boot_status)-
		(sizeof(void*)*3)
	];
}linux_boot;

// linux kernel main
typedef union linux_kernel_main{
	UINTN address;
	void*pointer;
	linux_kernel_arm32_main arm32;
	linux_kernel_arm64_main arm64;
}linux_kernel_main;

// src/linux-boot/loader.c: allocate pages memory for file
extern bool linux_file_allocate(linux_file_info*fi,size_t size);

// src/linux-boot/loader.c: load linux file into memory
extern int linux_file_load(linux_file_info*fi,linux_load_from*from);

// src/linux-boot/loader.c: load linux files from confd
extern int linux_load_from_config(linux_boot*lb);

// src/linux-boot/loader.c: init linux boot struct
extern int linux_boot_init(linux_boot*boot);

// src/linux-boot/aboot.c: load abootimg
extern int linux_boot_load_abootimg(linux_boot*lb,aboot_image*img);

// src/linux-boot/aboot.c: load abootimg from filesystem handle
extern int linux_boot_load_abootimg_fsh(linux_boot*lb,fsh*f);

// src/linux-boot/aboot.c: load abootimg from path
extern int linux_boot_load_abootimg_path(linux_boot*lb,char*path);

// src/linux-boot/aboot.c: load abootimg from config
extern int linux_boot_load_abootimg_config(linux_boot*lb);

// src/linux-boot/aboot.c: load abootimg from kernel fdt
extern int linux_boot_load_abootimg_kfdt(linux_boot*lb);

// src/linux-boot/move.c: move files to specified memory region
extern int linux_boot_move(linux_boot*lb);

// src/linux-boot/compress.c: uncompress kernel
extern int linux_boot_uncompress_kernel(linux_boot*lb);

// src/linux-boot/dtbo.c: apply device tree overlay
extern int linux_boot_apply_dtbo(linux_boot*lb);

// src/linux-boot/dtb_sel.c: select device tree when multiple dtb
extern int linux_boot_select_fdt(linux_boot*lb);

// src/linux-boot/fdt.c: set device tree
extern int linux_boot_set_fdt(linux_boot*lb,void*data,size_t size);

// src/linux-boot/fdt.c: generate empty device tree if have no fdt
extern int linux_boot_generate_fdt(linux_boot*lb);

// src/linux-boot/fdt.c: update device tree
extern int linux_boot_update_fdt(linux_boot*lb);

// src/linux-boot/random.c: add linux random seed to system configuration tables
extern int linux_boot_install_random_seed();

// src/linux-boot/random.c: update device tree to add kaslr seed
extern int linux_boot_update_random(linux_boot*lb);

// src/linux-boot/splash.c: update device tree to add bootloader splash framebuffer address
extern int linux_boot_update_splash(linux_boot*lb);

// src/linux-boot/mem.c: update device tree to add memory
extern int linux_boot_update_memory(linux_boot*lb);

// src/linux-boot/cmdline.c: update device tree to add cmdline
extern int linux_boot_update_cmdline(linux_boot*lb);

// src/linux-boot/info.c: update device tree to add boot info
extern int linux_boot_update_info(linux_boot*lb);

// src/linux-boot/uefi.c: update device tree to add uefi info
extern int linux_boot_update_uefi(linux_boot*lb);

// src/linux-boot/initrd.c: update device tree to add initramfs
extern int linux_boot_update_initrd(linux_boot*lb);

// src/linux-boot/initrd.c: install initramfs
extern int linux_boot_install_initrd(linux_boot*lb);

// src/linux-boot/cmdline.c: add cmdline to buffer
extern void linux_boot_append_cmdline(linux_boot*lb,const char*cmdline);

// src/linux-boot/dump.c: dump linux boot info
extern int linux_boot_dump(linux_boot*lb);

// src/linux-boot/dump.c: dump linux file info
extern int linux_file_dump(char*name,linux_file_info*fi);

// src/linux-boot/linux.c: clean linux file info
extern int linux_file_clean(linux_file_info*fi);

// src/linux-boot/linux.c: fill kernel file device if load from memory
extern int fill_kernel_device_path(linux_file_info*fi);

// src/linux-boot/linux.c: check linux kernel magic
extern bool is_kernel_magic(linux_file_info*fi,UINTN off,UINT32 require);

// src/linux-boot/linux.c: check kernel fdt model match
extern bool linux_boot_match_kfdt_model(linux_boot*lb,const char*model);

// src/linux-boot/uefi.c: boot uefi mode linux kernel (efistub)
extern int boot_linux_uefi(linux_boot*boot);

// src/linux-boot/arm.c: boot arm mode kernel (without uefi)
extern int boot_linux_arm(linux_boot*boot);

#define LINUX_ARM32_OFFSET 0x00008000
#define LINUX_ARM64_OFFSET 0x00080000
#define MEM_ALIGN          0x00200000
#define MAX_DTB_SIZE       0x00200000
#define MAX_DTBO_SIZE      0x01800000
#define MAX_KERNEL_SIZE    0x04000000
#define MAGIC_DTBO         0x1EABB7D7
#define MAGIC_KERNEL_ARM32 0x016f2818
#define MAGIC_KERNEL_ARM64 0x644d5241
#define is_kernel_arm32(fi) is_kernel_magic(fi,0x24,MAGIC_KERNEL_ARM32)
#define is_kernel_arm64(fi) is_kernel_magic(fi,0x38,MAGIC_KERNEL_ARM64)
#endif

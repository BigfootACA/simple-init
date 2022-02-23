/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/Rng.h>
#include<comp_libfdt.h>
#include"logger.h"
#include"internal.h"
#include"fdtparser.h"
#define TAG "random"
#define EFI_RANDOM_SEED_SIZE 64U

struct linux_efi_random_seed{
        uint32_t size;
        uint8_t bits[];
};

extern EFI_GUID gLinuxEfiRandomSeedTableGuid;

int linux_boot_update_random(linux_boot*lb){
	int r,off;
	EFI_STATUS st;
	EFI_RNG_PROTOCOL*rng=NULL;
	UINT64 seed=0;
	if(!lb->dtb.address)return 0;

	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_error(
		-1,"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;

	fdt_delprop(lb->dtb.address,off,"kaslr-seed");

	st=gBS->LocateProtocol(&gEfiRngProtocolGuid,NULL,(VOID**)&rng);
	if(EFI_ERROR(st))return trlog_warn(
		0,"failed to locate rng protocol: %s",
		efi_status_to_string(st)
	);

	st=rng->GetRNG(rng,NULL,sizeof(seed),(UINT8*)&seed);
	if(EFI_ERROR(st))return trlog_warn(
		0,"failed to get rng for random seed: %s",
		efi_status_to_string(st)
	);

	r=fdt_setprop_u64(lb->dtb.address,off,"kaslr-seed",seed);
	if(r<0)return trlog_error(
		-1,"set kaslr seed failed: %s",
		fdt_strerror(r)
	);

	return 0;
}

int linux_boot_install_random_seed(){
	EFI_STATUS st;
	EFI_RNG_PROTOCOL*rng=NULL;
	struct linux_efi_random_seed*seed=NULL;

	st=gBS->LocateProtocol(&gEfiRngProtocolGuid,NULL,(VOID**)&rng);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"failed to locate rng protocol: %s",
		efi_status_to_string(st)
	));

	if(!(seed=AllocateRuntimeZeroPool(
		sizeof(struct linux_efi_random_seed)+
		EFI_RANDOM_SEED_SIZE
	)))EDONE(tlog_warn("failed to allocate random seed"));
	seed->size=EFI_RANDOM_SEED_SIZE;

	st=rng->GetRNG(rng,&(EFI_GUID)EFI_RNG_ALGORITHM_RAW,seed->size,seed->bits);
	if(st==EFI_UNSUPPORTED)st=rng->GetRNG(rng,NULL,seed->size,seed->bits);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"failed to get rng for random seed: %s",
		efi_status_to_string(st)
	));

	st=gBS->InstallConfigurationTable(&gLinuxEfiRandomSeedTableGuid,seed);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"failed to install random seed configuration table: %s",
		efi_status_to_string(st)
	));

	return 0;
	done:
	if(seed)FreePool(seed);
	return 0;
}

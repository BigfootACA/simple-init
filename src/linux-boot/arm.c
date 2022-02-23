/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#if defined (__aarch64__) || defined(__arm__)
#include<Uefi.h>
#include<Library/ArmLib.h>
#include<Library/UefiLib.h>
#include<Library/DebugLib.h>
#include<Library/ReportStatusCodeLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/CacheMaintenanceLib.h>
#include"str.h"
#include"confd.h"
#include"logger.h"
#include"internal.h"
#define TAG "linux-arm"

typedef VOID (*AARCH64_CACHE_OPERATION)(UINTN);
extern VOID AArch64DataCacheOperation(IN AARCH64_CACHE_OPERATION DataCacheOperation);
extern VOID EFIAPI ArmInvalidateDataCacheEntryBySetWay(IN UINTN SetWayFormat);

static VOID EFIAPI ArmInvalidateDataCacheInternal(VOID){
	// FUCK ASSERT
	ArmDataSynchronizationBarrier();
	AArch64DataCacheOperation(ArmInvalidateDataCacheEntryBySetWay);
}

static void platform_cleanup(linux_boot*lb){
	DEBUG((EFI_D_INFO,"boot: cleanup platform...\n"));

	ArmDisableBranchPrediction();
	ArmDisableInterrupts();
	ArmDisableAsynchronousAbort();

	ArmInvalidateInstructionCache();
	ArmDisableInstructionCache();

	ArmInvalidateDataCacheInternal();
	ArmDisableDataCache();

	ArmDisableMmu();
	ArmInvalidateTlb();
}

static void exit_boot_services(void){
	UINT32 dv;
	EFI_STATUS st;
	EFI_MEMORY_DESCRIPTOR *map=NULL;
	UINTN size=0,pages=0,mk,ds;

	DEBUG((EFI_D_INFO,"boot: shutdown uefi boot services...\n"));
	do{
		st=gBS->GetMemoryMap(&size,map,&mk,&ds,&dv);
		if(st==EFI_BUFFER_TOO_SMALL){
			pages=EFI_SIZE_TO_PAGES(size)+1;
			map=AllocatePages(pages);
			ASSERT((map!=NULL));
			if(!map)abort();
			st=gBS->GetMemoryMap(&size,map,&mk,&ds,&dv);
		}
		if(!EFI_ERROR(st)){
			st=gBS->ExitBootServices(gImageHandle,mk);
			if(EFI_ERROR(st)){
				FreePages(map,pages);
				map=NULL,size=0;
			}
		}
	}while(EFI_ERROR(st));
}

int boot_linux_arm(linux_boot*boot){
	linux_kernel_main main;

	switch(boot->arch){
		#ifdef __arm__
		case ARCH_ARM32:
			if(!is_kernel_arm32(&boot->kernel))goto inv_kernel;
		break;
		#endif
		#ifdef __aarch64__
		case ARCH_ARM64:
			if(!is_kernel_arm64(&boot->kernel))goto inv_kernel;
		break;
		#endif
		default:return -1;
	}

	if(!boot->dtb.address){
		tlog_error("missing dtb for linux arm boot");
		return -1;
	}

	if(boot->dtb.size>MAX_DTB_SIZE){
		char buf[64];
		tlog_error(
			"dtb too large: %zu bytes (%s)",boot->dtb.size,
			make_readable_str_buf(buf,sizeof(buf),boot->dtb.size,1,0)
		);
		return -1;
	}

	switch(ArmReadCurrentEL()){
		case AARCH64_EL1:tlog_info("in arm64 EL1");break;
		case AARCH64_EL2:tlog_info("in arm64 EL2");break;
		case AARCH64_EL3:tlog_info("in arm64 EL3");break;
		default:tlog_warn("unknown EL");break;
	}

	EfiSignalEventReadyToBoot();
	REPORT_STATUS_CODE(
		EFI_PROGRESS_CODE,(
			EFI_SOFTWARE_DXE_BS_DRIVER|
			EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT
		)
	);

	tlog_info("prepare platform for arm linux boot...");
	confd_save_file(NULL,NULL);
	exit_boot_services();
	platform_cleanup(boot);

	// FIXME: cannot boot qualcomm android kernel now
	main.pointer=boot->kernel.address;
	DEBUG((EFI_D_INFO,"boot: run arm linux kernel main at 0x%llx\n",main.address));
	switch(boot->arch){
		case ARCH_ARM32:
			main.arm32(0,0,(UINTN)boot->dtb.address);
		break;
		case ARCH_ARM64:
			main.arm64((UINTN)boot->dtb.address,0,0,0);
		break;
		default:;
	}

	DEBUG((EFI_D_ERROR,"Boot Failed\n"));
	ASSERT(FALSE);
	while(1)CpuDeadLoop();
	return 0;
	inv_kernel:
	tlog_error("kernel magic invalid");
	return -1;
}
#else
#include"internal.h"
int boot_linux_arm(linux_boot*boot __attribute__((unused))){return -1;}
#endif

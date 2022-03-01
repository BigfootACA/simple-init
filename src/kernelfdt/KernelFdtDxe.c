/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include "Library/MemoryAllocationLib.h"
#include<KernelFdt.h>
#include<Library/PcdLib.h>
#include<Library/DebugLib.h>
#include<Library/UefiDriverEntryPoint.h>
#include<Library/UefiBootServicesTableLib.h>

STATIC EFI_HANDLE Handle = NULL;
STATIC KERNEL_FDT_PROTOCOL KernelFdt;

EFI_STATUS
EFIAPI
KernelFdtMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  EFI_PHYSICAL_ADDRESS FdtStore;
  EFI_PHYSICAL_ADDRESS FdtAddress;
  VOID                 *Fdt;

  FdtStore = PcdGet64(PcdDeviceTreeStore);
  if (FdtStore == 0) {
    DEBUG ((EFI_D_ERROR, "PcdDeviceTreeStore is not set\n"));
    return EFI_NOT_FOUND;
  }

  FdtAddress = *(EFI_PHYSICAL_ADDRESS*)(UINTN)FdtStore;
  if (FdtAddress == 0) {
    DEBUG ((EFI_D_ERROR, "Device Tree Store is NULL\n"));
    return EFI_NOT_FOUND;
  }

  Fdt = get_fdt_from_pointer ((VOID*)(UINTN)FdtAddress);
  if (Fdt == NULL) {
    DEBUG ((EFI_D_ERROR, "Invalid Device Tree\n"));
    return EFI_NOT_FOUND;
  }

  KernelFdt.Fdt = AllocateCopyPool(fdt_totalsize (Fdt), Fdt);
  if (KernelFdt.Fdt == NULL) {
    DEBUG ((EFI_D_ERROR, "Allocate memory failed\n"));
    return EFI_NOT_FOUND;
  }

  KernelFdt.FdtSize = fdt_totalsize (KernelFdt.Fdt);
  DEBUG ((EFI_D_INFO, "Device Tree Address %llx, Size %ld",
    (UINTN)KernelFdt.Fdt,
    KernelFdt.FdtSize
  ));

  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gKernelFdtProtocolGuid,
    &KernelFdt,
    NULL
    );
  return Status;
}

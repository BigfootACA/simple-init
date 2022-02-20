
#ifndef _KERNEL_FDT_H
#define _KERNEL_FDT_H
#include<Uefi.h>
#include"fdtparser.h"
#include"comp_libfdt.h"

typedef struct _KERNEL_FDT_PROTOCOL {
	//
	// Device Tree pointer
	//
	fdt Fdt;

	//
	// Device Tree size
	//
	UINTN FdtSize;
} KERNEL_FDT_PROTOCOL;

extern EFI_GUID gKernelFdtProtocolGuid;
#endif

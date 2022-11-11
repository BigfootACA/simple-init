/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _UEFI_H
#define _UEFI_H
#include<Uefi.h>
#include<Library/UefiBootManagerLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>

// src/lib/uefi.c: dump memory
extern EFIAPI void dump_memory(EFI_PHYSICAL_ADDRESS base,UINT64 size);

// src/lib/uefi.c: set simpleinit efi var
extern EFIAPI EFI_STATUS efi_setvar(CHAR16*key,VOID*buf,UINTN size);

// src/lib/uefi.c: set simpleinit efi string var
extern EFIAPI EFI_STATUS efi_setvar_str(CHAR16*key,CHAR16*str,...);

// src/lib/uefi.c: set simpleinit efi integer var
extern EFIAPI EFI_STATUS efi_setvar_int(CHAR16 *name,UINTN num);

// src/lib/uefi.c: auto allocate buffer and call file protocol GetInfo
extern EFIAPI EFI_STATUS efi_file_get_info(EFI_FILE_PROTOCOL*file,EFI_GUID*guid,UINTN*size,VOID**data);

// src/lib/uefi.c: auto allocate buffer and call file protocol GetInfo with gEfiFileInfoGuid
extern EFIAPI EFI_STATUS efi_file_get_file_info(EFI_FILE_PROTOCOL*file,UINTN*size,EFI_FILE_INFO**data);

// src/lib/uefi.c: auto allocate buffer and read file
extern EFIAPI EFI_STATUS efi_file_read(EFI_FILE_PROTOCOL*file,UINTN size,VOID**data,UINTN*read);

// src/lib/uefi.c: auto allocate buffer and read folder item
extern EFIAPI EFI_STATUS efi_file_read_dir(EFI_FILE_PROTOCOL*file,EFI_FILE_INFO**data);

// src/lib/uefi.c: auto allocate buffer and read whole file
extern EFIAPI EFI_STATUS efi_file_read_whole(EFI_FILE_PROTOCOL*file,VOID**data,UINTN*read);

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LOCATE_H
#define _LOCATE_H
#include<Uefi.h>
#include<Protocol/BlockIo.h>
#include<Protocol/DevicePath.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<stdint.h>
#include"boot.h"
#include"aboot.h"

typedef enum locate_type{
	LOCATE_NONE,
	LOCATE_FILE,
	LOCATE_BLOCK,
}locate_type;

typedef struct locate_ret{
	char tag[256];
	char path[PATH_MAX-256-sizeof(void*)*7];
	locate_type type;
	EFI_HANDLE*hand;
	union{
		struct{
			EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs;
			EFI_FILE_PROTOCOL*file;
			EFI_FILE_PROTOCOL*root;
		};
		struct{
			EFI_BLOCK_IO_PROTOCOL*block;
		};
	};
	EFI_DEVICE_PATH_PROTOCOL*device;
	EFI_PARTITION_INFO_PROTOCOL*part;
}locate_ret;

// src/locate/locate.c: parse path for get target block or file
extern bool boot_locate(locate_ret*ret,const char*pattern);

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _BOOT_LOCATE_H
#define _BOOT_LOCATE_H
#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/BlockIo.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include<Guid/FileSystemInfo.h>
#include<Guid/FileSystemVolumeLabelInfo.h>
#include<stdlib.h>
#include"str.h"
#include"boot.h"
#include"uefi.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"locate.h"
#define TAG "locate"
#define BASE "locates"

#define GSTR(key,def) confd_get_string_dict(BASE,loc->tag,key,def)
#define GINT(key,def) confd_get_integer_dict(BASE,loc->tag,key,def)
#define GBOOL(key,def) confd_get_boolean_dict(BASE,loc->tag,key,def)

typedef struct locate_dest{
	char tag[255];
	bool dump;
	EFI_HANDLE*file_hand;
	EFI_FILE_PROTOCOL*root;
	EFI_BLOCK_IO_PROTOCOL*block_proto;
	EFI_PARTITION_INFO_PROTOCOL*part_proto;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*file_proto;
}locate_dest;

typedef enum locate_match_state{
	MATCH_NONE,
	MATCH_SKIP,
	MATCH_INVALID,
	MATCH_SUCCESS,
	MATCH_FAILED
}locate_match_state;

typedef locate_match_state(*locate_match)(locate_dest*loc);
extern locate_match locate_matches[];
#endif

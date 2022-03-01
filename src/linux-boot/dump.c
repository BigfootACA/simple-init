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
#include"str.h"
#include"logger.h"
#include"internal.h"
#define TAG "dump"

int linux_file_dump(char*name,linux_file_info*fi){
	char buf[64];
	if(!fi)return -1;
	if(!fi->address)return 0;
	if(name)tlog_verbose("%s:",name);
	tlog_verbose(
		"    address  : 0x%016llx",
		(unsigned long long)(UINTN)fi->address
	);
	tlog_verbose(
		"    size     : 0x%08llx (%s)",
		(unsigned long long)fi->size,
		make_readable_str_buf(buf,sizeof(buf),fi->size,1,0)
	);
	tlog_verbose(
		"    mem size : 0x%08llx (%s %d%%)",
		(unsigned long long)fi->mem_size,
		make_readable_str_buf(buf,sizeof(buf),fi->mem_size,1,0),
		(int)(fi->size*100/fi->mem_size)
	);
	tlog_verbose(
		"    pages    : 0x%llx",
		(unsigned long long)fi->mem_pages
	);
	return 0;
}

int linux_boot_dump(linux_boot*lb){
	if(!lb)return -1;
	linux_file_dump("kernel",&lb->kernel);
	linux_file_dump("initramfs",&lb->initrd);
	linux_file_dump("devicetree",&lb->dtb);
	tlog_info("kernel cmdline: %s",lb->cmdline);
	return 0;
}

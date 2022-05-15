/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdint.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<comp_libfdt.h>
#include"str.h"
#include"logger.h"
#include"compress.h"
#include"internal.h"
#define TAG "compress"

int linux_boot_uncompress_kernel(linux_boot*lb){
	char buf[64];
	compressor*comp;
	size_t pos=0,len=0;
	size_t mem_size=0,mem_pages=0;
	unsigned char*out=NULL,*inp=(unsigned char*)lb->kernel.address;
	if(!lb||!inp)return -1;
	if(!(comp=compressor_get_by_format(inp,lb->kernel.size)))return 0;

	tlog_debug(
		"found %s compressed kernel size %zu bytes (%s)",
		compressor_get_name(comp),lb->kernel.size,
		make_readable_str_buf(buf,sizeof(buf),lb->kernel.size,1,0)
	);

	mem_pages=EFI_SIZE_TO_PAGES(MAX_KERNEL_SIZE);
	mem_size=EFI_PAGES_TO_SIZE(mem_pages);
	if(!(out=AllocateAlignedPages(mem_pages,MEM_ALIGN)))
		EDONE(tlog_error("allocate for compress buffer failed"));
	ZeroMem(out,mem_size);

	if(compressor_decompress(
		comp,
		inp,lb->kernel.size,
		out+lb->kernel.offset,
		mem_size-lb->kernel.offset,
		&pos,&len
	)!=0)EDONE(tlog_error("decompress kernel failed at %zu",pos));
	tlog_info(
		"decompressed kernel size %zu (%s %d%%)",len,
		make_readable_str_buf(buf,sizeof(buf),len,1,0),
		(int)(lb->kernel.size*100/len)
	);

	if(pos<lb->kernel.size){
		bool should=!lb->dtb.address;
		if(lb->config->skip_dtb_after_kernel)should=false;
		if(lb->config->force_dtb_after_kernel)should=true;
		if(should){
			tlog_debug(
				"found dtb after compressed kernel at 0x%llx",
				(unsigned long long)pos
			);
			linux_boot_set_fdt(lb,inp+pos,lb->kernel.size-pos);
			CopyMem(
				&lb->dtb.mem_device,
				&lb->kernel.mem_device,
				sizeof(lb->dtb.mem_device)
			);
			lb->dtb.device=lb->kernel.device;
		}else tlog_debug("skip dtb after compressed kernel");
	}

	if(lb->kernel.allocated)FreePages(
		lb->kernel.address-lb->kernel.offset,
		lb->kernel.mem_pages
	);
	lb->kernel.address=out+lb->kernel.offset;
	lb->kernel.allocated=true;
	lb->kernel.decompressed=true;
	lb->kernel.mem_pages=mem_pages;
	lb->kernel.mem_size=mem_size;
	lb->kernel.size=len;
	lb->status.compressed=true;
	linux_file_dump("decompressed kernel",&lb->kernel);
	return 0;
	done:
	if(out)FreePages(out,mem_pages);
	return -1;
}

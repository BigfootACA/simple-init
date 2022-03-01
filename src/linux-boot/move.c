/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/PcdLib.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Protocol/SimpleFileSystem.h>
#include"str.h"
#include"logger.h"
#include"internal.h"
#define TAG "move"

#define COMPARE(left,right,sign,type) (((type)(left))sign((type)(right)))
#define UCOMPARE(left,right,sign) COMPARE(left,right,sign,UINT64)
#define IN_RANGE(addr,start,end) (UCOMPARE(addr,start,>)&&UCOMPARE(addr,end,<))
#define IN_SIZE(addr,start,size) IN_RANGE((UINT64)(addr),(UINT64)(start),(UINT64)((start)+(size)))
#define IN_RANGE_LI(addr,li) IN_RANGE(addr,li->start,li->end)
#define IN_RANGE_TLI(li1,li2) (IN_RANGE_LI(li1->start,li2)||IN_RANGE_LI(li1->end,li2))
#define IN_SIZE_FI(addr,fi) IN_RANGE(addr,(UINTN)fi->address,(UINTN)fi->size)
#define IN_SIZE_TLI(li,fi) (IN_SIZE_FI(li->start,fi)||IN_SIZE_FI(li->end,fi))

static bool check_load_info(linux_mem_region*li){
	if(!li)return false;
	if(li->start==0&&li->end==0)return true;
	if(li->start==0||li->end==0)return false;
	if(li->end-li->start>0)return true;
	return false;
}

static bool check_load_info_other(linux_mem_region*li,linux_mem_region*sec){
	if(!li||!sec)return false;
	if(!check_load_info(li)||!check_load_info(sec))return false;
	if(li->start==0&&sec->start==0)return true;
	if(li->start==0||sec->start==0)return false;
	if(IN_RANGE_TLI(li,sec)||IN_RANGE_TLI(sec,li))return false;
	return true;
}
static bool check_load_info_file(linux_mem_region*li,linux_file_info*fi){
	if(!li||!fi)return false;
	if(li->start==0||fi->address==0)return true;
	if(IN_RANGE_LI((UINTN)fi->address,li))return false;
	if(IN_SIZE_TLI(li,fi))return false;
	return true;
}
static bool check_load_info_boot(linux_mem_region*li,linux_boot*lb){
	if(!li||!lb)return false;
	if(!check_load_info_file(li,&lb->kernel))return false;
	if(!check_load_info_file(li,&lb->initrd))return false;
	if(!check_load_info_file(li,&lb->dtb))return false;
	return true;
}

static bool check_boot_load_info(linux_boot_addresses*bli,linux_boot*lb){
	if(!bli||!lb)return false;
	if(!check_load_info_other(&bli->kernel,&bli->initrd))return false;
	if(!check_load_info_other(&bli->kernel,&bli->fdt))return false;
	if(!check_load_info_other(&bli->initrd,&bli->fdt))return false;
	if(!check_load_info_boot(&bli->load,lb)||!check_load_info_boot(&bli->kernel,lb))return false;
	if(!check_load_info_boot(&bli->initrd,lb)||!check_load_info_boot(&bli->fdt,lb))return false;
	if(!bli->kernel.start&&!bli->load.start)return false;
	if(!bli->initrd.start&&!bli->load.start)return false;
	if(!bli->fdt.start&&!bli->load.start)return false;
	return true;
}

static int do_move(linux_mem_region*tgt,linux_file_info*src,size_t offset){
	char buf[64];
	if(!tgt||!src||!src->address)return -1;
	tgt->start=ALIGN_VALUE(tgt->start,MEM_ALIGN)+offset;
	if(tgt->end-tgt->start<src->size)
		return trlog_warn(-1,"memory too small for load");
	tlog_debug(
		"move from 0x%llx to 0x%llx size %zu bytes (%s)",
		(unsigned long long)(UINTN)src->address,
		(unsigned long long)tgt->start,
		src->size,
		make_readable_str_buf(buf,sizeof(buf),src->size,1,0)
	);
	CopyMem((VOID*)(UINTN)tgt->start,src->address,src->size);
	if(src->allocated)FreePages(src->address-src->offset,src->mem_pages);
	src->address=(void*)(UINTN)tgt->start;
	src->mem_pages=EFI_SIZE_TO_PAGES(MIN(
		ALIGN_VALUE(src->size,MEM_ALIGN),
		tgt->end-tgt->start
	));
	src->offset=offset;
	src->mem_size=EFI_PAGES_TO_SIZE(src->mem_pages);
	src->allocated=false;
	tgt->start+=ALIGN_VALUE(src->mem_size,MEM_ALIGN);
	return 0;
}

static void do_erase_load(linux_mem_region*load){
	if(!load||!load->start)return;
	ZeroMem((VOID*)(UINTN)load->start,load->end-load->start);
}

int linux_boot_move(linux_boot*lb){
	if(!lb->config->load_custom_address)return 0;
	linux_boot_addresses*info=&lb->config->load_address;

	if(!check_boot_load_info(info,lb))
		return trlog_error(-1,"invalid addresses, skip move");
	tlog_debug("try erase memory");
	do_erase_load(&info->load);
	do_erase_load(&info->kernel);
	do_erase_load(&info->initrd);
	do_erase_load(&info->fdt);
	tlog_debug("erase memory done, try move");
	size_t koff=0;
	switch(lb->arch){
		case ARCH_ARM32:koff=LINUX_ARM32_OFFSET;break;
		case ARCH_ARM64:koff=LINUX_ARM64_OFFSET;break;
		default:;
	}
	do_move((info->kernel.start?&info->kernel:&info->load),&lb->kernel,koff);
	do_move((info->initrd.start?&info->initrd:&info->load),&lb->initrd,0);
	do_move((info->fdt.start?&info->fdt:&info->load),&lb->dtb,0);
	tlog_debug("move done");
	return 0;
}

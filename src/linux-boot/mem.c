/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<comp_libfdt.h>
#include<stdint.h>
#include"str.h"
#include"logger.h"
#include"fdtparser.h"
#include"internal.h"
#include"KernelFdt.h"
#include"ddrgetconfig.h"
#include"rampartition.h"
#define TAG "memory"

static int fdt_append_reg(void*fdt,int off,bool use32,UINTN val){
	return use32?
		fdt_appendprop_u32(fdt,off,"reg",(uint32_t)val):
		fdt_appendprop_u64(fdt,off,"reg",(uint64_t)val);
}

static int fdt_add_memory(linux_boot*lb,UINTN addr,UINTN size){
	int ret=0;
	char buf[64];
	static int off=-1;
	static void*dtb=NULL;
	static bool use32=false;
	if(dtb!=lb->dtb.address){
		off=fdt_path_offset(lb->dtb.address,"/memory");
		if(off<0){
			off=fdt_add_subnode(lb->dtb.address,0,"/memory");
			if(off<0)return trlog_warn(
				-1,
				"get memory node failed: %s",
				fdt_strerror(off)
			);
			fdt_setprop_string(lb->dtb.address,off,"device_type","memory");
		}
		fdt_delprop(lb->dtb.address,off,"reg");
		dtb=lb->dtb.address;
	}
	tlog_info(
		"memory: 0x%016llx - 0x%016llx (%llu bytes / %s)",
		(unsigned long long)addr,
		(unsigned long long)addr+size,
		(unsigned long long)size,
		make_readable_str_buf(buf,sizeof(buf),size,1,0)
	);

	ret=fdt_append_reg(lb->dtb.address,off,use32,addr);
	if(ret<0)return trlog_warn(
		ret,
		"add memory address failed: %s",
		fdt_strerror(ret)
	);

	ret=fdt_append_reg(lb->dtb.address,off,use32,size);
	if(ret<0)tlog_warn(
		"add memory size failed: %s",
		fdt_strerror(ret)
	);
	return ret;
}

static int fdt_add_merge_memory(linux_boot*lb,UINTN addr,UINTN size){
	STATIC UINTN last_end=0,start=0;
	UINTN end=addr+size;
	if(size!=0){
		if(last_end!=addr){
			if(start>0&&end>start)fdt_add_memory(lb,start,end-start);
			start=addr;
		}
		last_end=end;
	}else if(start>0&&last_end>start)fdt_add_memory(lb,start,last_end-start);
	return 0;
}

static int update_from_memory_map(linux_boot*lb){
	EFI_STATUS st;
	EFI_MEMORY_DESCRIPTOR*mm=NULL,*md;
	UINTN ms=0,mk=0,ds=0;
	UINT32 dv=0;
	do{
		ms+=sizeof(EFI_MEMORY_DESCRIPTOR);
		if(mm)FreePool(mm);
		if(!(mm=AllocatePool(ms+(2*ds))))break;
		st=gBS->GetMemoryMap(&ms,mm,&mk,&ds,&dv);
	}while(st==EFI_BUFFER_TOO_SMALL);
	if(EFI_ERROR(st)){
		tlog_warn(
			"get memory map failed: %s",
			efi_status_to_string(st)
		);
		if(mm)FreePool(mm);
		return -1;
	}
	tlog_debug("update memory from uefi memory map");
	for(md=mm;(void*)md<(void*)mm+ms;md=(void*)md+ds){
		switch(md->Type){
			case EfiReservedMemoryType:
			case EfiLoaderCode:
			case EfiLoaderData:
			case EfiBootServicesCode:
			case EfiBootServicesData:
			case EfiRuntimeServicesCode:
			case EfiRuntimeServicesData:
			case EfiConventionalMemory:
			case EfiACPIReclaimMemory:
			case EfiACPIMemoryNVS:
			case EfiPalCode:
				fdt_add_merge_memory(
					lb,md->PhysicalStart,
					EFI_PAGES_TO_SIZE(md->NumberOfPages)
				);
			break;
			default:continue;
		}
	}
	fdt_add_merge_memory(lb,0,0);
	return 0;
}

static int update_from_conf(linux_boot*lb){
	int r=-1;
	if(lb->config)for(
		size_t i=0;
		i<ARRAY_SIZE(lb->config->memory);
		i++
	){
		linux_mem_region*reg=&lb->config->memory[i];
		UINTN size=reg->end-reg->start;
		if(reg->start<=0||reg->end<=0||size<=0)continue;
		if(r==-1)tlog_debug("update memory from config");
		fdt_add_memory(lb,(UINTN)reg->start,size);
		r=0;
	}
	return r;
}

static int update_from_kernel_fdt(linux_boot*lb){
	int node=0,r=-1;
	EFI_STATUS st;
	uint64_t base=0,size=0;
	KERNEL_FDT_PROTOCOL*fdt;
	if(lb->config->skip_kfdt_memory)return r;
	st=gBS->LocateProtocol(
		&gKernelFdtProtocolGuid,
		NULL,
		(VOID**)&fdt
	);
	if(EFI_ERROR(st)||!fdt||!fdt->Fdt)return r;
	tlog_debug("update memory from kernel fdt");
	while(fdt_get_memory(fdt->Fdt,node,&base,&size)){
		fdt_add_memory(lb,(UINTN)base,(UINTN)size);
		node++,r=0;
	}
	return r;
}

static int update_from_ram_partition(linux_boot*lb){
	int r=-1;
	UINT32 cnt=0;
	EFI_STATUS st;
	RamPartitionEntry *parts=NULL;
	EFI_RAMPARTITION_PROTOCOL*proto=NULL;
	st=gBS->LocateProtocol(&gEfiRamPartitionProtocolGuid,NULL,(VOID**)&proto);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"locate ram partition protocol failed: %s "
		"(not qualcomm platform or forget EnvDxe?)",
		efi_status_to_string(st)
	));
	st=proto->GetRamPartitions(proto,parts,&cnt);
	if(st==EFI_BUFFER_TOO_SMALL){
		if(!(parts=AllocateZeroPool(cnt*sizeof(RamPartitionEntry))))
			EDONE(tlog_warn("allocate memory for memory partitions failed"));
		st=proto->GetRamPartitions(proto,parts,&cnt);
	}
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get ram partitions failed: %s",
		efi_status_to_string(st)
	));
	for(UINT32 i=0;i<cnt;i++){
		if(parts[i].Base<=0||parts[i].AvailableLength<=0)continue;
		fdt_add_memory(lb,parts[i].Base,parts[i].AvailableLength);
		r=0;
	}
	done:
	if(parts)FreePool(parts);
	return r;
}

static int update_ddr_info(linux_boot*lb){
	int ret,off;
	EFI_STATUS st;
	char rank_prop[32],hbb_prop[32];
	struct ddr_details_entry_info *ddr;
	EFI_DDRGETINFO_PROTOCOL*proto;

	off=fdt_path_offset(lb->dtb.address,"/memory");
	if(off<0)EDONE(tlog_warn(
		"get memory node failed: %s",
		fdt_strerror(off)
	));

	if(!(ddr=AllocateZeroPool(sizeof(struct ddr_details_entry_info))))
		EDONE(tlog_warn("allocate pool for ddr details failed"));
	st=gBS->LocateProtocol(&gEfiDDRGetInfoProtocolGuid,NULL,(VOID**)&proto);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"locate ddr get info protocol failed: %s "
		"(not qualcomm platform or forget DDRInfoDxe?)",
		efi_status_to_string(st)
	));
	st=proto->GetDDRDetails(proto,ddr);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get ddr details failed: %s",
		efi_status_to_string(st)
	));
	tlog_debug("ddr device type: %d",ddr->device_type);
	ret=fdt_appendprop_u32(lb->dtb.address,off,"ddr_device_type",(UINT32)ddr->device_type);
	if(ret!=0)EDONE(tlog_warn(
		"set ddr_device_type failed: %s",
		fdt_strerror(off)
	));
	if(proto->Revision<EFI_DDRGETINFO_PROTOCOL_REVISION)
		EDONE(tlog_warn("hbb not supported");
	tlog_debug("ddr channels num: %d",ddr->num_channels));
	for(UINT8 chan=0;chan<ddr->num_channels;chan++){
		ZeroMem(rank_prop,sizeof(rank_prop));
		AsciiSPrint(rank_prop,sizeof(rank_prop),"ddr_device_rank_ch%d",chan);
		ret=fdt_appendprop_u32(lb->dtb.address,off,rank_prop,(UINT32)ddr->num_ranks[chan]);
		if(ret!=0)EDONE(tlog_warn(
			"add ddr_device_rank_ch%d failed: %s",
			chan,fdt_strerror(off)
		));
		for(UINT8 rank=0;rank<ddr->num_ranks[chan];rank++){
			ZeroMem(hbb_prop,sizeof(hbb_prop));
			AsciiSPrint(hbb_prop,sizeof(hbb_prop),"ddr_device_hbb_ch%d_rank%d",chan,rank);
			ret=fdt_appendprop_u32(lb->dtb.address,off,hbb_prop,(UINT32)ddr->hbb[chan][rank]);
			if(ret!=0)EDONE(tlog_warn(
				"add ddr_device_hbb_ch%d_rank%d failed: %s",
				chan,rank,fdt_strerror(off)
			));
		}
	}
	done:
	return 0;
}

int linux_boot_update_memory(linux_boot*lb){
	if(!lb->dtb.address)return 0;
	if(lb->config->pass_kfdt_dtb)return 0;
	if(update_ddr_info(lb)!=0)return -1;
	if(update_from_conf(lb)==0)return 0;
	if(update_from_ram_partition(lb)==0)return 0;
	if(update_from_kernel_fdt(lb)==0)return 0;
	if(update_from_memory_map(lb)==0)return 0;
	tlog_warn("no avaliable memory update method");
	return -1;
}

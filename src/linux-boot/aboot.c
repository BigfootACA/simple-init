/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include"str.h"
#include"list.h"
#include"aboot.h"
#include"logger.h"
#include"compress.h"
#include"internal.h"
#include"fdtparser.h"
#include"KernelFdt.h"
#define TAG "abootimg"

static void load_kernel(linux_boot*lb,aboot_image*img){
	if(!abootimg_have_kernel(img))return;
	if(lb->config->skip_abootimg_kernel)
		tlog_debug("skip kernel from abootimg");
	linux_file_clean(&lb->kernel);
	switch(lb->arch){
		case ARCH_ARM32:lb->kernel.offset=LINUX_ARM32_OFFSET;break;
		case ARCH_ARM64:lb->kernel.offset=LINUX_ARM64_OFFSET;break;
		default:;
	}
	lb->kernel.size=abootimg_get_kernel_size(img);
	if(linux_file_allocate(&lb->kernel,lb->kernel.size)){
		abootimg_copy_kernel(img,lb->kernel.address,lb->kernel.mem_size);
		tlog_info("loaded kernel image from abootimg");
		linux_file_dump("abootimg kernel",&lb->kernel);
	}else{
		ZeroMem(&lb->kernel,sizeof(linux_file_info));
		tlog_warn("allocate pages for kernel failed");
	}
}

static void load_ramdisk(linux_boot*lb,aboot_image*img){
	if(!abootimg_have_ramdisk(img))return;
	if(lb->config->skip_abootimg_initrd)
		tlog_debug("skip ramdisk from abootimg");
	linux_file_info*f=malloc(sizeof(linux_file_info));
	int cur=list_count(lb->initrd_buf);
	if(cur<0)cur=0;
	if(!f){
		tlog_warn("failed to allocate memory for initrd file info");
		return;
	}
	ZeroMem(f,sizeof(linux_file_info));
	f->size=abootimg_get_ramdisk_size(img);
	if(linux_file_allocate(f,f->size)){
		abootimg_copy_ramdisk(img,f->address,f->mem_size);
		tlog_info("loaded initramfs #%d image from abootimg",cur);
		linux_file_dump("abootimg initramfs",f);
		list_obj_add_new(&lb->initrd_buf,f);
	}else{
		tlog_warn("allocate pages for initramfs failed");
		free(f);
	}
}

int linux_boot_load_abootimg(linux_boot*lb,aboot_image*img){
	const char*name,*cmdline;

	if(abootimg_is_invalid(img))
		return trlog_warn(-1,"invalid android boot image");

	name=abootimg_get_name(img);
	cmdline=abootimg_get_cmdline(img);
	if(name&&*name)tlog_info("image name '%s'",name);

	load_kernel(lb,img);
	load_ramdisk(lb,img);

	if(abootimg_have_second(img))
		tlog_warn("second stage bootloader is not supported");

	if(!lb->config->skip_abootimg_cmdline)linux_boot_append_cmdline(lb,cmdline);
	else tlog_debug("skip cmdline from abootimg");

	return 0;
}

int linux_boot_load_abootimg_fsh(linux_boot*lb,fsh*f){
	int ret=-1;
	char buff[512];
	aboot_image*img=NULL;
	fs_get_path(f,buff,sizeof(buff));
	tlog_debug(
		"parse android boot image from %s ...",
		buff[0]?buff:"(unknown)"
	);
	if(!(img=abootimg_load_from_fsh(f)))
		return trlog_warn(-1,"parse android boot image failed");
	ret=linux_boot_load_abootimg(lb,img);
	abootimg_free(img);
	return ret;
}

int linux_boot_load_abootimg_path(linux_boot*lb,char*path){
	int r=-1;
	fsh*f=NULL;
	if(fs_open(NULL,&f,path,FILE_FLAG_READ)==0){
		r=linux_boot_load_abootimg_fsh(lb,f);
		fs_close(&f);
	}
	return r;
}

int linux_boot_load_abootimg_config(linux_boot*lb){
	aboot_image*img;
	if(!lb||!lb->config)return -1;
	linux_load_from*from=&lb->config->abootimg;
	if(!from->enabled)return 0;
	switch(from->type){
		case FROM_LOCATE:return linux_boot_load_abootimg_path(lb,from->locate);
		case FROM_FILE_SYSTEM_HANDLE:return linux_boot_load_abootimg_fsh(lb,from->fsh);
		case FROM_POINTER:img=abootimg_load_from_memory(from->pointer,from->size);break;
		case FROM_BLOCKIO_PROTOCOL:img=abootimg_load_from_blockio(from->blk_proto);break;
		case FROM_FILE_PROTOCOL:img=abootimg_load_from_fp(from->file_proto);break;
		default:return trlog_warn(-1,"unknown load from type");
	}
	if(!img)return trlog_warn(-1,"parse android boot image failed");
	int ret=linux_boot_load_abootimg(lb,img);
	abootimg_free(img);
	return ret;
}

int linux_boot_load_abootimg_kfdt(linux_boot*lb){
	int ret;
	char buf[64];
	EFI_STATUS st;
	aboot_image*img;
	compressor*comp=NULL;
	void*initrd=NULL,*bo=NULL;
	KERNEL_FDT_PROTOCOL*fdt=NULL;
	size_t size=0,bs=0x8000000,pos=0,len=0;
	if(!lb||!lb->config)return -1;
	if(!lb->config->use_kfdt_ramdisk_abootimg)return 0;
	st=gBS->LocateProtocol(
		&gKernelFdtProtocolGuid,
		NULL,
		(VOID**)&fdt
	);
	if(EFI_ERROR(st)||!fdt||!fdt->Fdt)return trlog_warn(
		-1,"failed to locate KernelFdtProtocol: %s",
		efi_status_to_string(st)
	);
	if(!fdt_get_initrd(fdt->Fdt,&initrd,&size))
		return trlog_warn(-1,"get initrd from kfdt failed");
	if((comp=compressor_get_by_format(initrd,size))){
		tlog_debug(
			"found %s compressed abootimg",
			compressor_get_name(comp)
		);
		if(!(bo=AllocateZeroPool(bs)))
			return trlog_debug(-1,"alloc for decompress failed");
		if(compressor_decompress(
			comp,initrd,size,
			bo,bs,&pos,&len
		)!=0){
			FreePool(bo);
			return trlog_error(-1,"decompress kernel failed at %zu",pos);
		}
		tlog_info(
			"decompressed abootimg size %zu (%s %d%%)",len,
			make_readable_str_buf(buf,sizeof(buf),len,1,0),
			(int)(size*100/len)
		);
		if(pos<size)tlog_warn(
			"ignore %zu bytes after compress data",
			size-pos
		);
		initrd=bo,size=len;
	}
	if((img=abootimg_load_from_memory(initrd,size))){
		ret=linux_boot_load_abootimg(lb,img);
		abootimg_free(img);
	}else ret=trlog_error(-1,"parse abootimg failed");
	if(bo)FreePool(bo);
	return ret;
}

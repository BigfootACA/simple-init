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
#include"list.h"
#include"aboot.h"
#include"logger.h"
#include"internal.h"
#define TAG "abootimg"

static void load_kernel(linux_boot*lb,aboot_image*img){
	if(!abootimg_have_kernel(img))return;
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

	linux_boot_append_cmdline(lb,cmdline);
	return 0;
}

int linux_boot_load_abootimg_locate(linux_boot*lb,locate_ret*loc){
	int ret=-1;
	aboot_image*img=NULL;
	switch(loc->type){
		case LOCATE_FILE:
			if(!loc->path[0])break;
			tlog_debug(
				"parse android boot image from file '%s'...",
				loc->path
			);
			img=abootimg_load_from_fp(loc->file);
		break;
		case LOCATE_BLOCK:
			if(loc->path[0])break;
			tlog_debug("parse android boot image from block...");
			img=abootimg_load_from_blockio(loc->block);
		break;
		default:return trlog_warn(-1,"unknown locate type");
	}
	if(!img)return trlog_warn(-1,"parse android boot image failed");
	ret=linux_boot_load_abootimg(lb,img);
	abootimg_free(img);
	return ret;
}

int linux_boot_load_abootimg_path(linux_boot*lb,char*path){
	locate_ret loc;
	if(!boot_locate(&loc,path))return -1;
	return linux_boot_load_abootimg_locate(lb,&loc);
}

int linux_boot_load_abootimg_config(linux_boot*lb){
	aboot_image*img;
	if(!lb||!lb->config)return -1;
	linux_load_from*from=&lb->config->abootimg;
	if(!from->enabled)return 0;
	switch(from->type){
		case FROM_LOCATE:return linux_boot_load_abootimg_path(lb,from->locate);
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

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
#include"aboot.h"
#include"logger.h"
#include"internal.h"
#define TAG "abootimg"

#define LOAD(type,tag)\
	if(abootimg_have_##type(img)){\
		linux_file_clean(&lb->tag);\
		lb->tag.size=abootimg_get_##type##_size(img);\
		if(linux_file_allocate(&lb->tag,lb->tag.size)){\
			abootimg_copy_##type(img,lb->tag.address,lb->tag.mem_size);\
			tlog_info("loaded "#tag" image from abootimg");\
			linux_file_dump("abootimg "#type,&lb->tag);\
		}else{\
			ZeroMem(&lb->tag,sizeof(linux_file_info));\
			tlog_warn("allocate pages for "#tag" failed");\
		}\
	}

int linux_boot_load_abootimg(linux_boot*lb,aboot_image*img){
	const char*name,*cmdline;

	if(abootimg_is_invalid(img))
		return trlog_warn(-1,"invalid android boot image");

	name=abootimg_get_name(img);
	cmdline=abootimg_get_cmdline(img);
	if(name&&*name)tlog_info("image name '%s'",name);

	LOAD(kernel,kernel)
	LOAD(ramdisk,initrd)

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

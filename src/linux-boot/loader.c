/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Protocol/BlockIo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include"str.h"
#include"list.h"
#include"aboot.h"
#include"logger.h"
#include"locate.h"
#include"internal.h"
#define TAG "loader"

bool linux_file_allocate(linux_file_info*fi,size_t size){
	fi->mem_pages=EFI_SIZE_TO_PAGES(ALIGN_VALUE(size,MEM_ALIGN)+fi->offset);
	fi->mem_size=EFI_PAGES_TO_SIZE(fi->mem_pages);
	if(!(fi->address=AllocateAlignedPages(fi->mem_pages,MEM_ALIGN))){
		tlog_warn("allocate pages for file failed");
		return false;
	}
	ZeroMem(fi->address,fi->mem_size);
	fi->address+=fi->offset;
	fi->allocated=true;
	return true;
}

static void load_done(linux_file_info*fi){
	char buff[64];
	tlog_info(
		"load done, %zu bytes (%s)",fi->size,
		make_readable_str_buf(buff,sizeof(buff),fi->size,1,0)
	);
	linux_file_dump(NULL,fi);
}

static int load_fp(linux_file_info*fi,EFI_FILE_PROTOCOL*fp){
	EFI_STATUS st;
	EFI_FILE_INFO*info=NULL;
	UINTN infos=0,read;

	// get file info
	st=fp->GetInfo(fp,&gEfiFileInfoGuid,&infos,info);
	if(st==EFI_BUFFER_TOO_SMALL){
		if(!(info=AllocateZeroPool(infos)))
			EDONE(tlog_error("allocate pool failed"));
		st=fp->GetInfo(fp,&gEfiFileInfoGuid,&infos,info);
	}
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get file info failed: %s",
		efi_status_to_string(st)
	));
	if(info->FileSize<=0)
		EDONE(tlog_warn("file size too small"));
	if(info->FileSize>=0x8000000)
		EDONE(tlog_warn("file size too big"));
	if(info->Attribute&EFI_FILE_DIRECTORY)
		EDONE(tlog_warn("file is a directory"));
	fi->size=info->FileSize;
	FreePool(info);
	info=NULL;

	// allocate memory for file
	if(!linux_file_allocate(fi,fi->size))goto done;

	// read file to memory
	read=fi->size;
	st=fp->Read(fp,&read,fi->address);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"read file failed: %s",
		efi_status_to_string(st)
	));
	if(read!=fi->size)EDONE(tlog_warn(
		"read file size not match %llu != %zu",
		(unsigned long long)read,fi->size
	));
	fp->Close(fp);

	load_done(fi);
	return 0;
	done:
	if(info)FreePool(info);
	if(fp)fp->Close(fp);
	linux_file_clean(fi);
	return -1;
}

static int load_bp(linux_file_info*fi,EFI_BLOCK_IO_PROTOCOL*bp){
	EFI_STATUS st;

	// calc block size
	fi->size=bp->Media->BlockSize*(bp->Media->LastBlock+1);
	if(fi->size>=0x8000000)
		EDONE(tlog_warn("block size too big"));

	// allocate memory for block
	if(!linux_file_allocate(fi,fi->size))goto done;

	// read block to memory
	st=bp->ReadBlocks(bp,
		bp->Media->MediaId,0,
		fi->size,fi->address
	);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"read block failed: %s",
		efi_status_to_string(st)
	));

	load_done(fi);
	return 0;
	done:
	linux_file_clean(fi);
	return -1;
}

static int load_pointer(linux_file_info*fi,void*p,size_t size){
	fi->size=size;
	if(fi->size>=0x8000000)
		EDONE(tlog_warn("pointer size too big"));

	if(!linux_file_allocate(fi,fi->size))goto done;

	CopyMem(fi->address,p,size);

	load_done(fi);
	return 0;
	done:
	linux_file_clean(fi);
	return -1;
}

int linux_file_load(linux_file_info*fi,linux_load_from*from){
	locate_ret loc;
	if(!fi||!from||!from->enabled)return -1;
	switch(from->type){
		case FROM_LOCATE:
			if(!boot_locate(&loc,from->locate)){
				tlog_warn("resolve locate failed");
				break;
			}
			switch(loc.type){
				case LOCATE_FILE:return load_fp(fi,loc.file);
				case LOCATE_BLOCK:return load_bp(fi,loc.block);
				default:tlog_warn("unsupported locate type");
			}
		break;
		case FROM_FILE_PROTOCOL:return load_fp(fi,from->file_proto);
		case FROM_BLOCKIO_PROTOCOL:return load_bp(fi,from->blk_proto);
		case FROM_POINTER:return load_pointer(fi,from->pointer,from->size);
		default:tlog_warn("unsupported from type");
	}
	return -1;
}

static int load_merged_initrd(linux_boot*lb){
	list*f;
	size_t off=0,cnt=0;
	char buff[64];
	linux_file_clean(&lb->initrd);
	if((f=list_first(lb->initrd_buf)))do{
		LIST_DATA_DECLARE(d,f,linux_file_info*);
		lb->initrd.size+=d->size,cnt++;
	}while((f=f->next));
	if(lb->initrd.size<=0)
		return trlog_warn(-1,"no any initramfs loaded, skip");
	tlog_info(
		"merge %zu initramfs %zu bytes (%s)",cnt,lb->initrd.size,
		make_readable_str_buf(buff,sizeof(buff),lb->initrd.size,1,0)
	);
	linux_file_allocate(&lb->initrd,lb->initrd.size);
	if(lb->initrd.address&&(f=list_first(lb->initrd_buf)))do{
		LIST_DATA_DECLARE(d,f,linux_file_info*);
		CopyMem(lb->initrd.address,d->address+off,d->size);
		off+=d->size;
	}while((f=f->next));
	list_free_all_def(lb->initrd_buf);
	lb->initrd_buf=NULL;
	return 0;
}

static void single_load(linux_load_from*from,linux_file_info*fi,const char*tag){
	if(!from->enabled)return;
	if(fi->address)linux_file_clean(fi);
	tlog_info("loading %s",tag);
	linux_file_load(fi,from);
}

static void multiple_load(list**from,list**fi,const char*tag){
	list*f;
	size_t cnt=0;
	if(!from)return;
	if((f=list_first(*from)))do{
		cnt++;
		LIST_DATA_DECLARE(d,f,linux_load_from*);
		if(!d->enabled)continue;
		linux_file_info*c=malloc(sizeof(linux_file_info));
		if(!c)continue;
		tlog_info("loading %s #%zu as #%d",tag,cnt,list_count(*from));
		linux_file_load(c,d);
		if(!c->address){
			tlog_warn("load failed");
			linux_file_clean(c);
			FreePool(c);
		}else list_obj_add_new(fi,c);
	}while((f=f->next));
}

int linux_load_from_config(linux_boot*lb){
	if(!boot||!lb)return -1;
	if(lb->config->use_uefi){
		tlog_info("use efistub boot method");
		lb->arch=ARCH_UEFI;
	}else if(lb->arch==ARCH_UNKNOWN){
		tlog_error("unsupported with non-efistub boot method");
		return -1;
	}
	if(lb->config->abootimg.enabled)
		linux_boot_load_abootimg_config(lb);
	switch(lb->arch){
		case ARCH_ARM32:lb->kernel.offset=LINUX_ARM32_OFFSET;break;
		case ARCH_ARM64:lb->kernel.offset=LINUX_ARM64_OFFSET;break;
		default:;
	}
	single_load(&lb->config->kernel,&lb->kernel,"kernel");
	single_load(&lb->config->dtb,&lb->dtb,"dtb");
	multiple_load(&lb->initrd_buf,&lb->config->initrd,"initrd");
	multiple_load(&lb->dtbo,&lb->config->dtbo,"dtbo");
	load_merged_initrd(lb);
	if(lb->config->cmdline[0])
		linux_boot_append_cmdline(lb,lb->config->cmdline);
	if(!lb->kernel.address){
		tlog_error("kernel not loaded, abort boot...");
		return -1;
	}
	tlog_notice("load done");
	return 0;
}

int linux_boot_init(linux_boot*boot){
	ZeroMem(boot,sizeof(linux_boot));
	boot->arch=ARCH_UNKNOWN;
	#if defined(__x86_64__)||defined(__amd64__)
	boot->arch=ARCH_X64;
	#elif defined(__i386__)
	boot->arch=ARCH_X86;
	#elif defined(__arm__)
	boot->arch=ARCH_ARM32;
	#elif defined(__aarch64__)
	boot->arch=ARCH_ARM64;
	#endif
	return 0;
}

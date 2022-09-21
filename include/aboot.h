/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _ABOOT_H
#define _ABOOT_H
#include<stdint.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include"filesystem.h"
#ifdef ENABLE_UEFI
#include<Protocol/SimpleFileSystem.h>
#include<Protocol/BlockIo.h>
#endif

// android boot image struct
typedef struct aboot_image aboot_image;

// src/lib/aboot.c: is an empty image without any parts
extern bool abootimg_is_empty(aboot_image*img);

// src/lib/aboot.c: is an invalid image has invalid header or no kernel
extern bool abootimg_is_invalid(aboot_image*img);

// src/lib/aboot.c: check page size is valid
extern bool abootimg_check_page(size_t p);

// src/lib/aboot.c: get image theoretical size (head + kernel size + ramdisk size + second size)
extern uint32_t abootimg_get_image_size(aboot_image*img);

// src/lib/aboot.c: allocate an empty image
extern aboot_image*abootimg_new_image();

// src/lib/aboot.c: deallocate image
extern void abootimg_free(aboot_image*img);

// src/lib/aboot.c: allocate and load image from a pointer
extern aboot_image*abootimg_load_from_memory(void*file,size_t len);

// src/lib/aboot.c: generate a full image for output
extern bool abootimg_generate(aboot_image*img,void**output,uint32_t*len);

// src/lib/aboot.c: allocate and load image from a fs handle
extern aboot_image*abootimg_load_from_fsh(fsh*f);

// src/lib/aboot.c: allocate and load image from an url struct
extern aboot_image*abootimg_load_from_url(url*u);

// src/lib/aboot.c: allocate and load image from an url path
extern aboot_image*abootimg_load_from_url_path(const char*path);

// src/lib/aboot.c: write image to a fs handle
extern bool abootimg_save_to_fsh(aboot_image*img,fsh*f);

// src/lib/aboot.c: write image to an url struct
extern bool abootimg_save_to_url(aboot_image*img,url*u);

// src/lib/aboot.c: write image to an url path
extern bool abootimg_save_to_url_path(aboot_image*img,const char*path);

#ifdef ENABLE_UEFI

// src/lib/aboot.c: allocate and load image from a block
extern aboot_image*abootimg_load_from_blockio(EFI_BLOCK_IO_PROTOCOL*bio);

// src/lib/aboot.c: allocate and load image from a file protocol
extern aboot_image*abootimg_load_from_fp(EFI_FILE_PROTOCOL*fp);

// src/lib/aboot.c: allocate and load image from a file with unicode path
extern aboot_image*abootimg_load_from_wfile(EFI_FILE_PROTOCOL*root,CHAR16*path);

// src/lib/aboot.c: allocate and load image from a file with ascii path
extern aboot_image*abootimg_load_from_file(EFI_FILE_PROTOCOL*root,char*path);

// src/lib/aboot.c: write image to a block
extern bool abootimg_save_to_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio);

// src/lib/aboot.c: write image to a file protocol
extern bool abootimg_save_to_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp);

// src/lib/aboot.c: write image to a file with unicode path
extern bool abootimg_save_to_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path);

// src/lib/aboot.c: write image to a file with ascii path
extern bool abootimg_save_to_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path);

// load and save kernel ramdisk second
#define DECL_ABOOTIMG_LOAD_SAVE(tag)\
	extern bool abootimg_load_##tag##_from_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio);\
	extern bool abootimg_load_##tag##_from_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp);\
	extern bool abootimg_load_##tag##_from_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path);\
	extern bool abootimg_load_##tag##_from_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path);\
	extern bool abootimg_save_##tag##_to_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio);\
	extern bool abootimg_save_##tag##_to_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp);\
	extern bool abootimg_save_##tag##_to_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path);\
	extern bool abootimg_save_##tag##_to_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path);
#else

// src/lib/aboot.c: allocate and load image from a file descriptor
extern aboot_image*abootimg_load_from_fd(int fd);

// src/lib/aboot.c: allocate and load image from a file
extern aboot_image*abootimg_load_from_file(int cfd,const char*file);

// src/lib/aboot.c: write image to a file descriptor
extern bool abootimg_save_to_fd(aboot_image*img,int fd);

// src/lib/aboot.c: write image to a file
extern bool abootimg_save_to_file(aboot_image*img,int cfd,const char*file);
#define DECL_ABOOTIMG_LOAD_SAVE(tag)\
	extern bool abootimg_save_##tag##_to_fd(aboot_image*img,int fd);\
	extern bool abootimg_save_##tag##_to_file(aboot_image*img,int cfd,const char*file);\
	extern bool abootimg_load_##tag##_from_fd(aboot_image*img,int fd);\
	extern bool abootimg_load_##tag##_from_file(aboot_image*img,int cfd,const char*file);
#endif

#define DECL_ABOOTIMG_FSH_LOAD_SAVE(tag)\
	extern bool abootimg_save_##tag##_to_fsh(aboot_image*img,fsh*f);\
	extern bool abootimg_save_##tag##_to_url(aboot_image*img,url*u);\
	extern bool abootimg_save_##tag##_to_url_path(aboot_image*img,const char*path);\
	extern bool abootimg_load_##tag##_from_fsh(aboot_image*img,fsh*f);\
	extern bool abootimg_load_##tag##_from_url(aboot_image*img,url*u);\
	extern bool abootimg_load_##tag##_from_url_path(aboot_image*img,const char*path);
#define DECL_ABOOTIMG_GET_SET(tag)\
	extern bool abootimg_copy_##tag(aboot_image*img,void*dest,size_t buf_len);\
	extern uint32_t abootimg_get_##tag(aboot_image*img,void**tag);\
	extern uint32_t abootimg_get_##tag##_end(aboot_image*img);\
	extern bool abootimg_set_##tag(aboot_image*img,void*tag,uint32_t len);\
        extern uint32_t abootimg_get_##tag##_offset(aboot_image*img);\
        extern bool abootimg_have_##tag(aboot_image*img);\
	DECL_ABOOTIMG_FSH_LOAD_SAVE(tag)\
        DECL_ABOOTIMG_LOAD_SAVE(tag)
#define DECL_ABOOTIMG_GET_VAR(type,key,def)extern type abootimg_get_##key(aboot_image*img);
#define DECL_ABOOTIMG_SET_VAR(type,key,def)extern void abootimg_set_##key(aboot_image*img,type key);
#define DECL_ABOOTIMG_GETSET_VAR(type,key,def) DECL_ABOOTIMG_GET_VAR(type,key,def) DECL_ABOOTIMG_SET_VAR(type,key,def)

DECL_ABOOTIMG_GET_SET(kernel)
DECL_ABOOTIMG_GET_SET(ramdisk)
DECL_ABOOTIMG_GET_SET(second)
DECL_ABOOTIMG_GETSET_VAR(const char*,name,NULL)
DECL_ABOOTIMG_GETSET_VAR(const char*,cmdline,NULL)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,kernel_size,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,kernel_address,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,ramdisk_size,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,ramdisk_address,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,second_size,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,second_address,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,tags_address,0)
DECL_ABOOTIMG_GETSET_VAR(uint32_t,page_size,0)

#undef DECL_ABOOTIMG_GETSET_VAR
#undef DECL_ABOOTIMG_GET_SET
#undef DECL_ABOOTIMG_GET_VAR
#undef DECL_ABOOTIMG_SET_VAR
#undef DECL_ABOOTIMG_LOAD_SAVE
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FSEXT_H
#define _FSEXT_H
#include"gui.h"
enum item_type{
	TYPE_DISK=1,
	TYPE_FILE,
	TYPE_DIR,
	TYPE_BLOCK,
	TYPE_CHAR,
	TYPE_SOCK,
	TYPE_LINK,
	TYPE_FIFO,
};
struct fsext{
	void*user_data;
	lv_res_t(*get_type_cb)(lv_fs_drv_t*,const char*,enum item_type*);
	lv_res_t(*get_volume_label)(lv_fs_drv_t*,char*,size_t);
	lv_res_t(*mkdir)(lv_fs_drv_t*,const char*);
	lv_res_t(*size)(lv_fs_drv_t*,void*,uint32_t*);
	lv_res_t(*rename)(lv_fs_drv_t*,const char*,const char*);
	lv_res_t(*remove)(lv_fs_drv_t*,const char*);
	lv_res_t(*creat)(lv_fs_drv_t*,const char*);
	bool(*is_dir_cb)(lv_fs_drv_t*,const char*);
};
extern bool fsext_is_multi;
extern lv_res_t lv_fs_get_type(enum item_type*type,const char*path);
extern lv_res_t lv_fs_get_volume_label(lv_fs_drv_t*drv,char*label,size_t len);
extern lv_res_t lv_fs_mkdir(const char*name);
extern lv_res_t lv_fs_creat(const char*name);
extern lv_fs_res_t lv_fs_size(lv_fs_file_t*file_p,uint32_t*size);
extern lv_fs_res_t lv_fs_rename(const char*oldname,const char*newname);
extern lv_fs_res_t lv_fs_remove(const char*path);
extern bool lv_fs_is_dir(const char*path);
extern const char*lv_fs_res_to_string(lv_fs_res_t res);
extern const char*lv_fs_res_to_i18n_string(lv_fs_res_t res);
#ifdef ENABLE_UEFI
#include<Protocol/SimpleFileSystem.h>
extern EFI_FILE_PROTOCOL*fs_get_root_by_letter(char letter);
extern EFI_HANDLE fs_get_root_handle_by_letter(char letter);
extern EFI_FILE_PROTOCOL*lv_fs_file_to_fp(lv_fs_file_t*fp);
extern EFI_DEVICE_PATH_PROTOCOL*fs_drv_get_device_path(char letter);
extern EFI_DEVICE_PATH_PROTOCOL*fs_get_device_path(const char*path);
#else
extern int lv_fs_file_to_fd(lv_fs_file_t*fp);
#endif
#endif

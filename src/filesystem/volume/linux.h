/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef FS_VOL_LINUX_H
#define FS_VOL_LINUX_H
#include"../fs_internal.h"
extern void fill_from_mount_item(fsvol_private_info*info,struct mount_item*mnt);
extern void fill_from_block_path(fsvol_private_info*info,char*block);
extern void fill_from_statfs(fsvol_private_info*info,struct statfs*st);
extern char*gen_id_from_mount_item(struct mount_item*item,char*buff,size_t len);
#endif

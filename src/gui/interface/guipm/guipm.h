/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_FDISK
#define _GNU_SOURCE
#ifndef GUIPM_H
#define GUIPM_H
#include<stdio.h>
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"gui.h"
#include"gui/activity.h"

struct disks_info;
struct disks_disk_info;
struct part_disk_info;
struct part_partition_info;

extern struct gui_register guireg_guipm_disk_select;
extern struct gui_register guireg_guipm_new_partition;
extern struct gui_register guireg_guipm_partitions;

extern void guipm_draw_title(lv_obj_t*screen);
extern void guipm_disk_operation_menu(struct fdisk_context*ctx);
extern void guipm_part_operation_menu(struct part_partition_info*pi);

struct disks_disk_info{
	bool enable;
	lv_obj_t*btn,*chk;
	struct fdisk_context*ctx;
	struct fdisk_label*lbl;
	long size;
	char name[256];
	char model[BUFSIZ];
	char path[BUFSIZ];
	char layout[16];
	int sysfs_fd;
	struct disks_info*di;
};

struct disks_info{
	bool is_show_all;
	lv_obj_t*lst,*selscr,*disks_info,*show_all;
	lv_obj_t*btn_ok,*btn_refresh,*btn_cancel;
	struct disks_disk_info disks[32],*selected;
};

struct part_disk_info{
	struct fdisk_context*ctx;
	struct fdisk_label*label;
	struct fdisk_table*table;
	fdisk_sector_t secs,lsec_size,psec_size;
	unsigned long size;
	char size_str[80],type[128],*target,*path;
	lv_obj_t*page,*disk_info,*btn_disk,*btn_part,*btn_save,*btn_reload,*btn_new;
	struct part_partition_info*partitions[1024],*selected;
};

struct part_partition_info{
	bool free;
	size_t no;
	char partname[32];
	char name[320];
	struct fdisk_partition*part;
	struct fdisk_parttype*type;
	fdisk_sector_t start_sec,end_sec,size_sec;
	unsigned long start,end,size;
	char start_str[64],end_str[64],size_str[64];
	char type_str[128];
	lv_obj_t*btn,*chk;
	struct part_disk_info*di;
};

struct part_new_size_block{
	struct part_new_info*par;
	lv_obj_t*unit;
	lv_obj_t*txt;
	lv_obj_t*txt_sec;
	fdisk_sector_t sec;
	fdisk_sector_t min_sec;
	fdisk_sector_t max_sec;
	bool txt_changed;
	bool unit_lock;
	bool txt_sec_changed;
	char buf_txt[64];
	char buf_txt_sec[128];
};

struct part_new_info{
	struct fdisk_context*ctx;
	struct part_partition_info*part;
	lv_obj_t*box,*ok,*cancel,*part_type,*part_num;
	struct part_new_size_block start;
	struct part_new_size_block end;
	struct part_new_size_block size;
};

#endif
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#ifndef GUIPM_H
#define GUIPM_H
#include<stdio.h>
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"gui.h"
#include"gui/msgbox.h"
#include"gui/activity.h"

struct size_block;
struct disks_info;
struct disks_disk_info;
struct part_disk_info;
struct part_partition_info;

extern struct gui_register guireg_guipm_disk_select;
extern struct gui_register guireg_guipm_new_partition;
extern struct gui_register guireg_guipm_partitions;
extern struct gui_register guireg_guipm_change_partition_type;
extern struct gui_register guireg_guipm_resize_partition;

extern void guipm_draw_title(lv_obj_t*screen);
extern bool guipm_save_label(struct fdisk_context*ctx);
extern void guipm_ask_save_label(struct fdisk_context*ctx,msgbox_callback cb,void*user_data);
extern void guipm_disk_operation_menu(struct fdisk_context*ctx);
extern void guipm_part_operation_menu(struct part_partition_info*pi);
extern void guipm_init_size_block(lv_obj_t*box,void*pi,fdisk_sector_t lsec,struct size_block*blk,char*title);

struct disks_disk_info{
	bool enable;
	lv_obj_t*btn;
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
	lv_obj_t*btn;
	struct part_disk_info*di;
};

struct size_block{
	void*par;
	lv_obj_t*unit;
	lv_obj_t*box;
	lv_obj_t*lbl;
	lv_obj_t*txt;
	lv_obj_t*txt_sec;
	lv_obj_t*slider;
	fdisk_sector_t sec;
	fdisk_sector_t min_sec;
	fdisk_sector_t max_sec;
	fdisk_sector_t lsec;
	void(*on_get_focus)(struct size_block*);
	void(*on_lost_focus)(struct size_block*);
	void(*on_change_value)(struct size_block*);
	void(*on_update_value)(struct size_block*);
	bool txt_changed;
	bool unit_lock;
	bool txt_sec_changed;
	char buf_txt[64];
	char buf_txt_sec[128];
};

#define SB_CALL(_blk,_ev)  (_blk).on_##_ev(&(_blk))
#define SB_PCALL(_blk,_ev) (_blk)->on_##_ev(_blk)
#define SB_SET(_blk,_min,_max,_val) \
	(_blk).min_sec=(_min),\
	(_blk).max_sec=(_max),\
	(_blk).sec=(_val),\
	SB_CALL((_blk),update_value)
#define SB_PSET(_blk,_min,_max,_val) \
	(_blk)->min_sec=(_min),\
	(_blk)->max_sec=(_max),\
	(_blk)->sec=(_val),\
	SB_PCALL((_blk),update_value)

struct part_new_info{
	struct gui_activity*act;
	struct fdisk_context*ctx;
	struct part_partition_info*part;
	lv_obj_t*box,*ok,*cancel,*part_type,*part_num,*bar;
	struct size_block start;
	struct size_block end;
	struct size_block size;
};

struct part_type_info{
	struct gui_activity*act;
	struct part_partition_info*part;
	lv_obj_t*box,*ok,*cancel,*part_type;
};

struct part_resize_info{
	struct gui_activity*act;
	struct part_partition_info*part;
	lv_obj_t*box,*ok,*cancel,*bar;
	struct size_block size;
};

#endif
#endif

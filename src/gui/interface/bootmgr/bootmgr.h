/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include"gui.h"
#include"str.h"
#include"boot.h"
#include"array.h"
#include"confd.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/activity.h"
#include"gui/msgbox.h"
#include"gui/inputbox.h"
#define TAG "bootmgr"

static __attribute__((unused)) const char*bootmgr_base="boot.configs";

struct bootmgr;
struct bootmgr_item;

struct bootmgr{
	lv_coord_t si;
	lv_label_long_mode_t lm;
	lv_obj_t*page,*title;
	lv_obj_t*btn_open,*btn_default,*btn_edit,*btn_delete;
	lv_obj_t*btn_create,*btn_reload,*btn_setting;
	list*items;
	char folder[256];
	struct bootmgr_item*selected;
};

struct bootmgr_item{
	struct bootmgr*bm;
	bool parent;
	boot_config cfg;
	lv_obj_t*btn,*w_img,*img,*title;
};

struct bootitem{
	struct gui_activity*act;
	lv_obj_t*title,*box,*ok,*cancel;
	lv_obj_t*lbl_name,*txt_name;
	lv_obj_t*lbl_icon,*txt_icon,*img_icon;
	lv_obj_t*lbl_desc,*txt_desc;
	lv_obj_t*lbl_mode,*sel_mode;
	lv_obj_t*lbl_parent,*sel_parent;
	lv_obj_t*chk_enable,*chk_show,*chk_save;
	lv_obj_t*btn_extra,*btn_conf;
	boot_config*folders[256];
	char name[256];
};

struct bootdata_efi{
	struct gui_activity*act;
	lv_obj_t*title,*box,*ok,*cancel;
	lv_obj_t*txt_path,*clr_path;
	lv_obj_t*txt_guid,*clr_guid;
	lv_obj_t*txt_options,*clr_options;
	lv_obj_t*chk_char16;
	char name[256];
};

struct bootdata_uefi_option{
	struct gui_activity*act;
	lv_obj_t*title,*box,*ok,*cancel;
	lv_obj_t*txt_opt,*sel_opt;
	char name[256];
	uint options[256];
};

struct bootdata_linux{
	struct gui_activity*act;
	lv_obj_t*title,*box,*ok,*cancel;
	lv_obj_t*txt_abootimg,*clr_abootimg;
	lv_obj_t*txt_kernel,*clr_kernel;
	struct bootdata_multi_item{
		lv_obj_t*box,*lbl,*sel;
		lv_obj_t*btn_edit,*lbl_edit;
		lv_obj_t*btn_del,*lbl_del;
		list*lst;
	}initrd,dtbo;
	lv_obj_t*txt_dtb,*clr_dtb;
	lv_obj_t*txt_cmdline,*clr_cmdline;
	lv_obj_t*lbl_splash,*btn_splash,*txt_splash;
	lv_obj_t*lbl_memory,*btn_memory,*txt_memory;
	lv_obj_t*chk_use_efi;
	lv_obj_t*chk_skip_dtb;
	lv_obj_t*chk_skip_dtbo;
	lv_obj_t*chk_skip_initrd;
	lv_obj_t*chk_skip_efi_memory_map;
	lv_obj_t*chk_skip_kernel_fdt_memory;
	lv_obj_t*chk_skip_kernel_fdt_cmdline;
	lv_obj_t*chk_match_kfdt_model;
	lv_obj_t*chk_ignore_dtbo_error;
	lv_obj_t*chk_update_splash;
	lv_obj_t*chk_load_custom_address;
	lv_obj_t*lbl_address_load,*btn_address_load,*txt_address_load;
	lv_obj_t*lbl_address_kernel,*btn_address_kernel,*txt_address_kernel;
	lv_obj_t*lbl_address_initrd,*btn_address_initrd,*txt_address_initrd;
	lv_obj_t*lbl_address_dtb,*btn_address_dtb,*txt_address_dtb;
	char name[256];
};

struct bootdata_memory_region{
	struct gui_activity*act;
	lv_obj_t*title,*box,*ok,*cancel;
	lv_obj_t*txt_reg_base;
	lv_obj_t*txt_reg_size;
	lv_obj_t*txt_reg_end;
	char path[PATH_MAX];
};

extern struct gui_register guireg_bootmgr;
extern struct gui_register guireg_bootitem;
extern struct gui_register guireg_bootdata_efi;
extern struct gui_register guireg_bootdata_linux;
extern struct gui_register guireg_bootdata_memreg;
extern struct gui_register guireg_bootdata_uefi_option;

#endif

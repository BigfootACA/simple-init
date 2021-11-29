/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_FDISK
#include<libfdisk/libfdisk.h>
#include"gui.h"
#include"guipm.h"
#include"logger.h"
#include"gui/msgbox.h"
#define TAG "guipm"

const char*guipm_units[]={"B","KB","MB","GB","TB","PB","EB","ZB","YB",NULL};

void guipm_draw_title(lv_obj_t*screen){
	lv_obj_t*title=lv_label_create(screen,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(title,gui_w,gui_h/16);
	lv_obj_set_y(title,16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("Partition Manager"));
}

bool guipm_save_label(struct fdisk_context*ctx){
	if(!ctx)return false;
	struct fdisk_label*lbl=fdisk_get_label(ctx,NULL);
	if(!lbl)return false;
	if(!fdisk_label_is_changed(lbl))return true;
	if((errno=fdisk_write_disklabel(ctx))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk save disk label failed");
		msgbox_alert("Save disk label failed: %m");
		return false;
	}
	tlog_debug("disk label saved");
	fdisk_label_set_changed(lbl,false);
	return true;
}

void guipm_ask_save_label(struct fdisk_context*ctx,msgbox_callback cb,void*user_data){
	if(!ctx)return;
	struct fdisk_label*lbl=fdisk_get_label(ctx,NULL);
	if(!lbl)return;
	if(!fdisk_label_is_changed(lbl))cb(0,NULL,user_data);
	else msgbox_set_user_data(msgbox_create_yesno(
		cb,"Partition table has been modified. "
		"Do you want to save it?"
	),user_data);
}

#endif
#endif

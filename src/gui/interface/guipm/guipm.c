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

#endif
#endif

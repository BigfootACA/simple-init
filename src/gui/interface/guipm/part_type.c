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
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"str.h"
#include"gui.h"
#include"guipm.h"
#include"logger.h"
#include"gui/activity.h"
#include"gui/tools.h"
#define TAG "guipm"

static void reload_info(struct part_type_info*pi){
	if(!pi)return;
	struct fdisk_parttype*pt=fdisk_partition_get_type(pi->part->part);
	lv_dropdown_clear_options(pi->part_type);
	lv_dropdown_add_option(pi->part_type,_("(none)"),0);
	struct fdisk_label*lbl=pi->part->di->label;
	for(size_t i=0;i<fdisk_label_get_nparttypes(lbl);i++){
		struct fdisk_parttype*t=fdisk_label_get_parttype(lbl,i);
		const char*name=fdisk_parttype_get_name(t);
		lv_dropdown_add_option(pi->part_type,_(name),i+1);
		if(pt==t)lv_dropdown_set_selected(pi->part_type,i+1);
	}
}

static int guipm_part_get_focus(struct gui_activity*d){
	struct part_type_info*pi=d->data;
	if(!pi)return 0;
	lv_group_add_obj(gui_grp,pi->part_type);
	lv_group_add_obj(gui_grp,pi->ok);
	lv_group_add_obj(gui_grp,pi->cancel);
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_type_info*pi=d->data;
	if(!pi)return 0;
	lv_group_remove_obj(pi->part_type);
	lv_group_remove_obj(pi->ok);
	lv_group_remove_obj(pi->cancel);
	return 0;
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_type_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->ok)return;
	uint16_t i=lv_dropdown_get_selected(pi->part_type);
	if(i==0){
		msgbox_alert("Invalid partition type");
		return;
	}
	struct part_partition_info*ppi=pi->part;
	struct part_disk_info*di=ppi->di;
	struct fdisk_partition*p=ppi->part;
	struct fdisk_label*lbl=di->label;
	struct fdisk_parttype*ot=fdisk_partition_get_type(p);
	struct fdisk_parttype*nt=fdisk_label_get_parttype(lbl,i-1);
	size_t pn=fdisk_partition_get_partno(p);
	if(ot!=nt){
		const char*on=fdisk_parttype_get_name(ot);
		const char*nn=fdisk_parttype_get_name(nt);
		tlog_debug("change partition %zu type from %s to %s",pn+1,on,nn);
		if((errno=fdisk_set_partition_type(di->ctx,pn,nt))!=0){
			if(errno<0)errno=-(errno);
			telog_error("fdisk set partition type failed");
			msgbox_alert("Set partition type failed: %m");
			return;
		}
	}
	guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_type_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->cancel)return;
	guiact_do_back();
}

static int guipm_draw_change_partition_type(struct gui_activity*act){
	struct part_partition_info*info=act->args;
	if(!info||info->free||!info->di)return -EINVAL;
	struct part_type_info*pi=malloc(sizeof(struct part_type_info));
	if(!pi)return -ENOMEM;
	memset(pi,0,sizeof(struct part_type_info));
	pi->part=info,act->data=pi;

	pi->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(pi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(pi->box,gui_sw/8*7);
	lv_obj_set_click(pi->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(pi->box);

	// Title
	lv_obj_t*title=lv_label_create(pi->box,NULL);
	lv_label_set_text(title,_("Change Partition Type"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	// Partition Type
	h+=(gui_font_size/2);
	pi->part_type=lv_dropdown_create(pi->box,NULL);
	lv_obj_set_event_cb(pi->part_type,lv_default_dropdown_cb);
	lv_obj_set_width(pi->part_type,w);
	lv_obj_set_y(pi->part_type,h);
	h+=lv_obj_get_height(pi->part_type);

	// OK Button
	h+=gui_font_size;
	pi->ok=lv_btn_create(pi->box,NULL);
	lv_style_set_action_button(pi->ok,true);
	lv_obj_set_size(pi->ok,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(pi->ok,NULL,LV_ALIGN_IN_TOP_LEFT,(gui_font_size/2),h);
	lv_obj_set_user_data(pi->ok,pi);
	lv_obj_set_event_cb(pi->ok,ok_cb);
	lv_label_set_text(lv_label_create(pi->ok,NULL),_("OK"));

	// Cancel Button
	pi->cancel=lv_btn_create(pi->box,NULL);
	lv_style_set_action_button(pi->cancel,true);
	lv_obj_set_size(pi->cancel,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(pi->cancel,NULL,LV_ALIGN_IN_TOP_RIGHT,-(gui_font_size/2),h);
	lv_obj_set_user_data(pi->cancel,pi);
	lv_obj_set_event_cb(pi->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(pi->cancel,NULL),_("Cancel"));
	h+=lv_obj_get_height(pi->cancel);

	h+=gui_font_size*3;
	lv_obj_set_height(pi->box,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(pi->box,NULL,LV_ALIGN_CENTER,0,0);

	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_change_partition_type={
	.name="guipm-change-partition-type",
	.title="Partition Manager",
	.icon="guipm.svg",
	.show_app=false,
	.open_file=false,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_change_partition_type,
	.back=true,
	.mask=true,
};
#endif
#endif

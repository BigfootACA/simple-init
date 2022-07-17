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

static void ok_cb(lv_event_t*e){
	struct part_type_info*pi=e->user_data;
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

static void cancel_cb(lv_event_t*e __attribute__((unused))){
	guiact_do_back();
}

static int guipm_part_init(struct gui_activity*act){
	struct part_type_info*pi;
	struct part_partition_info*info=act->args;
	if(!info||info->free||!info->di)return -EINVAL;
	if(!(pi=malloc(sizeof(struct part_type_info))))return -ENOMEM;
	memset(pi,0,sizeof(struct part_type_info));
	pi->part=info,pi->act=act,act->data=pi;
	return 0;
}

static int guipm_part_exit(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int guipm_draw_change_partition_type(struct gui_activity*act){
	struct part_type_info*pi=act->data;
	static lv_coord_t grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	},grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	};

	pi->box=lv_obj_create(act->page);
	lv_obj_set_style_pad_all(pi->box,gui_font_size,0);
	lv_obj_set_style_pad_row(pi->box,gui_font_size,0);
	lv_obj_set_style_max_width(pi->box,lv_pct(80),0);
	lv_obj_set_style_max_height(pi->box,lv_pct(80),0);
	lv_obj_set_style_min_width(pi->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(pi->box,gui_font_size*2,0);
	lv_obj_set_grid_dsc_array(pi->box,grid_col,grid_row);
	lv_obj_set_height(pi->box,LV_SIZE_CONTENT);
	lv_obj_center(pi->box);

	// Title
	lv_obj_t*title=lv_label_create(pi->box);
	lv_label_set_text(title,_("Change Partition Type"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_grid_cell(title,LV_GRID_ALIGN_CENTER,0,2,LV_GRID_ALIGN_CENTER,0,1);

	// Partition Type
	pi->part_type=lv_dropdown_create(pi->box);
	lv_obj_add_event_cb(pi->part_type,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_grid_cell(pi->part_type,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_CENTER,1,1);

	// OK Button
	pi->ok=lv_btn_create(pi->box);
	lv_obj_add_event_cb(pi->ok,ok_cb,LV_EVENT_CLICKED,pi);
	lv_obj_t*lbl_ok=lv_label_create(pi->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);
	lv_obj_set_grid_cell(pi->ok,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_CENTER,2,1);

	// Cancel Button
	pi->cancel=lv_btn_create(pi->box);
	lv_obj_add_event_cb(pi->cancel,cancel_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_t*lbl_cancel=lv_label_create(pi->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);
	lv_obj_set_grid_cell(pi->cancel,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,2,1);

	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_change_partition_type={
	.name="guipm-change-partition-type",
	.title="Partition Manager",
	.icon="guipm.svg",
	.show_app=false,
	.open_file=false,
	.init=guipm_part_init,
	.quiet_exit=guipm_part_exit,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_change_partition_type,
	.back=true,
	.mask=true,
};
#endif

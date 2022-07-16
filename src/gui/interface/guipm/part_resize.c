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
#include"gui.h"
#include"guipm.h"
#include"array.h"
#include"logger.h"
#include"gui/activity.h"
#define TAG "guipm"

static void reload_info(struct part_resize_info*pi){
	if(!pi)return;
	struct part_partition_info*p=pi->part;
	struct part_disk_info*d=p->di;
	fdisk_sector_t max=p->size_sec;
	size_t s=0,len=ARRLEN(d->partitions);
	while(d->partitions[s]!=p&&s<len)s++;
	for(size_t i=s+1;i<len;i++){
		struct part_partition_info*xp=d->partitions[i];
		if(!xp||!xp->free)break;
		max+=xp->size_sec;
	}
	SB_SET(pi->size,1,max,p->size_sec);
}

static int guipm_part_get_focus(struct gui_activity*d){
	struct part_resize_info*pi=d->data;
	if(!pi)return 0;
	pi->size.on_get_focus(&pi->size);
	lv_group_add_obj(gui_grp,pi->ok);
	lv_group_add_obj(gui_grp,pi->cancel);
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_resize_info*pi=d->data;
	if(!pi)return 0;
	pi->size.on_lost_focus(&pi->size);
	lv_group_remove_obj(pi->ok);
	lv_group_remove_obj(pi->cancel);
	return 0;
}

static void ok_cb(lv_event_t*e){
	struct part_resize_info*pi=e->user_data;
	if(pi->size.sec!=pi->part->size_sec){
		size_t pn=fdisk_partition_get_partno(pi->part->part);
		struct fdisk_partition*np=fdisk_new_partition();
		if(!np)return;
		if((errno=fdisk_partition_set_size(np,pi->size.sec))!=0){
			if(errno<0)errno=-(errno);
			telog_warn("set partition size failed");
			msgbox_alert("Set partition size failed: %m");
			fdisk_unref_partition(np);
			return;
		}
		if((errno=fdisk_set_partition(pi->part->di->ctx,pn,np))!=0){
			if(errno<0)errno=-(errno);
			telog_warn("change partition size failed");
			msgbox_alert("Change partition size failed: %m");
			fdisk_unref_partition(np);
			return;
		}
		tlog_debug(
			"change partition %zu size to %zu sectors",
			pn+1,pi->size.sec
		);
		fdisk_unref_partition(np);
		pi->act->data_changed=true;
	}
	guiact_do_back();
}

static void cancel_cb(lv_event_t*e __attribute__((unused))){
	guiact_do_back();
}

static int guipm_part_init(struct gui_activity*act){
	struct part_resize_info*pi;
	struct part_partition_info*info=act->args;
	if(!info||info->free||!info->di)return -EINVAL;
	if(!(pi=malloc(sizeof(struct part_resize_info))))return -ENOMEM;
	memset(pi,0,sizeof(struct part_resize_info));
	pi->part=info,act->data=pi,pi->act=act;
	return 0;
}

static int guipm_part_exit(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int guipm_draw_resize_partition(struct gui_activity*act){
	struct part_resize_info*pi=act->data;
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
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
	lv_label_set_text(title,_("Resize Partition"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_grid_cell(
		title,
		LV_GRID_ALIGN_CENTER,0,2,
		LV_GRID_ALIGN_CENTER,0,1
	);

	guipm_init_size_block(
		pi->box,pi,
		pi->part->di->lsec_size,
		&pi->size,
		"Size:"
	);
	lv_obj_set_grid_cell(
		pi->size.box,
		LV_GRID_ALIGN_STRETCH,0,2,
		LV_GRID_ALIGN_STRETCH,1,1
	);

	// OK Button
	pi->ok=lv_btn_create(pi->box);
	lv_obj_add_event_cb(pi->ok,ok_cb,LV_EVENT_CLICKED,pi);
	lv_obj_t*lbl_ok=lv_label_create(pi->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);
	lv_obj_set_grid_cell(
		pi->ok,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,2,1
	);

	// Cancel Button
	pi->cancel=lv_btn_create(pi->box);
	lv_obj_add_event_cb(pi->cancel,cancel_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_t*lbl_cancel=lv_label_create(pi->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);
	lv_obj_set_grid_cell(
		pi->cancel,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_STRETCH,2,1
	);

	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_resize_partition={
	.name="guipm-resize-partition",
	.title="Partition Manager",
	.icon="guipm.svg",
	.show_app=false,
	.open_file=false,
	.init=guipm_part_init,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_resize_partition,
	.quiet_exit=guipm_part_exit,
	.back=true,
	.mask=true,
};
#endif

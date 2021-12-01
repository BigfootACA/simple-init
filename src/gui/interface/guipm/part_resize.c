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
#include"gui.h"
#include"guipm.h"
#include"array.h"
#include"logger.h"
#include"gui/activity.h"
#include"gui/tools.h"
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
	lv_group_add_obj(gui_grp,pi->ok);
	lv_group_add_obj(gui_grp,pi->cancel);
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_resize_info*pi=d->data;
	if(!pi)return 0;
	lv_group_remove_obj(pi->ok);
	lv_group_remove_obj(pi->cancel);
	return 0;
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_resize_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->ok)return;
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
		tlog_debug("change partition %zu size to %zu sectors",pn+1,pi->size.sec);
		fdisk_unref_partition(np);
	}
	guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_resize_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->cancel)return;
	guiact_do_back();
}

static int guipm_draw_resize_partition(struct gui_activity*act){
	struct part_partition_info*info=act->args;
	if(!info||info->free||!info->di)return -EINVAL;
	struct part_resize_info*pi=malloc(sizeof(struct part_resize_info));
	if(!pi)return -ENOMEM;
	memset(pi,0,sizeof(struct part_resize_info));
	pi->part=info,act->data=pi;

	pi->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(pi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(pi->box,gui_sw/8*7);
	lv_obj_set_click(pi->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(pi->box);

	// Title
	lv_obj_t*title=lv_label_create(pi->box,NULL);
	lv_label_set_text(title,_("Resize Partition"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	guipm_init_size_block(&h,w,pi->box,pi,pi->part->di->lsec_size,&pi->size,"Size:");

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

struct gui_register guireg_guipm_resize_partition={
	.name="guipm-resize-partition",
	.title="Partition Manager",
	.icon="guipm.png",
	.show_app=false,
	.open_file=false,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_resize_partition,
	.back=true,
	.mask=true,
};
#endif
#endif

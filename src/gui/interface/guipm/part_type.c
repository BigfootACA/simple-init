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
		pi->act->data_changed=true;
	}
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
	pi->box=lv_draw_dialog_box(act->page,NULL,"Change Partition Type");
	pi->part_type=lv_dropdown_create(pi->box);
	lv_obj_set_width(pi->part_type,lv_pct(100));
	lv_obj_add_event_cb(pi->part_type,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_draw_btns_ok_cancel(pi->box,&pi->ok,&pi->cancel,ok_cb,pi);
	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_change_partition_type={
	.name="guipm-change-partition-type",
	.title="Partition Manager",
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

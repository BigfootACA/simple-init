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
#include"gui/tools.h"
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
	pi->box=lv_draw_dialog_box(act->page,NULL,"Resize Partition");
	guipm_init_size_block(
		pi->box,pi,
		pi->part->di->lsec_size,
		&pi->size,
		"Size:"
	);
	lv_draw_btns_ok_cancel(pi->box,&pi->ok,&pi->cancel,ok_cb,pi);
	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_resize_partition={
	.name="guipm-resize-partition",
	.title="Partition Manager",
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

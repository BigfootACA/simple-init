/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"str.h"
#include"gui.h"
#include"guipm.h"
#include"logger.h"
#include"gui/activity.h"
#include"gui/msgbox.h"
#include"gui/tools.h"
#define TAG "guipm"

static void update_bar(struct part_new_info*p){
	int16_t max=lv_slider_get_max_value(p->bar);
	fdisk_sector_t range=p->part->end_sec-p->part->start_sec;
	lv_bar_set_start_value(p->bar,max*((double)p->start.sec/range),LV_ANIM_ON);
	lv_bar_set_value(p->bar,max*((double)p->end.sec/range),LV_ANIM_ON);
}

static void update_data_secs(struct size_block*pi){
	struct part_new_info*p=pi->par;
	if(pi==&p->start){
		if(pi->sec>=p->end.sec){
			p->end.sec=pi->sec+1;
			SB_CALL(p->end,update_value);
		}
		p->size.sec=p->end.sec-p->start.sec+1;
		SB_CALL(p->size,update_value);
	}else if(pi==&p->end){
		if(pi->sec<=p->start.sec){
			p->start.sec=pi->sec-1;
			SB_CALL(p->start,update_value);
		}
		p->size.sec=p->end.sec-p->start.sec+1;
		SB_CALL(p->size,update_value);
	}else if(pi==&p->size){
		p->end.sec=p->start.sec+p->size.sec-1;
		if(p->end.sec>p->end.max_sec){
			p->end.sec=p->end.max_sec;
			p->start.sec=p->end.sec-pi->sec+1;
			SB_CALL(p->start,update_value);
		}
		SB_CALL(p->end,update_value);
	}
	update_bar(p);
}

static void reload_info(struct part_new_info*pi){
	if(!pi)return;
	struct part_partition_info*p=pi->part;
	SB_SET(pi->start,p->start_sec  ,p->end_sec-1,p->start_sec);
	SB_SET(pi->end  ,p->start_sec+1,p->end_sec  ,p->end_sec);
	SB_SET(pi->size ,1             ,p->size_sec ,p->size_sec);
	lv_dropdown_clear_options(pi->part_type);
	lv_dropdown_add_option(pi->part_type,_("(none)"),0);
	struct fdisk_label*lbl=pi->part->di->label;
	for(size_t i=0;i<fdisk_label_get_nparttypes(lbl);i++){
		const char*name=fdisk_parttype_get_name(fdisk_label_get_parttype(lbl,i));
		lv_dropdown_add_option(pi->part_type,_(name),i+1);
	}
	lv_dropdown_clear_options(pi->part_num);
	char buff[6];
	for(size_t i=0,z=0;i<255;i++){
		if(fdisk_is_partition_used(pi->ctx,i))continue;
		snprintf(buff,5,"%zu",i+1);
		lv_dropdown_add_option(pi->part_num,buff,z++);
	}
	update_bar(pi);
}

static int guipm_part_get_focus(struct gui_activity*d){
	struct part_new_info*pi=d->data;
	if(!pi)return 0;
	SB_CALL(pi->start,get_focus);
	SB_CALL(pi->end,get_focus);
	SB_CALL(pi->size,get_focus);
	lv_group_add_obj(gui_grp,pi->part_type);
	lv_group_add_obj(gui_grp,pi->part_num);
	lv_group_add_obj(gui_grp,pi->ok);
	lv_group_add_obj(gui_grp,pi->cancel);
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_new_info*pi=d->data;
	if(!pi)return 0;
	SB_CALL(pi->start,lost_focus);
	SB_CALL(pi->end,lost_focus);
	SB_CALL(pi->size,lost_focus);
	lv_group_remove_obj(pi->part_type);
	lv_group_remove_obj(pi->part_num);
	lv_group_remove_obj(pi->ok);
	lv_group_remove_obj(pi->cancel);
	return 0;
}

static void ok_cb(lv_event_t*e){
	struct part_new_info*pi=e->user_data;
	char*fs=NULL;
	size_t pn=0;
	struct fdisk_partition*fp;
	uint16_t type=lv_dropdown_get_selected(pi->part_type);
	if(type==0){
		msgbox_alert("Invalid partition type");
		return;
	}
	if(!(fp=fdisk_new_partition())){
		telog_error("fdisk allocate partition failed");
		msgbox_alert("Allocate partition failed");
		return;
	}
	tlog_debug("do new partition");
	if((errno=fdisk_partition_set_start(fp,pi->start.sec))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk set partition start failed");
		msgbox_alert("Set partition start failed: %m");
		goto end;
	}
	tlog_debug("partition start sector %lu",pi->start.sec);
	if((errno=fdisk_partition_set_size(fp,pi->size.sec))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk set partition size failed");
		msgbox_alert("Set partition size failed: %m");
		goto end;
	}
	tlog_debug("partition size sectors %lu",pi->size.sec);
	struct fdisk_parttype*p=fdisk_label_get_parttype(
		pi->part->di->label,type-1
	);
	if(!p){
		telog_error("fdisk lookup partition type failed");
		msgbox_alert("Lookup partition type failed");
		goto end;
	}
	if((errno=fdisk_partition_set_type(fp,p))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk set partition type failed");
		msgbox_alert("Set partition type failed: %m");
		goto end;
	}
	tlog_debug("partition type %s",fdisk_parttype_get_name(p));
	char str[8]={0};
	lv_dropdown_get_selected_str(pi->part_num,str,7);
	pn=parse_int(str,0);
	if(pn<=0||(errno=fdisk_partition_set_partno(fp,pn-1))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk set partition number failed");
		msgbox_alert("Set partition number failed: %m");
		goto end;
	}
	tlog_debug("partition number %zu",pn);
	if((errno=fdisk_add_partition(pi->ctx,fp,&pn))!=0){
		if(errno<0)errno=-(errno);
		telog_error("fdisk add partition failed");
		msgbox_alert("Add partition failed: %m");
		goto end;
	}
	tlog_debug("partition %zu created",pn+1);
	pi->act->data_changed=true;
	guiact_do_back();
	end:
	if(fp)fdisk_unref_partition(fp);
	if(fs)free(fs);
}

static void cancel_cb(lv_event_t*e __attribute__((unused))){
	guiact_do_back();
}

static int guipm_part_init(struct gui_activity*act){
	struct part_new_info*pi;
	struct part_partition_info*info=act->args;
	if(!info||!info->free||!info->di)return -EINVAL;
	if(!(pi=malloc(sizeof(struct part_new_info))))return -ENOMEM;
	memset(pi,0,sizeof(struct part_new_info));
	pi->ctx=info->di->ctx,act->data=pi;
	pi->part=info,pi->act=act;
	return 0;
}

static int guipm_part_exit(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int guipm_draw_new_partition(struct gui_activity*act){
	struct part_new_info*pi=act->data;

	pi->box=lv_obj_create(act->page);
	lv_obj_set_style_max_width(pi->box,lv_pct(80),0);
	lv_obj_set_style_max_height(pi->box,lv_pct(80),0);
	lv_obj_set_style_min_width(pi->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(pi->box,gui_dpi,0);
	lv_obj_set_height(pi->box,LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(pi->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_center(pi->box);

	// Title
	lv_obj_t*title=lv_label_create(pi->box);
	lv_obj_set_width(title,lv_pct(100));
	lv_label_set_text(title,_("New Partition"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);

	// Partition Bar
	pi->bar=lv_bar_create(pi->box);
	lv_obj_set_size(pi->bar,lv_pct(100),gui_font_size);
	lv_bar_set_mode(pi->bar,LV_BAR_MODE_RANGE);
	lv_bar_set_range(pi->bar,0,MIN(gui_sw/2,32767));
	lv_obj_set_grid_cell(pi->bar,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_set_style_bg_color(pi->bar,lv_palette_main(LV_PALETTE_GREY),0);

	guipm_init_size_block(pi->box,pi,pi->part->di->lsec_size,&pi->start,"Start:");
	guipm_init_size_block(pi->box,pi,pi->part->di->lsec_size,&pi->end,"End:");
	guipm_init_size_block(pi->box,pi,pi->part->di->lsec_size,&pi->size,"Size:");
	pi->start.on_change_value=update_data_secs;
	pi->end.on_change_value=update_data_secs;
	pi->size.on_change_value=update_data_secs;

	// Partition Type
	lv_obj_t*lbl_type=lv_label_create(pi->box);
	lv_label_set_text(lbl_type,_("Partition Type:"));

	pi->part_type=lv_dropdown_create(pi->box);
	lv_obj_add_event_cb(pi->part_type,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_width(pi->part_type,lv_pct(100));

	// Partition Number
	lv_obj_t*lbl_partno=lv_label_create(pi->box);
	lv_label_set_text(lbl_partno,_("Partition Number:"));

	pi->part_num=lv_dropdown_create(pi->box);
	lv_obj_add_event_cb(pi->part_num,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_width(pi->part_num,lv_pct(100));

	lv_obj_t*btns=lv_obj_create(pi->box);
	lv_obj_set_style_radius(btns,0,0);
	lv_obj_set_scroll_dir(btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(btns,0,0);
	lv_obj_set_style_bg_opa(btns,LV_OPA_0,0);
	lv_obj_set_style_pad_all(btns,gui_dpi/50,0);
	lv_obj_set_flex_flow(btns,LV_FLEX_FLOW_ROW);
	lv_obj_clear_flag(btns,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_row(btns,gui_font_size/2,0);
	lv_obj_set_style_pad_column(btns,gui_font_size/2,0);
	lv_obj_set_size(btns,lv_pct(100),LV_SIZE_CONTENT);

	// OK Button
	pi->ok=lv_btn_create(btns);
	lv_obj_add_event_cb(pi->ok,ok_cb,LV_EVENT_CLICKED,pi);
	lv_obj_t*lbl_ok=lv_label_create(pi->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);
	lv_obj_set_flex_grow(pi->ok,1);

	// Cancel Button
	pi->cancel=lv_btn_create(btns);
	lv_obj_add_event_cb(pi->cancel,cancel_cb,LV_EVENT_CLICKED,pi);
	lv_obj_t*lbl_cancel=lv_label_create(pi->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);
	lv_obj_set_flex_grow(pi->cancel,1);

	reload_info(pi);
	return 0;
}

struct gui_register guireg_guipm_new_partition={
	.name="guipm-new-partition",
	.title="Partition Manager",
	.icon="guipm.svg",
	.show_app=false,
	.open_file=false,
	.init=guipm_part_init,
	.quiet_exit=guipm_part_exit,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_new_partition,
	.back=true,
	.mask=true,
};
#endif

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
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/tools.h"
#define TAG "guipm"

static const char*units[]={"B","KB","MB","GB","TB","PB","EB","ZB","YB",NULL};

static void update_data_secs(fdisk_sector_t sec,struct part_new_size_block*pi,bool manual){
	int type=0;
	struct part_new_info*p=pi->par;
	if(sec<pi->min_sec&&pi->min_sec>0)sec=pi->min_sec;
	if(sec>pi->max_sec&&pi->max_sec>0)sec=pi->max_sec;
	int64_t cnt=sec*pi->par->part->di->lsec_size;
	if(!pi->unit_lock)while(cnt>=1024&&units[type])cnt/=1024,type++;
	else for(type=0;type<lv_dropdown_get_selected(pi->unit);type++)cnt/=1024;
	lv_dropdown_set_selected(pi->unit,type);
	snprintf(pi->buf_txt,63,"%ld",cnt);
	snprintf(pi->buf_txt_sec,127,"%ld",sec);
	lv_textarea_set_text(pi->txt,pi->buf_txt);
	lv_textarea_set_text(pi->txt_sec,pi->buf_txt_sec);
	pi->sec=sec;
	if(!manual)return;
	if(pi==&p->start){
		if(sec>=p->end.sec)update_data_secs(sec+1,&p->end,false);
		update_data_secs(p->end.sec-p->start.sec+1,&p->size,false);
	}else if(pi==&p->end){
		if(sec<=p->start.sec)update_data_secs(sec-1,&p->start,false);
		update_data_secs(p->end.sec-p->start.sec+1,&p->size,false);
	}else if(pi==&p->size){
		p->end.sec=p->start.sec+p->size.sec-1;
		if(p->end.sec>p->end.max_sec){
			p->end.sec=p->end.max_sec;
			p->start.sec=p->end.sec-sec+1;
			update_data_secs(p->start.sec,&p->start,false);
		}
		update_data_secs(p->end.sec,&p->end,false);
	}
}

static void update_data_size(unsigned long size,struct part_new_size_block*pi){
	for(int type=lv_dropdown_get_selected(pi->unit);type>0;type--)size*=1024;
	update_data_secs(size/pi->par->part->di->lsec_size,pi,true);
}

static void reload_info(struct part_new_info*pi){
	if(!pi)return;
	pi->start.min_sec=pi->part->start_sec;
	pi->start.max_sec=pi->part->end_sec-1;
	pi->end.min_sec=pi->part->start_sec+1;
	pi->end.max_sec=pi->part->end_sec;
	pi->size.min_sec=1;
	pi->size.max_sec=pi->part->size_sec;
	update_data_secs(pi->part->start_sec,&pi->start,true);
	update_data_secs(pi->part->end_sec,&pi->end,true);
	update_data_secs(pi->part->size_sec,&pi->size,true);
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
}

static void size_block_get_focus(struct part_new_size_block*blk){
	lv_group_add_obj(gui_grp,blk->txt);
	lv_group_add_obj(gui_grp,blk->unit);
	lv_group_add_obj(gui_grp,blk->txt_sec);
}

static void size_block_lost_focus(struct part_new_size_block*blk){
	lv_group_remove_obj(blk->txt);
	lv_group_remove_obj(blk->unit);
	lv_group_remove_obj(blk->txt_sec);
}

static int guipm_part_get_focus(struct gui_activity*d){
	struct part_new_info*pi=d->data;
	if(!pi)return 0;
	size_block_get_focus(&pi->start);
	size_block_get_focus(&pi->end);
	size_block_get_focus(&pi->size);
	lv_group_add_obj(gui_grp,pi->part_type);
	lv_group_add_obj(gui_grp,pi->part_num);
	lv_group_add_obj(gui_grp,pi->ok);
	lv_group_add_obj(gui_grp,pi->cancel);
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_new_info*pi=d->data;
	if(!pi)return 0;
	size_block_lost_focus(&pi->start);
	size_block_lost_focus(&pi->end);
	size_block_lost_focus(&pi->size);
	lv_group_remove_obj(pi->part_type);
	lv_group_remove_obj(pi->part_num);
	lv_group_remove_obj(pi->ok);
	lv_group_remove_obj(pi->cancel);
	return 0;
}

static void dropdown_add_units(lv_obj_t*dd){
	lv_dropdown_clear_options(dd);
	for(int i=0;units[i];i++)
		lv_dropdown_add_option(dd,units[i],i);
}

static void block_size_cb(lv_obj_t*obj,lv_event_t e){
	struct part_new_size_block*pi=lv_obj_get_user_data(obj);
	if(!pi||!pi->par||obj!=pi->txt)return;
	const char*value;
	char*end;
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
			sysbar_keyboard_open();
		break;
		case LV_EVENT_DEFOCUSED:
			if(!pi->txt_changed)break;
			errno=0;
			value=lv_textarea_get_text(obj);
			if(strcmp(pi->buf_txt,value)!=0){
				unsigned long l=(unsigned long)strtol(value,&end,10);
				if(*end||value==end||errno!=0)tlog_warn("invalid size number");
				else{
					update_data_size(l,pi);
					memset(pi->buf_txt,0,sizeof(pi->buf_txt));
					strcpy(pi->buf_txt,value);
				}
				lv_textarea_set_text(obj,pi->buf_txt);
			}
			pi->txt_changed=false;
		break;
		case LV_EVENT_VALUE_CHANGED:pi->txt_changed=true;break;
	}
}

static void block_unit_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct part_new_size_block*pi=lv_obj_get_user_data(obj);
	if(!pi||!pi->par||obj!=pi->unit)return;
	pi->unit_lock=true;
	int64_t cnt=pi->sec*pi->par->part->di->lsec_size;
	for(int type=0;type<lv_dropdown_get_selected(obj);type++)cnt/=1024;
	snprintf(pi->buf_txt,63,"%ld",cnt);
	lv_textarea_set_text(pi->txt,pi->buf_txt);
}

static void block_sector_cb(lv_obj_t*obj,lv_event_t e){
	struct part_new_size_block*pi=lv_obj_get_user_data(obj);
	if(!pi||!pi->par||obj!=pi->txt_sec)return;
	const char*value;
	char*end;
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
			sysbar_keyboard_open();
		break;
		case LV_EVENT_DEFOCUSED:
			if(!pi->txt_sec_changed)break;
			errno=0;
			value=lv_textarea_get_text(obj);
			if(strcmp(pi->buf_txt_sec,value)!=0){
				fdisk_sector_t l=(fdisk_sector_t)strtol(value,&end,10);
				if(*end||value==end||errno!=0)tlog_warn("invalid sector number");
				else{
					update_data_secs(l,pi,true);
					memset(pi->buf_txt_sec,0,sizeof(pi->buf_txt_sec));
					strcpy(pi->buf_txt_sec,value);
				}
				lv_textarea_set_text(obj,pi->buf_txt_sec);
			}
			pi->txt_sec_changed=false;
		break;
		case LV_EVENT_VALUE_CHANGED:pi->txt_sec_changed=true;break;
	}
}

static void init_size_block(
	lv_coord_t*h,
	lv_coord_t w,
	struct part_new_info*pi,
	struct part_new_size_block*blk,
	char*title
){
	if(!pi||!blk||!title)return;
	blk->par=pi;

	(*h)+=gui_font_size;
	lv_obj_t*lbl=lv_label_create(pi->box,NULL);
	lv_label_set_text(lbl,_(title));
	lv_obj_set_y(lbl,*h);

	blk->txt=lv_textarea_create(pi->box,NULL);
	lv_textarea_set_text(blk->txt,"0");
	lv_textarea_set_one_line(blk->txt,true);
	lv_textarea_set_cursor_hidden(blk->txt,true);
	lv_textarea_set_accepted_chars(blk->txt,NUMBER);
	lv_obj_set_user_data(blk->txt,blk);
	lv_obj_set_event_cb(blk->txt,block_size_cb);
	lv_obj_align(blk->txt,lbl,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(blk->txt,*h);
	lv_obj_align(lbl,blk->txt,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);

	blk->unit=lv_dropdown_create(pi->box,NULL);
	lv_obj_set_user_data(blk->unit,blk);
	lv_obj_set_event_cb(blk->unit,block_unit_cb);
	lv_obj_set_width(blk->unit,gui_font_size*4);
	lv_obj_set_width(blk->txt,w-lv_obj_get_width(blk->unit)-lv_obj_get_width(lbl)-gui_font_size);
	lv_obj_align(blk->unit,blk->txt,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	dropdown_add_units(blk->unit);
	(*h)+=lv_obj_get_height(blk->unit);

	(*h)+=(gui_font_size/2);
	blk->txt_sec=lv_textarea_create(pi->box,NULL);
	lv_textarea_set_text(blk->txt_sec,"0");
	lv_textarea_set_one_line(blk->txt_sec,true);
	lv_textarea_set_cursor_hidden(blk->txt_sec,true);
	lv_textarea_set_accepted_chars(blk->txt_sec,NUMBER);
	lv_obj_set_user_data(blk->txt_sec,blk);
	lv_obj_set_event_cb(blk->txt_sec,block_sector_cb);
	lv_obj_set_width(blk->txt_sec,lv_page_get_scrl_width(pi->box));
	lv_obj_set_y(blk->txt_sec,*h);
	(*h)+=lv_obj_get_height(blk->txt_sec);
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_new_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->ok)return;
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
	struct fdisk_parttype*p=fdisk_label_get_parttype(pi->part->di->label,type-1);
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
	guiact_do_back();
	end:
	if(fp)fdisk_unref_partition(fp);
	if(fs)free(fs);
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_new_info*pi=lv_obj_get_user_data(obj);
	if(!pi||obj!=pi->cancel)return;
	guiact_do_back();
}

static int guipm_draw_new_partition(struct gui_activity*act){
	struct part_partition_info*info=act->args;
	if(!info||!info->free||!info->di)return -EINVAL;
	struct part_new_info*pi=malloc(sizeof(struct part_new_info));
	if(!pi)return -ENOMEM;
	memset(pi,0,sizeof(struct part_new_info));
	pi->part=info,pi->ctx=info->di->ctx,act->data=pi;

	pi->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(pi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(pi->box,gui_sw/8*7);
	lv_obj_set_click(pi->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(pi->box);

	// Title
	lv_obj_t*title=lv_label_create(pi->box,NULL);
	lv_label_set_text(title,_("New Partition"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	init_size_block(&h,w,pi,&pi->start,"Start:");
	init_size_block(&h,w,pi,&pi->end,"End:");
	init_size_block(&h,w,pi,&pi->size,"Size:");

	// Partition Type
	h+=gui_font_size;
	lv_obj_t*lbl_type=lv_label_create(pi->box,NULL);
	lv_label_set_text(lbl_type,_("Partition Type:"));
	lv_obj_set_y(lbl_type,h);
	h+=lv_obj_get_height(lbl_type);

	h+=(gui_font_size/2);
	pi->part_type=lv_dropdown_create(pi->box,NULL);
	lv_obj_set_width(pi->part_type,w);
	lv_obj_set_y(pi->part_type,h);
	h+=lv_obj_get_height(pi->part_type);

	// Partition Number
	h+=gui_font_size;
	lv_obj_t*lbl_partno=lv_label_create(pi->box,NULL);
	lv_label_set_text(lbl_partno,_("Partition Number:"));
	lv_obj_set_y(lbl_partno,h);

	pi->part_num=lv_dropdown_create(pi->box,NULL);
	lv_obj_align(pi->part_num,lbl_partno,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(pi->part_num,h);
	lv_obj_align(lbl_partno,pi->part_num,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);
	lv_obj_set_width(pi->part_num,w-lv_obj_get_width(lbl_partno)-gui_font_size/2);
	h+=lv_obj_get_height(pi->part_num);

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
struct gui_register guireg_guipm_new_partition={
	.name="guipm-new-partition",
	.title="Partition Manager",
	.icon="guipm.png",
	.show_app=false,
	.open_file=false,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_new_partition,
	.back=true,
	.mask=true,
};
#endif
#endif

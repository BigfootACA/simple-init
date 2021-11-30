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
#include"str.h"
#include"gui.h"
#include"guipm.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
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

static void size_block_get_focus(struct size_block*blk){
	lv_group_add_obj(gui_grp,blk->txt);
	lv_group_add_obj(gui_grp,blk->unit);
	lv_group_add_obj(gui_grp,blk->txt_sec);
}

static void size_block_lost_focus(struct size_block*blk){
	lv_group_remove_obj(blk->txt);
	lv_group_remove_obj(blk->unit);
	lv_group_remove_obj(blk->txt_sec);
}

static void block_unit_cb(lv_obj_t*obj,lv_event_t e){
	lv_default_dropdown_cb(obj,e);
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct size_block*pi=lv_obj_get_user_data(obj);
	if(!pi||!pi->par||obj!=pi->unit)return;
	pi->unit_lock=true;
	int64_t cnt=pi->sec*pi->lsec;
	for(int type=0;type<lv_dropdown_get_selected(obj);type++)cnt/=1024;
	snprintf(pi->buf_txt,63,"%ld",cnt);
	lv_textarea_set_text(pi->txt,pi->buf_txt);
}

static void block_size_cb(lv_obj_t*obj,lv_event_t e){
	struct size_block*pi=lv_obj_get_user_data(obj);
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
					if(pi->on_size_changed)pi->on_size_changed(l,pi);
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

static void block_sector_cb(lv_obj_t*obj,lv_event_t e){
	struct size_block*pi=lv_obj_get_user_data(obj);
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
					if(pi->on_sector_changed)pi->on_sector_changed(l,pi);
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

void guipm_init_size_block(
	lv_coord_t*h,
	lv_coord_t w,
	lv_obj_t*box,
	void*pi,
	fdisk_sector_t lsec,
	struct size_block*blk,
	char*title
){
	if(!pi||!blk||!title)return;
	blk->par=pi,blk->lsec=lsec;
	blk->on_get_focus=size_block_get_focus;
	blk->on_lost_focus=size_block_lost_focus;

	(*h)+=gui_font_size;
	lv_obj_t*lbl=lv_label_create(box,NULL);
	lv_label_set_text(lbl,_(title));
	lv_obj_set_y(lbl,*h);

	blk->txt=lv_textarea_create(box,NULL);
	lv_textarea_set_text(blk->txt,"0");
	lv_textarea_set_one_line(blk->txt,true);
	lv_textarea_set_cursor_hidden(blk->txt,true);
	lv_textarea_set_accepted_chars(blk->txt,NUMBER);
	lv_obj_set_user_data(blk->txt,blk);
	lv_obj_set_event_cb(blk->txt,block_size_cb);
	lv_obj_align(blk->txt,lbl,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(blk->txt,*h);
	lv_obj_align(lbl,blk->txt,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);

	blk->unit=lv_dropdown_create(box,NULL);
	lv_obj_set_user_data(blk->unit,blk);
	lv_obj_set_event_cb(blk->unit,block_unit_cb);
	lv_obj_set_width(blk->unit,gui_font_size*4);
	lv_obj_set_width(blk->txt,w-lv_obj_get_width(blk->unit)-lv_obj_get_width(lbl)-gui_font_size);
	lv_obj_align(blk->unit,blk->txt,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_dropdown_clear_options(blk->unit);
	for(int i=0;guipm_units[i];i++)
		lv_dropdown_add_option(blk->unit,guipm_units[i],i);

	(*h)+=lv_obj_get_height(blk->unit);

	(*h)+=(gui_font_size/2);
	blk->txt_sec=lv_textarea_create(box,NULL);
	lv_textarea_set_text(blk->txt_sec,"0");
	lv_textarea_set_one_line(blk->txt_sec,true);
	lv_textarea_set_cursor_hidden(blk->txt_sec,true);
	lv_textarea_set_accepted_chars(blk->txt_sec,NUMBER);
	lv_obj_set_user_data(blk->txt_sec,blk);
	lv_obj_set_event_cb(blk->txt_sec,block_sector_cb);
	lv_obj_set_width(blk->txt_sec,lv_page_get_scrl_width(box));
	lv_obj_set_y(blk->txt_sec,*h);
	(*h)+=lv_obj_get_height(blk->txt_sec);
}


#endif
#endif

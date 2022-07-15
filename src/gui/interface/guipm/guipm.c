/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<libfdisk/libfdisk.h>
#include"str.h"
#include"gui.h"
#include"guipm.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#define TAG "guipm"

void guipm_draw_title(lv_obj_t*screen){
	lv_obj_t*title=lv_label_create(screen);
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(title,lv_pct(100));
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("Partition Manager"));
}

bool guipm_save_label(struct fdisk_context*ctx){
	if(!ctx)return false;
	struct fdisk_label*lbl=fdisk_get_label(ctx,NULL);
	if(!lbl||!fdisk_label_is_changed(lbl))return true;
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

void guipm_ask_save_label(
	struct fdisk_context*ctx,
	msgbox_callback cb,
	void*user_data
){
	struct fdisk_label*lbl=NULL;
	if(ctx)lbl=fdisk_get_label(ctx,NULL);
	if(!lbl||!fdisk_label_is_changed(lbl))cb(0,NULL,user_data);
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

static void block_size_update(struct size_block*blk){
	uint16_t type=0;
	int64_t cnt=blk->sec*blk->lsec;
	if(blk->unit_lock){
		uint16_t units=lv_dropdown_get_selected(blk->unit);
		for(type=0;type<units;type++)cnt/=1024;
	}else{
		while(cnt>=1024&&size_units[type])cnt/=1024,type++;
		lv_dropdown_set_selected(blk->unit,type);
	}
	snprintf(blk->buf_txt,sizeof(blk->buf_txt)-1,"%ld",cnt);
	lv_textarea_set_text(blk->txt,blk->buf_txt);
}

static void block_update(struct size_block*blk){
	if(blk->sec<blk->min_sec&&blk->min_sec>0)blk->sec=blk->min_sec;
	if(blk->sec>blk->max_sec&&blk->max_sec>0)blk->sec=blk->max_sec;
	snprintf(blk->buf_txt_sec,sizeof(blk->buf_txt_sec)-1,"%ld",blk->sec);
	lv_textarea_set_text(blk->txt_sec,blk->buf_txt_sec);
	block_size_update(blk);
	if(blk->max_sec>0){
		double k=(double)(blk->sec-blk->min_sec)/(double)(blk->max_sec-blk->min_sec);
		int16_t max=lv_slider_get_max_value(blk->slider);
		int16_t min=lv_slider_get_min_value(blk->slider);
		int16_t v=(max-min)*k+min;
		if(lv_slider_get_value(blk->slider)!=v)
			lv_slider_set_value(blk->slider,v,LV_ANIM_ON);
	}
}

static void block_unit_cb(lv_event_t*e){
	struct size_block*pi=lv_event_get_user_data(e);
	pi->unit_lock=true;
	block_size_update(pi);
}

static void block_size_defocus_cb(lv_event_t*e){
	errno=0;
	char*end;
	struct size_block*pi=e->user_data;
	if(!pi->txt_changed)return;
	const char*value=lv_textarea_get_text(pi->txt);
	if(strcmp(pi->buf_txt,value)!=0){
		unsigned long l=(unsigned long)strtol(value,&end,10);
		if(*end||value==end||errno!=0)tlog_warn("invalid size number");
		else{
			memset(pi->buf_txt,0,sizeof(pi->buf_txt));
			strcpy(pi->buf_txt,value);
			unsigned long size=l;
			int type=lv_dropdown_get_selected(pi->unit);
			for(;type>0;type--)size*=1024;
			pi->sec=size/pi->lsec;
			SB_PCALL(pi,update_value);
			if(pi->on_change_value)pi->on_change_value(pi);
		}
		lv_textarea_set_text(pi->txt,pi->buf_txt);
	}
	pi->txt_changed=false;
}

static void block_sector_defocus_cb(lv_event_t*e){
	errno=0;
	char*end;
	struct size_block*pi=e->user_data;
	if(!pi->txt_sec_changed)return;
	const char*value=lv_textarea_get_text(pi->txt_sec);
	if(strcmp(pi->buf_txt_sec,value)!=0){
		fdisk_sector_t l=(fdisk_sector_t)strtol(value,&end,10);
		if(*end||value==end||errno!=0)tlog_warn("invalid sector number");
		else{
			memset(pi->buf_txt_sec,0,sizeof(pi->buf_txt_sec));
			strcpy(pi->buf_txt_sec,value);
			pi->sec=l;
			SB_PCALL(pi,update_value);
			if(pi->on_change_value)pi->on_change_value(pi);
		}
		lv_textarea_set_text(pi->txt_sec,pi->buf_txt_sec);
	}
	pi->txt_sec_changed=false;
}

static void block_size_value_changed(lv_event_t*e){
	struct size_block*pi=e->user_data;
	pi->txt_changed=true;
}

static void block_sector_value_changed(lv_event_t*e){
	struct size_block*pi=e->user_data;
	pi->txt_sec_changed=true;
}

static void block_slider_cb(lv_event_t*e){
	struct size_block*pi=e->user_data;
	if(pi->max_sec<=0||lv_slider_is_dragged(pi->slider))return;
	int16_t max=lv_slider_get_max_value(pi->slider);
	int16_t min=lv_slider_get_min_value(pi->slider);
	double k=(double)(lv_slider_get_value(pi->slider)-min)/(double)(max-min);
	fdisk_sector_t sec=(pi->max_sec-pi->min_sec)*k+pi->min_sec;
	if(sec==pi->sec)return;
	if(sec<pi->min_sec)sec=pi->min_sec;
	if(sec>pi->max_sec)sec=pi->max_sec;
	pi->sec=sec;
	SB_PCALL(pi,update_value);
	if(pi->on_change_value)pi->on_change_value(pi);
}

void guipm_init_size_block(
	lv_obj_t*box,
	void*pi,
	fdisk_sector_t lsec,
	struct size_block*blk,
	char*title
){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),0,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	if(grid_col[2]==0)grid_col[2]=gui_font_size*5;
	if(!pi||!blk||!title)return;
	blk->par=pi,blk->lsec=lsec;
	blk->on_get_focus=size_block_get_focus;
	blk->on_lost_focus=size_block_lost_focus;
	blk->on_update_value=block_update;

	blk->box=lv_obj_create(box);
	lv_obj_set_style_radius(blk->box,0,0);
	lv_obj_set_scroll_dir(blk->box,LV_DIR_NONE);
	lv_obj_set_style_border_width(blk->box,0,0);
	lv_obj_set_style_bg_opa(blk->box,LV_OPA_0,0);
	lv_obj_set_style_pad_all(blk->box,gui_font_size/2,0);
	lv_obj_set_grid_dsc_array(blk->box,grid_col,grid_row);
	lv_obj_clear_flag(blk->box,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_size(blk->box,lv_pct(100),LV_SIZE_CONTENT);

	blk->lbl=lv_label_create(blk->box);
	lv_label_set_text(blk->lbl,_(title));
	lv_obj_set_grid_cell(
		blk->lbl,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	blk->txt=lv_textarea_create(blk->box);
	lv_textarea_set_text(blk->txt,"0");
	lv_textarea_set_one_line(blk->txt,true);
	lv_textarea_set_accepted_chars(blk->txt,NUMBER);
	lv_obj_add_event_cb(blk->txt,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_add_event_cb(blk->txt,block_size_defocus_cb,LV_EVENT_DEFOCUSED,blk);
	lv_obj_add_event_cb(blk->txt,block_size_value_changed,LV_EVENT_VALUE_CHANGED,blk);
	lv_obj_set_grid_cell(
		blk->txt,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);

	blk->unit=lv_dropdown_create(blk->box);
	lv_obj_add_event_cb(blk->unit,block_unit_cb,LV_EVENT_VALUE_CHANGED,blk);
	lv_obj_add_event_cb(blk->unit,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_grid_cell(
		blk->unit,
		LV_GRID_ALIGN_STRETCH,2,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	lv_dropdown_clear_options(blk->unit);
	for(int i=0;size_units[i];i++)
		lv_dropdown_add_option(blk->unit,size_units[i],i);

	blk->txt_sec=lv_textarea_create(blk->box);
	lv_textarea_set_text(blk->txt_sec,"0");
	lv_textarea_set_one_line(blk->txt_sec,true);
	lv_textarea_set_accepted_chars(blk->txt_sec,NUMBER);
	lv_obj_add_event_cb(blk->txt_sec,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_add_event_cb(blk->txt_sec,block_sector_defocus_cb,LV_EVENT_DEFOCUSED,blk);
	lv_obj_add_event_cb(blk->txt_sec,block_sector_value_changed,LV_EVENT_VALUE_CHANGED,blk);
	lv_obj_set_grid_cell(
		blk->txt_sec,
		LV_GRID_ALIGN_STRETCH,0,3,
		LV_GRID_ALIGN_STRETCH,1,1
	);

	blk->slider=lv_slider_create(blk->box);
	lv_slider_set_range(blk->slider,0,MIN(gui_sw/2,32767));
	lv_obj_add_event_cb(blk->slider,block_slider_cb,LV_EVENT_RELEASED,blk);
	lv_obj_set_grid_cell(
		blk->slider,
		LV_GRID_ALIGN_STRETCH,0,3,
		LV_GRID_ALIGN_STRETCH,2,1
	);
}

#endif

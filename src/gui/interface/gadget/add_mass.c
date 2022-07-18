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
#include<sys/stat.h>
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"

static int mass_id=0;
struct gadget_add_mass{
	lv_obj_t*box,*ok,*cancel,*btn_sel;
	lv_obj_t*title,*lbl_path,*lbl_id;
	lv_obj_t*txt_id,*txt_path;
	lv_obj_t*ro,*removable,*cdrom;
	bool check_changed;
};

static int add_mass_get_focus(struct gui_activity*d){
	struct gadget_add_mass*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->txt_path);
	lv_group_add_obj(gui_grp,am->btn_sel);
	lv_group_add_obj(gui_grp,am->txt_id);
	lv_group_add_obj(gui_grp,am->cdrom);
	lv_group_add_obj(gui_grp,am->removable);
	lv_group_add_obj(gui_grp,am->ro);
	lv_group_add_obj(gui_grp,am->ok);
	lv_group_add_obj(gui_grp,am->cancel);
	return 0;
}

static int add_mass_lost_focus(struct gui_activity*d){
	struct gadget_add_mass*am=d->data;
	if(!am)return 0;
	lv_group_remove_obj(am->txt_path);
	lv_group_remove_obj(am->btn_sel);
	lv_group_remove_obj(am->txt_id);
	lv_group_remove_obj(am->cdrom);
	lv_group_remove_obj(am->removable);
	lv_group_remove_obj(am->ro);
	lv_group_remove_obj(am->ok);
	lv_group_remove_obj(am->cancel);
	return 0;
}

static void set_disk_file(struct gadget_add_mass*am,const char*path){
	lv_textarea_set_text(am->txt_path,path);
	if(am->check_changed)return;
	char*ext=strrchr(path,'.');
	if(!ext)return;
	ext++;
	bool is_cdrom=strcasecmp(ext,"iso")==0;
	lv_obj_set_checked(am->ro,is_cdrom);
	lv_obj_set_checked(am->cdrom,is_cdrom);
}

static bool select_cb(
	bool ok,
	const char**path,
	uint16_t cnt,
	void*user_data
){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	struct gadget_add_mass*am=user_data;
	set_disk_file(am,path[0]+2);
	return false;
}

static void sel_cb(lv_event_t*e){
	struct gadget_add_mass*am=e->user_data;
	struct filepicker*fp=filepicker_create(
		select_cb,
		"Open disk file"
	);
	filepicker_set_user_data(fp,am);
	filepicker_set_max_item(fp,1);
}

extern void gadget_restart_service();
static bool restart_cb(
	uint16_t id,
	const char*text __attribute__((unused)),
	void*user_data __attribute__((unused))
){
	if(id==0)gadget_restart_service();
	return false;
}

static void ok_cb(lv_event_t*e){
	struct gadget_add_mass*am=e->user_data;
	static char*base="gadget.func";
	struct stat st;
	int cnt=confd_count(base);
	if(cnt<0)cnt=0;
	const char*path=lv_textarea_get_text(am->txt_path);
	if(path[0]!='/'){
		msgbox_alert("Disk file path is not absolute");
		return;
	}
	if(stat(path,&st)!=0){
		msgbox_alert("stat disk file failed: %m");
		return;
	}
	if(!S_ISBLK(st.st_mode)&&!(S_ISREG(st.st_mode)&&st.st_size!=0)){
		msgbox_alert("Invalid disk file");
		return;
	}
	confd_set_string_array(
		base,cnt,"mode",
		"mass"
	);
	confd_set_string_array(
		base,cnt,"func",
		"mass_storage"
	);
	confd_set_string_array(
		base,cnt,"name",
		(char*)lv_textarea_get_text(am->txt_id)
	);
	confd_set_string_array(
		base,cnt,"path",
		(char*)path
	);
	confd_set_boolean_array(
		base,cnt,"ro",
		lv_obj_is_checked(am->ro)
	);
	confd_set_boolean_array(
		base,cnt,"cdrom",
		lv_obj_is_checked(am->cdrom)
	);
	confd_set_boolean_array(
		base,cnt,"removable",
		lv_obj_is_checked(am->removable)
	);
	mass_id++;
	guiact_do_back();
	if(!guiact_has_activity_name("usb-gadget"))msgbox_create_yesno(
		restart_cb,
		"The operation will be applied after restarting USB Gadget service, "
		"do you want to restart USB Gadget service?"
	);
}

static void cancel_cb(lv_event_t*e __attribute__((unused))){
	guiact_do_back();
}

static int add_mass_init(struct gui_activity*act){
	struct gadget_add_mass*am=malloc(sizeof(struct gadget_add_mass));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct gadget_add_mass));
	act->data=am;
	char*p=act->args;
	if(p&&p[0]!='/'){
		if(p[1]!=':')return -EINVAL;
		act->args+=2;
	}
	return 0;
}

static int add_mass_cleanup(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static void chk_cb(lv_event_t*e){
	struct gadget_add_mass*am=e->user_data;
	if(am)am->check_changed=true;
}

static int draw_add_mass(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	struct gadget_add_mass*am=act->data;

	am->box=lv_obj_create(act->page);
	lv_obj_set_style_max_width(am->box,lv_pct(80),0);
	lv_obj_set_style_max_height(am->box,lv_pct(80),0);
	lv_obj_set_style_min_width(am->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(am->box,gui_dpi,0);
	lv_obj_set_height(am->box,LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(am->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_center(am->box);

	// Title
	am->title=lv_label_create(am->box);
	lv_label_set_text(am->title,_("Add Mass Storage into USB Gadget"));
	lv_label_set_long_mode(am->title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(am->title,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(am->title,lv_pct(100));

	lv_obj_t*fields=lv_obj_create(am->box);
	lv_obj_set_style_radius(fields,0,0);
	lv_obj_set_scroll_dir(fields,LV_DIR_NONE);
	lv_obj_set_style_border_width(fields,0,0);
	lv_obj_set_style_bg_opa(fields,LV_OPA_0,0);
	lv_obj_set_style_pad_all(fields,gui_dpi/50,0);
	lv_obj_set_grid_dsc_array(fields,grid_col,grid_row);
	lv_obj_clear_flag(fields,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_row(fields,gui_font_size/2,0);
	lv_obj_set_style_pad_column(fields,gui_font_size/2,0);
	lv_obj_set_size(fields,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_center(fields);

	// File select
	am->lbl_path=lv_label_create(fields);
	lv_label_set_text(am->lbl_path,_("Path:"));
	lv_obj_set_grid_cell(
		am->lbl_path,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	am->txt_path=lv_textarea_create(fields);
	lv_textarea_set_text(am->txt_path,"");
	lv_textarea_set_one_line(am->txt_path,true);
	lv_obj_set_user_data(am->txt_path,am);
	lv_obj_add_event_cb(am->txt_path,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(
		am->txt_path,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);

	am->btn_sel=lv_btn_create(fields);
	lv_obj_set_enabled(am->btn_sel,true);
	lv_obj_add_event_cb(am->btn_sel,sel_cb,LV_EVENT_CLICKED,am);
	lv_obj_t*lbl_sel=lv_label_create(am->btn_sel);
	lv_label_set_text(lbl_sel,"...");
	lv_obj_center(lbl_sel);
	lv_obj_set_grid_cell(
		am->btn_sel,
		LV_GRID_ALIGN_STRETCH,2,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);

	// Function Name
	am->lbl_id=lv_label_create(fields);
	lv_label_set_text(am->lbl_id,_("Name:"));
	lv_obj_set_grid_cell(
		am->lbl_id,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,1,1
	);

	char buf[32];
	snprintf(buf,31,"%d",mass_id);
	am->txt_id=lv_textarea_create(fields);
	lv_textarea_set_text(am->txt_id,buf);
	lv_textarea_set_one_line(am->txt_id,true);
	lv_textarea_set_accepted_chars(am->txt_id,VALID);
	lv_obj_add_event_cb(am->txt_id,lv_input_cb,LV_EVENT_CLICKED,am);
	lv_obj_set_grid_cell(
		am->txt_id,
		LV_GRID_ALIGN_STRETCH,1,2,
		LV_GRID_ALIGN_STRETCH,1,1
	);

	// Is CDROM
	am->cdrom=lv_checkbox_create(fields);
	lv_checkbox_set_text(am->cdrom,_("CDROM"));
	lv_obj_add_event_cb(am->cdrom,chk_cb,LV_EVENT_VALUE_CHANGED,am);
	lv_obj_set_grid_cell(
		am->cdrom,
		LV_GRID_ALIGN_START,0,3,
		LV_GRID_ALIGN_CENTER,2,1
	);

	// Is Removable
	am->removable=lv_checkbox_create(fields);
	lv_checkbox_set_text(am->removable,_("Removable"));
	lv_obj_set_checked(am->removable,true);
	lv_obj_set_user_data(am->removable,am);
	lv_obj_add_event_cb(am->removable,chk_cb,LV_EVENT_VALUE_CHANGED,am);
	lv_obj_set_grid_cell(
		am->removable,
		LV_GRID_ALIGN_START,0,3,
		LV_GRID_ALIGN_CENTER,3,1
	);

	// Is Read-Only
	am->ro=lv_checkbox_create(fields);
	lv_checkbox_set_text(am->ro,_("Read Only"));
	lv_obj_set_user_data(am->ro,am);
	lv_obj_add_event_cb(am->ro,chk_cb,LV_EVENT_VALUE_CHANGED,am);
	lv_obj_set_grid_cell(
		am->ro,
		LV_GRID_ALIGN_START,0,3,
		LV_GRID_ALIGN_CENTER,4,1
	);

	lv_obj_t*btns=lv_obj_create(am->box);
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
	lv_obj_center(btns);

	// OK Button
	am->ok=lv_btn_create(btns);
	lv_obj_set_enabled(am->ok,true);
	lv_obj_set_user_data(am->ok,am);
	lv_obj_add_event_cb(am->ok,ok_cb,LV_EVENT_CLICKED,am);
	lv_obj_set_flex_grow(am->ok,1);
	lv_obj_t*lbl_ok=lv_label_create(am->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);

	// Cancel Button
	am->cancel=lv_btn_create(btns);
	lv_obj_set_enabled(am->cancel,true);
	lv_obj_set_user_data(am->cancel,am);
	lv_obj_add_event_cb(am->cancel,cancel_cb,LV_EVENT_CLICKED,am);
	lv_obj_set_flex_grow(am->cancel,1);
	lv_obj_t*lbl_cancel=lv_label_create(am->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);

	if(act->args)set_disk_file(am,act->args);
	return 0;
}

struct gui_register guireg_add_mass={
	.name="usb-gadget-add-mass",
	.title="Add Mass Storage",
	.icon="usb.svg",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"^[A-Z]?:/dev/.+",
		".*\\.img$",
		".*\\.raw$",
		".*\\.iso$",
		NULL
	},
	.init=add_mass_init,
	.quiet_exit=add_mass_cleanup,
	.get_focus=add_mass_get_focus,
	.lost_focus=add_mass_lost_focus,
	.draw=draw_add_mass,
	.back=true,
	.mask=true,
};
#endif

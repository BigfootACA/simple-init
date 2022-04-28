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
#include"gui/sysbar.h"
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
	lv_checkbox_set_checked(am->ro,is_cdrom);
	lv_checkbox_set_checked(am->cdrom,is_cdrom);
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

static void sel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct gadget_add_mass*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->btn_sel)return;
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

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct gadget_add_mass*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->ok)return;
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
		lv_checkbox_is_checked(am->ro)
	);
	confd_set_boolean_array(
		base,cnt,"cdrom",
		lv_checkbox_is_checked(am->cdrom)
	);
	confd_set_boolean_array(
		base,cnt,"removable",
		lv_checkbox_is_checked(am->removable)
	);
	mass_id++;
	guiact_do_back();
	if(!guiact_has_activity_name("usb-gadget"))msgbox_create_yesno(
		restart_cb,
		"The operation will be applied after restarting USB Gadget service, "
		"do you want to restart USB Gadget service?"
	);
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct gadget_add_mass*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->cancel)return;
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

static void chk_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct gadget_add_mass*am=lv_obj_get_user_data(obj);
	if(am)am->check_changed=true;
}

static int add_mass_resize(struct gui_activity*act){
	lv_coord_t w=0,h=0;
	struct gadget_add_mass*am=act->data;
	if(!am)return 0;
	lv_obj_set_style_local_pad_all(
		am->box,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		gui_font_size
	);
	lv_obj_set_width(am->box,act->w/8*7);
	w=lv_page_get_scrl_width(am->box);
	lv_obj_set_width(am->title,w);
	lv_obj_set_y(am->title,h);
	h+=lv_obj_get_height(am->title)+gui_font_size;
	lv_obj_set_y(am->lbl_path,h);
	lv_obj_align_x(
		am->txt_path,am->lbl_path,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->txt_path,h);
	lv_obj_align(
		am->lbl_path,am->txt_path,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_style_local_radius(
		am->btn_sel,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_font_size/2
	);
	lv_obj_set_size(
		am->btn_sel,
		gui_font_size*3,
		lv_obj_get_height(am->txt_path)
	);
	lv_obj_set_width(
		am->txt_path,
		w-lv_obj_get_width(am->btn_sel)-
		lv_obj_get_width(am->lbl_path)-
		gui_font_size
	);
	lv_obj_align(
		am->btn_sel,am->txt_path,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/4,0
	);
	h+=lv_obj_get_height(am->btn_sel)+gui_font_size;
	lv_obj_set_y(am->lbl_id,h);
	lv_obj_set_width(
		am->txt_id,
		w-lv_obj_get_width(am->lbl_id)-
		gui_font_size
	);
	lv_obj_align_x(
		am->txt_id,am->lbl_id,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->txt_id,h);
	lv_obj_align(
		am->lbl_id,am->txt_id,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	h+=lv_obj_get_height(am->txt_id)+gui_font_size;
	lv_obj_set_y(am->cdrom,h);
	h+=lv_obj_get_height(am->cdrom)+gui_font_size;
	lv_obj_set_y(am->removable,h);
	h+=lv_obj_get_height(am->removable)+gui_font_size;
	lv_obj_set_y(am->ro,h);
	h+=lv_obj_get_height(am->ro)+gui_font_size;
	lv_obj_set_size(
		am->ok,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		am->ok,NULL,
		LV_ALIGN_IN_TOP_LEFT,
		(gui_font_size/2),h
	);
	lv_obj_set_size(
		am->cancel,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		am->cancel,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	h+=lv_obj_get_height(am->cancel)+(gui_font_size*3);
	lv_obj_set_height(am->box,MIN(h,(lv_coord_t)act->h/6*5));
	lv_obj_align(am->box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int draw_add_mass(struct gui_activity*act){
	struct gadget_add_mass*am=act->data;

	am->box=lv_page_create(act->page,NULL);
	lv_obj_set_click(am->box,false);

	// Title
	am->title=lv_label_create(am->box,NULL);
	lv_label_set_text(am->title,_("Add Mass Storage into USB Gadget"));
	lv_label_set_long_mode(am->title,LV_LABEL_LONG_BREAK);
	lv_label_set_align(am->title,LV_LABEL_ALIGN_CENTER);

	// File select
	am->lbl_path=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_path,_("Path:"));

	am->txt_path=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->txt_path,"");
	lv_textarea_set_one_line(am->txt_path,true);
	lv_textarea_set_cursor_hidden(am->txt_path,true);
	lv_obj_set_user_data(am->txt_path,am);
	lv_obj_set_event_cb(am->txt_path,lv_input_cb);

	am->btn_sel=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->btn_sel,true);
	lv_obj_set_user_data(am->btn_sel,am);
	lv_obj_set_event_cb(am->btn_sel,sel_cb);
	lv_label_set_text(lv_label_create(am->btn_sel,NULL),"...");

	// Function Name
	am->lbl_id=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_id,_("Name:"));

	char buf[32];
	snprintf(buf,31,"%d",mass_id);
	am->txt_id=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->txt_id,buf);
	lv_textarea_set_one_line(am->txt_id,true);
	lv_textarea_set_cursor_hidden(am->txt_id,true);
	lv_textarea_set_accepted_chars(am->txt_id,VALID);
	lv_obj_set_user_data(am->txt_id,am);
	lv_obj_set_event_cb(am->txt_id,lv_input_cb);

	// Is CDROM
	am->cdrom=lv_checkbox_create(am->box,NULL);
	lv_style_set_focus_checkbox(am->cdrom);
	lv_checkbox_set_text(am->cdrom,_("CDROM"));
	lv_obj_set_user_data(am->cdrom,am);
	lv_obj_set_event_cb(am->cdrom,chk_cb);

	// Is Removable
	am->removable=lv_checkbox_create(am->box,NULL);
	lv_style_set_focus_checkbox(am->removable);
	lv_checkbox_set_text(am->removable,_("Removable"));
	lv_checkbox_set_checked(am->removable,true);
	lv_obj_set_user_data(am->removable,am);
	lv_obj_set_event_cb(am->removable,chk_cb);

	// Is Read-Only
	am->ro=lv_checkbox_create(am->box,NULL);
	lv_style_set_focus_checkbox(am->ro);
	lv_checkbox_set_text(am->ro,_("Read Only"));
	lv_obj_set_user_data(am->ro,am);
	lv_obj_set_event_cb(am->ro,chk_cb);

	// OK Button
	am->ok=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->ok,true);
	lv_obj_set_user_data(am->ok,am);
	lv_obj_set_event_cb(am->ok,ok_cb);
	lv_label_set_text(lv_label_create(am->ok,NULL),_("OK"));

	// Cancel Button
	am->cancel=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->cancel,true);
	lv_obj_set_user_data(am->cancel,am);
	lv_obj_set_event_cb(am->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(am->cancel,NULL),_("Cancel"));

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
	.resize=add_mass_resize,
	.quiet_exit=add_mass_cleanup,
	.get_focus=add_mass_get_focus,
	.lost_focus=add_mass_lost_focus,
	.draw=draw_add_mass,
	.back=true,
	.mask=true,
};
#endif

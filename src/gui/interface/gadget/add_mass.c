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
	lv_obj_t*title;
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

static void path_cb(lv_event_t*e){
	struct gadget_add_mass*am=e->user_data;
	if(!am||am->txt_path!=e->target)return;
	const char*path=lv_textarea_get_text(am->txt_path);
	if(am->check_changed)return;
	char*ext=strrchr(path,'.');
	if(!ext)return;
	ext++;
	bool is_cdrom=strcasecmp(ext,"iso")==0;
	lv_obj_set_checked(am->ro,is_cdrom);
	lv_obj_set_checked(am->cdrom,is_cdrom);
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
	char*x;
	struct gadget_add_mass*am=e->user_data;
	static char*base="gadget.func";
	struct stat st;
	int cnt=confd_count(base);
	if(cnt<0)cnt=0;
	const char*path=lv_textarea_get_text(am->txt_path);
	if((x=strstr(path,"://"))){
		if(strncmp(path,"file",x-path)!=0){
			msgbox_alert("Only file:// path supported");
			return;
		}
		path=x+3;
	}
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

static int add_mass_init(struct gui_activity*act){
	struct gadget_add_mass*am=malloc(sizeof(struct gadget_add_mass));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct gadget_add_mass));
	act->data=am;
	char*p=act->args,*x;
	if(p&&(x=strstr(p,"://"))){
		if(strncmp(p,"file",x-p)!=0){
			msgbox_alert("Only file:// path supported");
			return -1;
		}
		act->args=x+3;
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
	struct gadget_add_mass*am=act->data;
	char buf[32];
	snprintf(buf,31,"%d",mass_id);
	am->box=lv_draw_dialog_box(act->page,&am->title,"Add Mass Storage into USB Gadget");
	lv_draw_input(am->box,"Path:",NULL,NULL,&am->txt_path,&am->btn_sel);
	lv_draw_input(am->box,"Name:",NULL,NULL,&am->txt_id,NULL);
	lv_obj_add_event_cb(am->txt_path,path_cb,LV_EVENT_DEFOCUSED,am);
	am->cdrom     =lv_draw_checkbox(am->box,"CDROM",     false, chk_cb,am);
	am->removable =lv_draw_checkbox(am->box,"Removable", true,  chk_cb,am);
	am->ro        =lv_draw_checkbox(am->box,"Read Only", false, chk_cb,am);
	lv_draw_btns_ok_cancel(am->box,&am->ok,&am->cancel,ok_cb,am);
	if(act->args){
		lv_textarea_set_text(am->txt_path,act->args);
		lv_event_send(am->txt_path,LV_EVENT_DEFOCUSED,NULL);
	}
	return 0;
}

struct gui_register guireg_add_mass={
	.name="usb-gadget-add-mass",
	.title="Add Mass Storage",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"file:///dev/.+",
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

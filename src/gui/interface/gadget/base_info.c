/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdlib.h>
#include<dirent.h>
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "gadget"

static lv_obj_t*sel_udc;
static lv_obj_t*txt_name,*txt_config,*txt_serial;
static lv_obj_t*txt_id_vendor,*txt_id_product;
static lv_obj_t*txt_manufacturer,*txt_product;
static lv_obj_t*btn_ok,*btn_reset,*btn_cancel;
static char*base="gadget";

static inline void lv_textarea_set_text_config(lv_obj_t*txt,char*key,char*def){
	char*v=confd_get_string_base(base,key,def);
	if(!v)return;
	lv_textarea_set_text(txt,v);
	free(v);
}

static void load_base_info(){
	char idv[6],idp[6];
	tlog_debug("loading gadget base info");
	snprintf(idv,5,"%04lX",confd_get_integer_base(base,"id_vendor",0));
	snprintf(idp,5,"%04lX",confd_get_integer_base(base,"id_product",0));
	lv_textarea_set_text(txt_id_vendor,idv);
	lv_textarea_set_text(txt_id_product,idp);
	lv_textarea_set_text_config(txt_name,"name","gadget");
	lv_textarea_set_text_config(txt_config,"config","linux-gadget");
	lv_textarea_set_text_config(txt_serial,"serial","1234567890");
	lv_textarea_set_text_config(txt_manufacturer,"manufacturer","manufacturer");
	lv_textarea_set_text_config(txt_product,"product","product");
	lv_dropdown_clear_options(sel_udc);
	lv_dropdown_add_option(sel_udc,_("(None)"),0);
	lv_dropdown_set_selected(sel_udc,0);
	DIR*d=opendir(_PATH_SYS_CLASS"/udc");
	if(d){
		char*p=confd_get_string_base(base,"udc",NULL);
		struct dirent*ent;
		while((ent=readdir(d))){
			if(ent->d_type!=DT_LNK)continue;
			if(ent->d_name[0]=='.')continue;
			uint16_t i=lv_dropdown_get_option_cnt(sel_udc);
			lv_dropdown_add_option(sel_udc,ent->d_name,i);
			if(p&&strcmp(p,ent->d_name)==0)lv_dropdown_set_selected(sel_udc,i);
		}
		if(p)free(p);
		closedir(d);
	}
}

static int save_base_info(){
	char*sv,*sp,*ev,*ep;
	long idv,idp;
	tlog_debug("saving gadget base info");
	sv=(char*)lv_textarea_get_text(txt_id_vendor);
	sp=(char*)lv_textarea_get_text(txt_id_product);
	errno=0,idv=strtol(sv,&ev,16),idp=strtol(sp,&ep,16);
	if(errno!=0||ev==sv||ep==sp||idv<0||idv>0xFFFF||idp<0||idp>0xFFFF){
		msgbox_alert("Invalid idVendor or idProduct number");
		return -1;
	}
	confd_set_integer_base(base,"id_vendor",idv);
	confd_set_integer_base(base,"id_product",idp);
	confd_set_string_base(base,"name",(char*)lv_textarea_get_text(txt_name));
	confd_set_string_base(base,"config",(char*)lv_textarea_get_text(txt_config));
	confd_set_string_base(base,"serial",(char*)lv_textarea_get_text(txt_serial));
	confd_set_string_base(base,"manufacturer",(char*)lv_textarea_get_text(txt_manufacturer));
	confd_set_string_base(base,"product",(char*)lv_textarea_get_text(txt_product));
	if(lv_dropdown_get_selected(sel_udc)==0)confd_delete("gadget.udc");
	else{
		char name[256]={0};
		lv_dropdown_get_selected_str(sel_udc,name,255);
		confd_set_string_base(base,"udc",name);
	}
	return 0;
}

static void btn_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	sysbar_focus_input(NULL);
	if(obj==btn_reset)load_base_info();
	else{
		if(obj==btn_ok&&save_base_info()!=0)return;
		guiact_do_back();
	}
}

static void input_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	sysbar_focus_input(obj);
	sysbar_keyboard_open();
}

static int base_info_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,txt_name);
	lv_group_add_obj(gui_grp,txt_config);
	lv_group_add_obj(gui_grp,txt_serial);
	lv_group_add_obj(gui_grp,txt_id_vendor);
	lv_group_add_obj(gui_grp,txt_id_product);
	lv_group_add_obj(gui_grp,txt_manufacturer);
	lv_group_add_obj(gui_grp,txt_product);
	lv_group_add_obj(gui_grp,sel_udc);
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,btn_reset);
	lv_group_add_obj(gui_grp,btn_cancel);
	return 0;
}

static int base_info_lost_focus(struct gui_activity*d __attribute__((unused))){
	sysbar_focus_input(NULL);
	lv_group_remove_obj(txt_name);
	lv_group_remove_obj(txt_config);
	lv_group_remove_obj(txt_serial);
	lv_group_remove_obj(txt_id_vendor);
	lv_group_remove_obj(txt_id_product);
	lv_group_remove_obj(txt_manufacturer);
	lv_group_remove_obj(txt_product);
	lv_group_remove_obj(sel_udc);
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(btn_reset);
	lv_group_remove_obj(btn_cancel);
	return 0;
}

static int base_info_clean(struct gui_activity*act __attribute__((unused))){
	sysbar_focus_input(NULL);
	return 0;
}

static int base_info_draw(struct gui_activity*act){
	int bth=gui_font_size+(gui_dpi/8),btw=gui_sw/3-(gui_dpi/8);

	// app title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_sh/32);
	lv_obj_set_size(title,gui_sw,gui_sh/16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("USB Gadget Base Info Edit"));

	// gadget name
	txt_name=lv_textarea_create(act->page,NULL);
	lv_textarea_set_one_line(txt_name,true);
	lv_textarea_set_max_length(txt_name,255);
	lv_textarea_set_cursor_hidden(txt_name,true);
	lv_obj_set_event_cb(txt_name,input_cb);
	lv_obj_set_width(txt_name,gui_sw-gui_font_size*2);
	lv_obj_align(txt_name,title,LV_ALIGN_OUT_BOTTOM_LEFT,gui_font_size,gui_font_size*2);
	lv_obj_t*lbl_name=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_name,_("Gadget Name:"));
	lv_obj_align(lbl_name,txt_name,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb gadget configuration
	txt_config=lv_textarea_create(act->page,NULL);
	lv_textarea_set_one_line(txt_config,true);
	lv_textarea_set_max_length(txt_config,255);
	lv_textarea_set_cursor_hidden(txt_config,true);
	lv_obj_set_event_cb(txt_config,input_cb);
	lv_obj_set_width(txt_config,gui_sw-gui_font_size*2);
	lv_obj_align(txt_config,txt_name,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_t*lbl_config=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_config,_("USB Configuration:"));
	lv_obj_align(lbl_config,txt_config,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb serial number
	txt_serial=lv_textarea_create(act->page,NULL);
	lv_textarea_set_accepted_chars(txt_serial,NUMBER"ABCDEF");
	lv_textarea_set_one_line(txt_serial,true);
	lv_textarea_set_max_length(txt_serial,64);
	lv_textarea_set_cursor_hidden(txt_serial,true);
	lv_obj_set_event_cb(txt_serial,input_cb);
	lv_obj_set_width(txt_serial,gui_sw-gui_font_size*2);
	lv_obj_align(txt_serial,txt_config,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_t*lbl_serial=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_serial,_("USB Serial Number:"));
	lv_obj_align(lbl_serial,txt_serial,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb id vendor
	txt_id_vendor=lv_textarea_create(act->page,NULL);
	lv_textarea_set_accepted_chars(txt_id_vendor,NUMBER"ABCDEF");
	lv_textarea_set_one_line(txt_id_vendor,true);
	lv_textarea_set_max_length(txt_id_vendor,4);
	lv_textarea_set_cursor_hidden(txt_id_vendor,true);
	lv_obj_set_event_cb(txt_id_vendor,input_cb);
	lv_obj_set_width(txt_id_vendor,(gui_sw-gui_font_size*3)/2);
	lv_obj_align(txt_id_vendor,txt_serial,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_t*lbl_id_vendor=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_id_vendor,_("USB Vendor ID:"));
	lv_obj_align(lbl_id_vendor,txt_id_vendor,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb id product
	txt_id_product=lv_textarea_create(act->page,NULL);
	lv_textarea_set_accepted_chars(txt_id_product,NUMBER"ABCDEF");
	lv_textarea_set_one_line(txt_id_product,true);
	lv_textarea_set_max_length(txt_id_product,4);
	lv_textarea_set_cursor_hidden(txt_id_product,true);
	lv_obj_set_event_cb(txt_id_product,input_cb);
	lv_obj_set_width(txt_id_product,(gui_sw-gui_font_size*3)/2);
	lv_obj_align(txt_id_product,txt_id_vendor,LV_ALIGN_OUT_RIGHT_BOTTOM,gui_font_size,0);
	lv_obj_t*lbl_id_product=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_id_product,_("USB Product ID:"));
	lv_obj_align(lbl_id_product,txt_id_product,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb manufacturer string
	txt_manufacturer=lv_textarea_create(act->page,NULL);
	lv_textarea_set_one_line(txt_manufacturer,true);
	lv_textarea_set_max_length(txt_manufacturer,255);
	lv_textarea_set_cursor_hidden(txt_manufacturer,true);
	lv_obj_set_event_cb(txt_manufacturer,input_cb);
	lv_obj_set_width(txt_manufacturer,gui_sw-gui_font_size*2);
	lv_obj_align(txt_manufacturer,txt_id_vendor,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_t*lbl_manufacturer=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_manufacturer,_("Manufacturer String:"));
	lv_obj_align(lbl_manufacturer,txt_manufacturer,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// usb product string
	txt_product=lv_textarea_create(act->page,NULL);
	lv_textarea_set_one_line(txt_product,true);
	lv_textarea_set_max_length(txt_product,255);
	lv_textarea_set_cursor_hidden(txt_product,true);
	lv_obj_set_event_cb(txt_product,input_cb);
	lv_obj_set_width(txt_product,gui_sw-gui_font_size*2);
	lv_obj_align(txt_product,txt_manufacturer,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_t*lbl_product=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_product,_("Product String:"));
	lv_obj_align(lbl_product,txt_product,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// udc selector
	sel_udc=lv_dropdown_create(act->page,NULL);
	lv_obj_set_width(sel_udc,gui_sw-gui_font_size*2);
	lv_obj_align(sel_udc,txt_product,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size*2);
	lv_obj_set_event_cb(sel_udc,lv_default_dropdown_cb);
	lv_obj_t*lbl_udc=lv_label_create(act->page,NULL);
	lv_label_set_text(lbl_udc,_("USB Device Controller (UDC):"));
	lv_obj_align(lbl_udc,sel_udc,LV_ALIGN_OUT_TOP_LEFT,gui_font_size/2,0);

	// ok button
	btn_ok=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_ok,btn_cb);
	lv_obj_set_size(btn_ok,btw,bth);
	lv_obj_align(btn_ok,sel_udc,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_style_set_action_button(btn_ok,true);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	// reset button
	btn_reset=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_reset,btn_cb);
	lv_obj_set_size(btn_reset,btw,bth);
	lv_obj_align(btn_reset,sel_udc,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_style_set_action_button(btn_reset,true);
	lv_label_set_text(lv_label_create(btn_reset,NULL),_("Reset"));

	// cancel button
	btn_cancel=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_cancel,btn_cb);
	lv_obj_set_size(btn_cancel,btw,bth);
	lv_obj_align(btn_cancel,sel_udc,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
	lv_style_set_action_button(btn_cancel,true);
	lv_label_set_text(lv_label_create(btn_cancel,NULL),_("Cancel"));

	load_base_info();

	return 0;
}

struct gui_register guireg_gadget_base_info={
	.name="usb-gadget-base-info",
	.title="USB Gadget Base Info",
	.icon="usb.svg",
	.show_app=false,
	.draw=base_info_draw,
	.quiet_exit=base_info_clean,
	.get_focus=base_info_get_focus,
	.lost_focus=base_info_lost_focus,
	.back=true
};
#endif

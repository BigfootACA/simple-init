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
	char*t="",*v;
	if((v=confd_get_string_base(base,key,def)))t=v;
	lv_textarea_set_text(txt,t);
	if(v)free(v);
}

static void load_base_info(){
	char id[8];
	tlog_debug("loading gadget base info");
	snprintf(id,7,"0x%04lX",confd_get_integer_base(base,"id_vendor",0));
	lv_textarea_set_text(txt_id_vendor,id);
	snprintf(id,7,"0x%04lX",confd_get_integer_base(base,"id_product",0));
	lv_textarea_set_text(txt_id_product,id);
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
			if(p&&strcmp(p,ent->d_name)==0)
				lv_dropdown_set_selected(sel_udc,i);
		}
		if(p)free(p);
		closedir(d);
	}
}

static int save_base_info(){
	long id;
	char*s,*e;
	tlog_debug("saving gadget base info");
	errno=0,s=(char*)lv_textarea_get_text(txt_id_vendor),id=strtol(s,&e,16);
	if(errno!=0||e==s||id<0||id>0xFFFF){
		msgbox_alert("Invalid vendor id number");
		return -1;
	}
	confd_set_integer_base(base,"id_vendor",id);
	errno=0,s=(char*)lv_textarea_get_text(txt_id_product),id=strtol(s,&e,16);
	if(errno!=0||e==s||id<0||id>0xFFFF){
		msgbox_alert("Invalid product id number");
		return -1;
	}
	confd_set_integer_base(base,"id_product",id);
	confd_set_string_base(base,"name",(char*)lv_textarea_get_text(txt_name));
	confd_set_string_base(base,"config",(char*)lv_textarea_get_text(txt_config));
	confd_set_string_base(base,"serial",(char*)lv_textarea_get_text(txt_serial));
	confd_set_string_base(base,"manufacturer",(char*)lv_textarea_get_text(txt_manufacturer));
	confd_set_string_base(base,"product",(char*)lv_textarea_get_text(txt_product));
	if(lv_dropdown_get_selected(sel_udc)==0)confd_delete("gadget.udc");
	else{
		char name[256];
		memset(name,0,sizeof(name));
		lv_dropdown_get_selected_str(sel_udc,name,255);
		confd_set_string_base(base,"udc",name);
	}
	return 0;
}

static void btn_cb(lv_event_t*e){
	sysbar_focus_input(NULL);
	if(e->target==btn_reset)load_base_info();
	else{
		if(e->target==btn_ok&&save_base_info()!=0)return;
		guiact_do_back();
	}
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
	static lv_coord_t grid_fields_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_fields_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// app title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("USB Gadget Base Info Edit"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);

	lv_obj_t*view=lv_obj_create(act->page);
	lv_obj_set_style_radius(view,0,0);
	lv_obj_set_style_border_width(view,0,0);
	lv_obj_set_style_bg_opa(view,LV_OPA_0,0);
	lv_obj_set_size(view,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_grid_dsc_array(view,grid_fields_col,grid_fields_row);
	lv_obj_set_flex_grow(view,1);

	// gadget name
	lv_obj_t*lbl_name=lv_label_create(view);
	lv_label_set_text(lbl_name,_("Gadget Name:"));
	txt_name=lv_textarea_create(view);
	lv_textarea_set_one_line(txt_name,true);
	lv_textarea_set_max_length(txt_name,255);
	lv_obj_add_event_cb(txt_name,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_name,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,0,1);
	lv_obj_set_grid_cell(txt_name,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,1,1);

	// usb gadget configuration
	lv_obj_t*lbl_config=lv_label_create(view);
	lv_label_set_text(lbl_config,_("USB Configuration:"));
	txt_config=lv_textarea_create(view);
	lv_textarea_set_one_line(txt_config,true);
	lv_textarea_set_max_length(txt_config,255);
	lv_obj_add_event_cb(txt_config,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_config,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,2,1);
	lv_obj_set_grid_cell(txt_config,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,3,1);

	// usb serial number
	lv_obj_t*lbl_serial=lv_label_create(view);
	lv_label_set_text(lbl_serial,_("USB Serial Number:"));
	txt_serial=lv_textarea_create(view);
	lv_textarea_set_accepted_chars(txt_serial,HEX);
	lv_textarea_set_one_line(txt_serial,true);
	lv_textarea_set_max_length(txt_serial,64);
	lv_obj_add_event_cb(txt_serial,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_serial,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,4,1);
	lv_obj_set_grid_cell(txt_serial,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,5,1);

	// usb id vendor
	lv_obj_t*lbl_id_vendor=lv_label_create(view);
	lv_label_set_text(lbl_id_vendor,_("USB Vendor ID:"));
	txt_id_vendor=lv_textarea_create(view);
	lv_textarea_set_accepted_chars(txt_id_vendor,HEX"x");
	lv_textarea_set_one_line(txt_id_vendor,true);
	lv_textarea_set_max_length(txt_id_vendor,6);
	lv_obj_add_event_cb(txt_id_vendor,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_id_vendor,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,6,1);
	lv_obj_set_grid_cell(txt_id_vendor,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,7,1);

	// usb id product
	lv_obj_t*lbl_id_product=lv_label_create(view);
	lv_label_set_text(lbl_id_product,_("USB Product ID:"));
	txt_id_product=lv_textarea_create(view);
	lv_textarea_set_accepted_chars(txt_id_product,HEX"x");
	lv_textarea_set_one_line(txt_id_product,true);
	lv_textarea_set_max_length(txt_id_product,6);
	lv_obj_add_event_cb(txt_id_product,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_id_product,LV_GRID_ALIGN_START,1,1,LV_GRID_ALIGN_CENTER,6,1);
	lv_obj_set_grid_cell(txt_id_product,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_STRETCH,7,1);

	// usb manufacturer string
	lv_obj_t*lbl_manufacturer=lv_label_create(view);
	lv_label_set_text(lbl_manufacturer,_("Manufacturer String:"));
	txt_manufacturer=lv_textarea_create(view);
	lv_textarea_set_one_line(txt_manufacturer,true);
	lv_textarea_set_max_length(txt_manufacturer,255);
	lv_obj_add_event_cb(txt_manufacturer,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_manufacturer,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,8,1);
	lv_obj_set_grid_cell(txt_manufacturer,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,9,1);

	// usb product string
	lv_obj_t*lbl_product=lv_label_create(view);
	lv_label_set_text(lbl_product,_("Product String:"));
	txt_product=lv_textarea_create(view);
	lv_textarea_set_one_line(txt_product,true);
	lv_textarea_set_max_length(txt_product,255);
	lv_obj_add_event_cb(txt_product,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_product,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,10,1);
	lv_obj_set_grid_cell(txt_product,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,11,1);

	// udc selector
	lv_obj_t*lbl_udc=lv_label_create(view);
	lv_label_set_text(lbl_udc,_("USB Device Controller (UDC):"));
	sel_udc=lv_dropdown_create(view);
	lv_obj_add_event_cb(sel_udc,lv_default_dropdown_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(lbl_udc,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,12,1);
	lv_obj_set_grid_cell(sel_udc,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,13,1);

	// ok button
	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,x)&(struct button_dsc){\
			&tgt,true,_(title),btn_cb,NULL,x,1,0,1,NULL\
		}
		BTN(btn_ok,     "OK",     0),
		BTN(btn_reset,  "Reset",  1),
		BTN(btn_cancel, "Cancel", 2),
		NULL
		#undef BTN
	);

	load_base_info();

	return 0;
}

struct gui_register guireg_gadget_base_info={
	.name="usb-gadget-base-info",
	.title="USB Gadget Base Info",
	.show_app=false,
	.draw=base_info_draw,
	.quiet_exit=base_info_clean,
	.get_focus=base_info_get_focus,
	.lost_focus=base_info_lost_focus,
	.back=true
};
#endif

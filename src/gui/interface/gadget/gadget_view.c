/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<unistd.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"init_internal.h"
#define TAG "gadget"

static lv_obj_t*view,*info;
static lv_obj_t*btn_add,*btn_edit;
static lv_obj_t*btn_delete,*btn_restart;
static lv_obj_t*btn_reload,*btn_base;
static char*base="gadget.func";

struct func_item{
	bool enable;
	lv_obj_t*btn,*lbl,*type,*mode;
	char id[64];
}items[64],*selected;

static void btns_toggle(bool state){
	lv_obj_set_enabled(btn_add,state);
	lv_obj_set_enabled(btn_edit,state);
	lv_obj_set_enabled(btn_delete,state);
}

static void clean_items(){
	if(info)lv_obj_del(info);
	info=NULL,selected=NULL;
	for(int i=0;i<64;i++){
		if(!items[i].enable)continue;
		lv_obj_del(items[i].btn);
		memset(&items[i],0,sizeof(struct func_item));
	}
}

static void set_info(char*text){
	btns_toggle(false);
	clean_items();
	info=lv_label_create(view);
	lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(info,lv_pct(100));
	lv_obj_set_style_text_align(info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(info,text);
}

static void item_click(lv_event_t*e){
	lv_obj_set_checked(e->target,true);
	if(selected){
		if(e->target==selected->btn)return;
		else lv_obj_set_checked(selected->btn,false);
	}
	selected=NULL;
	for(int i=0;i<64&&!selected;i++)
		if(items[i].enable&&items[i].btn==e->target)
			selected=&items[i];
	if(!selected)return;
	lv_obj_set_checked(selected->btn,true);
	btns_toggle(true);
	tlog_debug("selected function %s",selected->id);
}

static void view_add_item(struct func_item*k){
	char*b;
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};

	// option select button
	k->btn=lv_btn_create(view);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_size(k->btn,lv_pct(100),gui_dpi/2);
	lv_obj_set_grid_dsc_array(k->btn,grid_col,grid_row);
	lv_obj_add_event_cb(k->btn,item_click,LV_EVENT_CLICKED,k);
	lv_group_add_obj(gui_grp,k->btn);

	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;

	// function name and checkbox
	k->lbl=lv_label_create(k->btn);
	b=confd_get_string_dict(base,k->id,"name",NULL);
	lv_checkbox_set_text(k->lbl,b&&b[0]?b:_("(unknown)"));
	lv_obj_set_grid_cell(k->lbl,LV_GRID_ALIGN_START,0,2,LV_GRID_ALIGN_CENTER,0,1);
	if(b)free(b);

	// function type
	k->type=lv_label_create(k->btn);
	lv_label_set_long_mode(k->type,lm);
	b=confd_get_string_dict(base,k->id,"func",NULL);
	lv_label_set_text(k->type,b&&b[0]?b:_("(unknown)"));
	lv_obj_set_grid_cell(k->type,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);
	if(b)free(b);

	// function mode
	k->mode=lv_label_create(k->btn);
	lv_label_set_long_mode(k->mode,lm);
	lv_obj_set_style_text_align(k->mode,LV_TEXT_ALIGN_RIGHT,0);
	b=confd_get_string_dict(base,k->id,"mode",NULL);
	lv_label_set_text(k->mode,b&&b[0]?b:_("(unknown)"));
	lv_obj_set_grid_cell(k->mode,LV_GRID_ALIGN_END,1,1,LV_GRID_ALIGN_CENTER,1,1);
	if(b)free(b);
}

static void view_reload(){
	btns_toggle(false);
	clean_items();
	int i=0;
	char**list=confd_ls(base),*item;
	if(list)for(;(item=list[i]);i++){
		struct func_item*k=&items[i];
		k->enable=true;
		strcpy(k->id,item);
		view_add_item(k);
		if(i>=64){
			tlog_warn("functions too many, only show 64 functions");
			break;
		}
	}
	if(i==0)set_info(_("(none)"));
	tlog_info("found %d functions",i);
}

static int do_reload(struct gui_activity*d __attribute__((unused))){
	view_reload();
	return 0;
}

static int gadget_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,btn_add);
	lv_group_add_obj(gui_grp,btn_edit);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_restart);
	lv_group_add_obj(gui_grp,btn_reload);
	lv_group_add_obj(gui_grp,btn_base);
	return 0;
}

static int gadget_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<64;i++){
		if(!items[i].enable)continue;
		lv_group_remove_obj(items[i].btn);
	}
	lv_group_remove_obj(btn_add);
	lv_group_remove_obj(btn_edit);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_restart);
	lv_group_remove_obj(btn_reload);
	lv_group_remove_obj(btn_base);
	return 0;
}

static bool delete_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0&&selected){
		tlog_info("delete %s",selected->id);
		confd_delete_base(base,selected->id);
	}
	return false;
}

void gadget_restart_service(){
	tlog_info("try restart gadget service");
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_RESTART);
	strcpy(msg.data.data,"usb-gadget");
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		if(errno==0)errno=response.data.status.ret;
		telog_warn("restart service failed");
		msgbox_alert("Restart gadget service failed: %m");
	}
}

static bool restart_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0)gadget_restart_service();
	return false;
}

extern struct gui_register guireg_gadget_base_info;
static void btns_cb(lv_event_t*e){
	if(strcmp(guiact_get_last()->name,"usb-gadget")!=0)return;
	tlog_info("click button %s",(char*)e->user_data);
	if(e->target==btn_restart){
		msgbox_create_yesno(restart_cb,"Are you sure to restart gadget service?");
	}else if(e->target==btn_reload){
		view_reload();
	}else if(e->target==btn_base){
		guiact_start_activity(&guireg_gadget_base_info,NULL);
	}else if(!selected)return;
	else if(e->target==btn_add){

	}else if(e->target==btn_edit){

	}else if(e->target==btn_delete){
		msgbox_create_yesno(delete_cb,"Are you sure to delete function %s?",selected->id);
	}
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	clean_items();
	return 0;
}

static int gadget_draw(struct gui_activity*act){
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// app title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("USB Gadget"));

	// function view
	view=lv_obj_create(act->page);
	lv_obj_set_width(view,lv_pct(100));
	lv_obj_set_flex_flow(view,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(view,1);

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,data,title,x)&(struct button_dsc){\
			&tgt,en,title,btns_cb,data,x,1,0,1,NULL\
		}
		BTN(btn_add,     false, "add",      LV_SYMBOL_PLUS,     0),
		BTN(btn_edit,    false, "edit",     LV_SYMBOL_EDIT,     1),
		BTN(btn_delete,  false, "delete",   LV_SYMBOL_TRASH,    2),
		BTN(btn_restart, true,  "restart",  LV_SYMBOL_OK,       3),
		BTN(btn_reload,  true,  "reload",   LV_SYMBOL_REFRESH,  4),
		BTN(btn_base,    true,  "settings", LV_SYMBOL_SETTINGS, 5),
		#undef BTN
		NULL
	);

	return 0;
}

struct gui_register guireg_gadget={
	.name="usb-gadget",
	.title="USB Gadget",
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=gadget_draw,
	.get_focus=gadget_get_focus,
	.lost_focus=gadget_lost_focus,
	.data_load=do_reload,
	.back=true
};
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "theme"

struct theme_menu{
	lv_obj_t*box,*chk_bg,*chk_scroll;
	lv_obj_t*title,*txt_bg,*btn_bg,*chk_dark;
	lv_obj_t*btn_ok,*btn_restart;
	lv_obj_t*lbl_tips,*lbl_tip_icon;
};

static void dark_switch(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->chk_dark)return;
	bool val=lv_checkbox_is_checked(obj);
	if(val==gui_dark)return;
	lv_obj_set_x(tm->lbl_tip_icon,gui_font_size);
	lv_obj_set_x(tm->lbl_tips,gui_font_size+lv_obj_get_width(tm->lbl_tip_icon));
	gui_dark=val;
	confd_set_boolean("gui.dark",gui_dark);
}

static void text_scroll(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->chk_scroll)return;
	confd_set_boolean("gui.text_scroll",lv_checkbox_is_checked(obj));
}

static void do_save(struct theme_menu*tm){
	confd_set_boolean("gui.show_background",lv_checkbox_is_checked(tm->chk_bg));
	confd_set_string("gui.background",(char*)lv_textarea_get_text(tm->txt_bg));
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data __attribute__((unused))){
	struct theme_menu*tm=user_data;
	if(!ok||!tm)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	lv_textarea_set_text(tm->txt_bg,path[0]+2);
	return false;
}

static void bg_btn(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->btn_bg)return;
	filepicker_set_max_item(filepicker_create(select_cb,"Select background"),1);
}

static void bg_text(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->txt_bg)return;
	sysbar_focus_input(obj);
	sysbar_keyboard_open();
}

static void ok_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->btn_ok)return;
	do_save(tm);
	guiact_do_back();
}

static void do_restart(lv_task_t*t __attribute__((unused))){
	gui_screen_init();
	gui_draw();
}

static void restart_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct theme_menu*tm=lv_obj_get_user_data(obj);
	if(obj!=tm->btn_restart)return;
	do_save(tm);
	lv_task_once(lv_task_create(do_restart,100,LV_TASK_PRIO_LOWEST,NULL));
}

static int theme_menu_init(struct gui_activity*act){
	struct theme_menu*tm=malloc(sizeof(struct theme_menu));
	if(!tm)ERET(ENOMEM);
	memset(tm,0,sizeof(struct theme_menu));
	act->data=tm;
	return 0;
}

static int theme_menu_clean(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int theme_menu_resize(struct gui_activity*act){
	struct theme_menu*tm=act->data;
	int bts=gui_font_size+(gui_dpi/8);
	if(!tm)return -1;
	lv_obj_set_width(tm->box,gui_sw/6*5);
	lv_obj_set_width(
		tm->title,
		lv_obj_get_width(tm->box)
	);
	lv_obj_align(
		tm->title,NULL,
		LV_ALIGN_IN_TOP_MID,
		0,gui_font_size
	);
	lv_obj_set_width(
		tm->chk_bg,
		lv_obj_get_width(tm->box)-
		gui_font_size*2
	);
	lv_obj_align(
		tm->chk_bg,
		tm->title,
		LV_ALIGN_OUT_BOTTOM_LEFT,
		gui_font_size,gui_font_size
	);
	lv_obj_set_width(
		tm->txt_bg,
		lv_obj_get_width(tm->box)-
		gui_font_size*2
	);
	lv_obj_align(
		tm->txt_bg,
		tm->chk_bg,
		LV_ALIGN_OUT_BOTTOM_LEFT,
		0,gui_font_size
	);
	lv_obj_set_size(
		tm->btn_bg,
		gui_font_size*3,
		lv_obj_get_height(tm->txt_bg)
	);
	lv_obj_set_width(
		tm->txt_bg,
		lv_obj_get_width(tm->box)-
		lv_obj_get_width(tm->btn_bg)-
		gui_font_size*2
	);
	lv_obj_align(
		tm->btn_bg,
		tm->txt_bg,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/4,0
	);
	lv_obj_set_width(
		tm->chk_scroll,
		lv_obj_get_width(tm->box)-
		gui_font_size*2
	);
	lv_obj_align(
		tm->chk_scroll,
		tm->txt_bg,
		LV_ALIGN_OUT_BOTTOM_MID,
		0,gui_font_size
	);
	lv_obj_set_x(
		tm->chk_scroll,
		(lv_obj_get_width(tm->box)-
		lv_obj_get_width(tm->chk_scroll))/2
	);
	lv_obj_set_width(
		tm->chk_dark,
		lv_obj_get_width(tm->box)-
		gui_font_size*2
	);
	lv_obj_align(
		tm->chk_dark,
		tm->chk_scroll,
		LV_ALIGN_OUT_BOTTOM_MID,
		0,gui_font_size
	);
	lv_obj_align(
		tm->lbl_tip_icon,
		tm->chk_dark,
		LV_ALIGN_OUT_BOTTOM_LEFT,
		0,gui_font_size
	);
	lv_obj_set_x(tm->lbl_tip_icon,gui_font_size);
	lv_obj_set_width(
		tm->lbl_tips,
		lv_obj_get_width(tm->box)-
		gui_font_size*2-
		lv_obj_get_width(tm->lbl_tip_icon)
	);
	lv_obj_align(
		tm->lbl_tips,
		tm->lbl_tip_icon,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size,
		gui_font_size/4
	);
	lv_obj_set_size(
		tm->btn_ok,
		lv_obj_get_width(tm->box)/2-
		gui_font_size/3*4,bts
	);
	lv_obj_align(
		tm->btn_ok,
		tm->lbl_tips,
		LV_ALIGN_OUT_BOTTOM_LEFT,
		0,gui_font_size
	);
	lv_obj_set_x(tm->btn_ok,gui_font_size);
	lv_obj_set_size(
		tm->btn_restart,
		lv_obj_get_width(tm->box)/2-
		gui_font_size/3*4,bts
	);
	lv_obj_align(
		tm->btn_restart,
		tm->lbl_tips,
		LV_ALIGN_OUT_BOTTOM_RIGHT,
		0,gui_font_size
	);
	lv_obj_set_x(
		tm->btn_restart,
		lv_obj_get_width(tm->box)-
		lv_obj_get_width(tm->btn_restart)-
		gui_font_size
	);
	lv_obj_set_x(
		tm->lbl_tips,
		lv_obj_get_width(tm->lbl_tips)*2
	);
	lv_obj_set_x(
		tm->lbl_tip_icon,
		lv_obj_get_width(tm->lbl_tips)*2
	);
	lv_obj_set_height(tm->box,
		lv_obj_get_y(tm->chk_dark)+
		lv_obj_get_height(tm->lbl_tips)+
		lv_obj_get_height(tm->btn_ok)+
		(gui_font_size*3)+
		bts
	);
	lv_obj_align(tm->box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int theme_menu_draw(struct gui_activity*act){
	struct theme_menu*tm=act->data;
	if(!tm)return -1;

	tm->box=lv_obj_create(act->page,NULL);
	lv_obj_set_click(tm->box,false);
	lv_obj_set_style_local_pad_all(
		tm->box,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		gui_font_size
	);

	tm->title=lv_label_create(tm->box,NULL);
	lv_label_set_long_mode(tm->title,LV_LABEL_LONG_BREAK);
	lv_label_set_align(tm->title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(tm->title,_("Select theme"));

	tm->chk_bg=lv_checkbox_create(tm->box,NULL);
	lv_checkbox_set_checked(
		tm->chk_bg,
		confd_get_boolean("gui.show_background",true)
	);
	lv_checkbox_set_text(tm->chk_bg,_("Background"));

	tm->txt_bg=lv_textarea_create(tm->box,NULL);
	char*bg=confd_get_string("gui.background",NULL);
	lv_textarea_set_text(tm->txt_bg,bg?bg:"");
	lv_textarea_set_one_line(tm->txt_bg,true);
	lv_textarea_set_cursor_hidden(tm->txt_bg,true);
	lv_obj_set_user_data(tm->txt_bg,tm);
	lv_obj_set_event_cb(tm->txt_bg,bg_text);
	if(bg)free(bg);

	tm->btn_bg=lv_btn_create(tm->box,NULL);
	lv_style_set_action_button(tm->btn_bg,true);
	lv_obj_set_user_data(tm->btn_bg,tm);
	lv_obj_set_event_cb(tm->btn_bg,bg_btn);
	lv_obj_set_style_local_radius(
		tm->btn_bg,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_font_size/2
	);
	lv_label_set_text(lv_label_create(tm->btn_bg,NULL),"...");

	tm->chk_scroll=lv_checkbox_create(tm->box,NULL);
	lv_checkbox_set_checked(
		tm->chk_scroll,
		confd_get_boolean("gui.text_scroll",true)
	);
	lv_checkbox_set_text(
		tm->chk_scroll,
		_("Scroll text when overflow")
	);
	lv_obj_set_user_data(tm->chk_scroll,tm);
	lv_obj_set_event_cb(tm->chk_scroll,text_scroll);

	tm->chk_dark=lv_checkbox_create(tm->box,NULL);
	lv_checkbox_set_checked(tm->chk_dark,gui_dark);
	lv_checkbox_set_text(tm->chk_dark,_("Dark Mode"));
	lv_obj_set_user_data(tm->chk_dark,tm);
	lv_obj_set_event_cb(tm->chk_dark,dark_switch);

	tm->lbl_tip_icon=lv_label_create(tm->box,NULL);
	lv_label_set_align(tm->lbl_tip_icon,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(tm->lbl_tip_icon,LV_SYMBOL_WARNING);

	tm->lbl_tips=lv_label_create(tm->box,NULL);
	lv_label_set_long_mode(tm->lbl_tips,LV_LABEL_LONG_BREAK);
	lv_label_set_align(tm->lbl_tips,LV_LABEL_ALIGN_LEFT);
	lv_obj_set_small_text_font(tm->lbl_tips,LV_LABEL_PART_MAIN);
	lv_label_set_text(
		tm->lbl_tips,
		_("Need to restart GUI when changing theme")
	);

	tm->btn_ok=lv_btn_create(tm->box,NULL);
	lv_obj_set_user_data(tm->btn_ok,tm);
	lv_obj_set_event_cb(tm->btn_ok,ok_click);
	lv_style_set_action_button(tm->btn_ok,true);
	lv_label_set_text(
		lv_label_create(tm->btn_ok,NULL),
		_("OK")
	);
	tm->btn_restart=lv_btn_create(tm->box,NULL);
	lv_obj_set_user_data(tm->btn_restart,tm);
	lv_obj_set_event_cb(tm->btn_restart,restart_click);
	lv_style_set_action_button(tm->btn_restart,true);
	lv_label_set_text(
		lv_label_create(tm->btn_restart,NULL),
		_("Restart")
	);
	return 0;
}

static int theme_get_focus(struct gui_activity*d){
	struct theme_menu*tm=d->data;
	if(!tm)return 0;
	lv_group_add_obj(gui_grp,tm->chk_bg);
	lv_group_add_obj(gui_grp,tm->txt_bg);
	lv_group_add_obj(gui_grp,tm->btn_bg);
	lv_group_add_obj(gui_grp,tm->chk_scroll);
	lv_group_add_obj(gui_grp,tm->chk_dark);
	lv_group_add_obj(gui_grp,tm->btn_ok);
	lv_group_add_obj(gui_grp,tm->btn_restart);
	lv_group_focus_obj(tm->btn_ok);
	return 0;
}

static int theme_lost_focus(struct gui_activity*d){
	struct theme_menu*tm=d->data;
	if(!tm)return 0;
	lv_group_remove_obj(tm->chk_bg);
	lv_group_remove_obj(tm->txt_bg);
	lv_group_remove_obj(tm->btn_bg);
	lv_group_remove_obj(tm->chk_scroll);
	lv_group_remove_obj(tm->chk_dark);
	lv_group_remove_obj(tm->btn_ok);
	lv_group_remove_obj(tm->btn_restart);
	return 0;
}

struct gui_register guireg_theme_menu={
	.name="theme-select-menu",
	.title="Themes",
	.icon="theme.svg",
	.show_app=true,
	.init=theme_menu_init,
	.draw=theme_menu_draw,
	.resize=theme_menu_resize,
	.lost_focus=theme_lost_focus,
	.get_focus=theme_get_focus,
	.quiet_exit=theme_menu_clean,
	.back=true,
	.mask=true,
};
#endif

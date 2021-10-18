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
#include"gui/activity.h"
#define TAG "theme"

static lv_obj_t*box,*chk_dark,*btn_ok,*btn_restart,*lbl_tips,*lbl_tip_icon;

static void dark_switch(lv_obj_t*obj,lv_event_t e){
	if(obj!=chk_dark||e!=LV_EVENT_VALUE_CHANGED)return;
	bool val=lv_checkbox_is_checked(obj);
	if(val==gui_dark)return;
	lv_obj_set_x(lbl_tip_icon,gui_font_size);
	lv_obj_set_x(lbl_tips,gui_font_size+lv_obj_get_width(lbl_tip_icon));
	gui_dark=val;
	confd_set_boolean("gui.dark",gui_dark);
}

static void ok_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_ok||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static void do_restart(lv_task_t*t __attribute__((unused))){
	gui_screen_init();
	gui_draw();
}

static void restart_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_restart||e!=LV_EVENT_CLICKED)return;
	lv_task_once(lv_task_create(do_restart,100,LV_TASK_PRIO_LOWEST,NULL));
}

static int theme_menu_draw(struct gui_activity*act){
	int bts=gui_font_size+(gui_dpi/8);

	static lv_style_t bs;
	lv_style_init(&bs);
	lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

	box=lv_obj_create(act->page,NULL);
	lv_obj_add_style(box,LV_PAGE_PART_BG,&bs);
	lv_obj_set_click(box,false);
	lv_obj_set_width(box,gui_sw/6*5);

	lv_obj_t*txt=lv_label_create(box,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(txt,lv_obj_get_width(box));
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_obj_align(txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);
	lv_label_set_text(txt,_("Select theme"));

	chk_dark=lv_checkbox_create(box,NULL);
	lv_checkbox_set_checked(chk_dark,gui_dark);
	lv_checkbox_set_text(chk_dark,_("Dark Mode"));
	lv_obj_set_event_cb(chk_dark,dark_switch);
	lv_obj_set_width(chk_dark,lv_obj_get_width(box)-gui_font_size*2);
	lv_obj_align(chk_dark,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size*2);

	lbl_tip_icon=lv_label_create(box,NULL);
	lv_label_set_align(lbl_tip_icon,LV_LABEL_ALIGN_CENTER);
	lv_obj_align(lbl_tip_icon,chk_dark,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_obj_set_x(lbl_tip_icon,gui_font_size);
	lv_obj_set_style_local_text_font(lbl_tip_icon,LV_STATE_DEFAULT,LV_LABEL_PART_MAIN,symbol_font);
	lv_label_set_text(lbl_tip_icon,LV_SYMBOL_WARNING);

	lbl_tips=lv_label_create(box,NULL);
	lv_label_set_long_mode(lbl_tips,LV_LABEL_LONG_BREAK);
	lv_label_set_align(lbl_tips,LV_LABEL_ALIGN_LEFT);
	lv_obj_set_width(lbl_tips,lv_obj_get_width(box)-gui_font_size*2-lv_obj_get_width(lbl_tip_icon));
	lv_obj_align(lbl_tips,lbl_tip_icon,LV_ALIGN_OUT_RIGHT_MID,gui_font_size,gui_font_size/4);
	lv_obj_set_small_text_font(lbl_tips,LV_LABEL_PART_MAIN);
	lv_label_set_text_fmt(lbl_tips,_("Need to restart GUI when changing theme"));

	btn_ok=lv_btn_create(box,NULL);
	lv_obj_set_event_cb(btn_ok,ok_click);
	lv_obj_set_size(btn_ok,lv_obj_get_width(box)/2-gui_font_size/3*4,bts);
	lv_obj_align(btn_ok,lbl_tips,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_obj_set_x(btn_ok,gui_font_size);
	lv_style_set_action_button(btn_ok,true);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	btn_restart=lv_btn_create(box,NULL);
	lv_obj_set_event_cb(btn_restart,restart_click);
	lv_obj_set_size(btn_restart,lv_obj_get_width(box)/2-gui_font_size/3*4,bts);
	lv_obj_align(btn_restart,lbl_tips,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
	lv_obj_set_x(btn_restart,lv_obj_get_width(box)-lv_obj_get_width(btn_restart)-gui_font_size);
	lv_style_set_action_button(btn_restart,true);
	lv_label_set_text(lv_label_create(btn_restart,NULL),_("Restart"));

	lv_obj_set_x(lbl_tips,lv_obj_get_width(lbl_tips)*2);
	lv_obj_set_x(lbl_tip_icon,lv_obj_get_width(lbl_tips)*2);

	lv_obj_set_height(box,
		lv_obj_get_y(chk_dark)+
		lv_obj_get_height(lbl_tips)+
		lv_obj_get_height(btn_ok)+
		(gui_font_size*3)+
		bts
	);
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int theme_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,chk_dark);
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,btn_restart);
	lv_group_focus_obj(btn_ok);
	return 0;
}

static int theme_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(chk_dark);
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(btn_restart);
	return 0;
}

struct gui_register guireg_theme_menu={
	.name="theme-select-menu",
	.title="Themes",
	.icon="theme.png",
	.show_app=true,
	.draw=theme_menu_draw,
	.lost_focus=theme_lost_focus,
	.get_focus=theme_get_focus,
	.back=true,
	.mask=true,
};
#endif

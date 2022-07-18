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
#include"gui/filepicker.h"
#define TAG "theme"

struct theme_menu{
	lv_obj_t*box,*chk_bg,*chk_scroll;
	lv_obj_t*title,*txt_bg,*btn_bg,*chk_dark;
	lv_obj_t*btn_ok,*btn_restart;
	lv_obj_t*lbl_tips,*lbl_tip_icon;
	lv_obj_t*input_box,*tip_line,*btns;
};

static void dark_switch(lv_event_t*e){
	struct theme_menu*tm=e->user_data;
	bool val=lv_obj_is_checked(e->target);
	if(val==gui_dark)return;
	lv_obj_clear_flag(tm->tip_line,LV_OBJ_FLAG_HIDDEN);
	gui_dark=val;
	confd_set_boolean("gui.dark",gui_dark);
}

static void text_scroll(lv_event_t*e){
	confd_set_boolean("gui.text_scroll",lv_obj_is_checked(e->target));
}

static void do_save(struct theme_menu*tm){
	confd_set_boolean("gui.show_background",lv_obj_is_checked(tm->chk_bg));
	confd_set_string("gui.background",(char*)lv_textarea_get_text(tm->txt_bg));
}

static void ok_click(lv_event_t*e){
	struct theme_menu*tm=e->user_data;
	do_save(tm);
	guiact_do_back();
}

static void do_restart(lv_timer_t*t __attribute__((unused))){
	gui_screen_init();
	gui_draw();
}

static void restart_click(lv_event_t*e){
	struct theme_menu*tm=e->user_data;
	do_save(tm);
	lv_timer_set_repeat_count(lv_timer_create(do_restart,100,NULL),1);
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

static int theme_menu_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_col_f[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	struct theme_menu*tm=act->data;
	if(!tm)return -1;

	tm->box=lv_obj_create(act->page);
	lv_obj_set_flex_flow(tm->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tm->box,gui_font_size/2,0);
	lv_obj_set_style_pad_row(tm->box,gui_font_size/2,0);
	lv_obj_set_style_max_width(tm->box,lv_pct(80),0);
	lv_obj_set_style_max_height(tm->box,lv_pct(80),0);
	lv_obj_set_style_min_width(tm->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(tm->box,gui_font_size*2,0);
	lv_obj_set_height(tm->box,LV_SIZE_CONTENT);
	lv_obj_center(tm->box);

	tm->title=lv_label_create(tm->box);
	lv_label_set_text(tm->title,_("Select theme"));
	lv_label_set_long_mode(tm->title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(tm->title,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(tm->title,lv_pct(100));

	tm->input_box=lv_draw_input(
		tm->box,_("Background"),
		&tm->chk_bg,NULL,
		&tm->txt_bg,&tm->btn_bg
	);
	lv_obj_set_checked(
		tm->chk_bg,
		confd_get_boolean("gui.show_background",true)
	);
	char*bg=confd_get_string("gui.background",NULL);
	lv_textarea_set_text(tm->txt_bg,bg?bg:"");
	if(bg)free(bg);

	tm->chk_scroll=lv_checkbox_create(tm->box);
	lv_obj_set_checked(
		tm->chk_scroll,
		confd_get_boolean("gui.text_scroll",true)
	);
	lv_checkbox_set_text(
		tm->chk_scroll,
		_("Scroll text when overflow")
	);
	lv_obj_add_event_cb(tm->chk_scroll,text_scroll,LV_EVENT_VALUE_CHANGED,tm);

	tm->chk_dark=lv_checkbox_create(tm->box);
	lv_obj_set_checked(tm->chk_dark,gui_dark);
	lv_checkbox_set_text(tm->chk_dark,_("Dark Mode"));
	lv_obj_add_event_cb(tm->chk_dark,dark_switch,LV_EVENT_VALUE_CHANGED,tm);

	tm->tip_line=lv_obj_create(tm->box);
	lv_obj_add_flag(tm->tip_line,LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_border_width(tm->tip_line,0,0);
	lv_obj_set_style_pad_all(tm->tip_line,0,0);
	lv_obj_set_grid_dsc_array(tm->tip_line,grid_col,grid_row);
	lv_obj_set_size(tm->tip_line,lv_pct(100),LV_SIZE_CONTENT);

	tm->lbl_tip_icon=lv_label_create(tm->tip_line);
	lv_obj_set_style_text_align(tm->lbl_tip_icon,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_grid_cell(tm->lbl_tip_icon,LV_GRID_ALIGN_CENTER,0,1,LV_GRID_ALIGN_CENTER,0,1);
	lv_label_set_text(tm->lbl_tip_icon,LV_SYMBOL_WARNING);

	tm->lbl_tips=lv_label_create(tm->tip_line);
	lv_label_set_long_mode(tm->lbl_tips,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(tm->lbl_tips,LV_TEXT_ALIGN_LEFT,0);
	lv_obj_set_small_text_font(tm->lbl_tips,LV_PART_MAIN);
	lv_obj_set_grid_cell(tm->lbl_tips,LV_GRID_ALIGN_START,1,1,LV_GRID_ALIGN_CENTER,0,1);
	lv_label_set_text(
		tm->lbl_tips,
		_("Need to restart GUI when changing theme")
	);

	tm->btns=lv_obj_create(tm->box);
	lv_obj_set_style_border_width(tm->btns,0,0);
	lv_obj_set_style_pad_all(tm->btns,gui_font_size/4,0);
	lv_obj_set_grid_dsc_array(tm->btns,grid_col_f,grid_row);
	lv_obj_set_size(tm->btns,lv_pct(100),LV_SIZE_CONTENT);

	tm->btn_ok=lv_btn_create(tm->btns);
	lv_obj_add_event_cb(tm->btn_ok,ok_click,LV_EVENT_CLICKED,tm);
	lv_obj_set_grid_cell(tm->btn_ok,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_ok=lv_label_create(tm->btn_ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);

	tm->btn_restart=lv_btn_create(tm->btns);
	lv_obj_add_event_cb(tm->btn_restart,restart_click,LV_EVENT_CLICKED,tm);
	lv_obj_set_grid_cell(tm->btn_restart,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_restart=lv_label_create(tm->btn_restart);
	lv_label_set_text(lbl_restart,_("Restart"));
	lv_obj_center(lbl_restart);
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
	.lost_focus=theme_lost_focus,
	.get_focus=theme_get_focus,
	.quiet_exit=theme_menu_clean,
	.back=true,
	.mask=true,
};
#endif

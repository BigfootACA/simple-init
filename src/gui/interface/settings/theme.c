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

static void do_restart(void*d __attribute__((unused))){
	gui_screen_init();
	gui_draw();
}

static void restart_click(lv_event_t*e){
	struct theme_menu*tm=e->user_data;
	do_save(tm);
	lv_async_call(do_restart,NULL);
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
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	struct theme_menu*tm=act->data;
	if(!tm)return -1;

	tm->box=lv_draw_dialog_box(act->page,&tm->title,"Select theme");

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

	tm->chk_scroll=lv_draw_checkbox(
		tm->box,"Scroll text when overflow",
		confd_get_boolean("gui.text_scroll",true),
		text_scroll,tm
	);
	tm->chk_dark=lv_draw_checkbox(
		tm->box,"Dark Mode",
		gui_dark,dark_switch,tm
	);

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

	tm->btns=lv_draw_buttons_auto_arg(
		tm->box,
		#define BTN(tgt,title,cb,x)&(struct button_dsc){\
			&tgt,true,_(title),cb,tm,x,1,0,1,NULL\
		}
		BTN(tm->btn_ok,      "OK",      ok_click,      0),
		BTN(tm->btn_restart, "Restart", restart_click, 1),
		NULL
		#undef BTN
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

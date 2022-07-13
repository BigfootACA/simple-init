/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_LIBTSM
#include<ctype.h>
#include<errno.h>
#include<string.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include"gui.h"
#include"array.h"
#include"libtsm.h"
#include"defines.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/sysbar.h"
#include"gui/console.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/termview.h"
#include"gui/clipboard.h"
#include"xkbcommon/xkbcommon-keysyms.h"
#define TAG "console"

static struct alt_pad_btn_map btns[6][6];

static void pad_key_cb(struct alt_pad_btn*btn){
	struct tsm_screen*scr=lv_termview_get_screen(
		btn->con->termview
	);
	if(btn->map->attr>0&&tsm_vte_handle_keyboard(
		lv_termview_get_vte(btn->con->termview),
		btn->map->attr,0,
		btn->con->mods,
		btn->map->attr
	))tsm_screen_sb_reset(scr);
	lv_termview_update(btn->con->termview);
}

static void pad_toggle_cb(struct alt_pad_btn*btn){
	bool checked=lv_obj_is_checked(btn->btn);
	if(checked)btn->con->mods|=btn->map->attr;
	else btn->con->mods&=~btn->map->attr;
	lv_termview_set_mods(
		btn->con->termview,
		btn->con->mods
	);
}

static void pad_close_cb(struct alt_pad_btn*btn){
	size_t x,y;
	lv_obj_add_flag(btn->con->alt_btns,LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(btn->con->toggle_btn,LV_OBJ_FLAG_HIDDEN);
	for(x=0;x<ARRLEN(btns);x++)for(y=0;y<ARRLEN(btns[x]);y++)
		lv_group_remove_obj(btn->con->btns[x][y].btn);
	lv_group_add_obj(gui_grp,btn->con->toggle_btn);
}

static void pad_move_cb(struct alt_pad_btn*btn){
	lv_coord_t pm=gui_font_size*2;
	lv_coord_t sw=btn->con->act->w;
	lv_coord_t sh=btn->con->act->h;
	lv_coord_t pw=lv_obj_get_width(btn->con->alt_btns);
	lv_coord_t ph=lv_obj_get_height(btn->con->alt_btns);
	lv_coord_t py=lv_obj_get_y(btn->con->alt_btns);
	lv_coord_t ax=(sw-pw)/2;
	lv_coord_t ay=py>(sh/2)?pm:(sh-ph-pm);
	lv_obj_set_pos(btn->con->alt_btns,ax,ay);
}

static void pad_scr_cb(struct alt_pad_btn*btn){
	struct tsm_screen*scr=lv_termview_get_screen(
		btn->con->termview
	);
	switch(btn->map->attr){
		case 0:tsm_screen_reset(scr);break;
		case 1:tsm_screen_sb_page_up(scr,1);break;
		case 2:tsm_screen_sb_page_down(scr,1);break;
	}
}

static bool copy_cb(
	bool ok,
	const char*content,
	void*data __attribute__((unused))
){
	if(ok)clipboard_set(
		CLIP_TEXT,
		content,
		strlen(content)
	);
	return false;
}

static void pad_copy_cb(struct alt_pad_btn*btn){
	size_t bs=0,p=0;
	char*out=NULL,*buf=NULL;
	uint32_t col,row,c,r;
	struct tsm_screen*scr=lv_termview_get_screen(
		btn->con->termview
	);
	col=lv_termview_get_cols(btn->con->termview);
	row=lv_termview_get_rows(btn->con->termview);
	tsm_screen_selection_reset(scr);
	tsm_screen_selection_start(scr,0,0);
	tsm_screen_selection_target(scr,col,row);
	tsm_screen_selection_copy(scr,&out);
	tsm_screen_selection_reset(scr);
	bs=col*row+1;
	if(!out||!(buf=malloc(bs)))goto fail;
	for(r=0;r<row;r++)for(c=0;c<col;c++){
		char x=out[(r*col)+c];
		if(x)buf[p++]=x;
	}
	do{buf[p]=0;}while(buf[--p]=='\n');
	struct inputbox*in=inputbox_create(
		copy_cb,
		"Select string to copy"
	);
	inputbox_set_one_line(in,false);
	inputbox_set_text_select(in,true);
	inputbox_set_content(in,"%s",buf);
	fail:
	if(buf)free(buf);
	if(out)free(out);
}

static struct alt_pad_btn_map btns[6][6]={
	{
		{.type=ALT_PAD_ACT_CLOSE,     .title=LV_SYMBOL_CLOSE,     .hold=false, .callback=pad_close_cb,  .attr=0},
		{.type=ALT_PAD_KEY_ENTER,     .title="ENTER",             .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Return},
		{.type=ALT_PAD_KEY_SUPER,     .title="SUPER",             .hold=true,  .callback=pad_toggle_cb, .attr=TSM_LOGO_MASK},
		{.type=ALT_PAD_KEY_CTRL,      .title="CTRL",              .hold=true,  .callback=pad_toggle_cb, .attr=TSM_CONTROL_MASK},
		{.type=ALT_PAD_KEY_F1,        .title="F1",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F1},
		{.type=ALT_PAD_KEY_F7,        .title="F7",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F7},
	},{
		{.type=ALT_PAD_KEY_TAB,       .title="TAB",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Tab},
		{.type=ALT_PAD_KEY_CAPS,      .title="CAPS",              .hold=true,  .callback=pad_toggle_cb, .attr=TSM_LOCK_MASK},
		{.type=ALT_PAD_KEY_SHIFT,     .title="SHIFT",             .hold=true,  .callback=pad_toggle_cb, .attr=TSM_SHIFT_MASK},
		{.type=ALT_PAD_KEY_ALT,       .title="ALT",               .hold=true,  .callback=pad_toggle_cb, .attr=TSM_ALT_MASK},
		{.type=ALT_PAD_KEY_F2,        .title="F2",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F2},
		{.type=ALT_PAD_KEY_F8,        .title="F8",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F8},
	},{
		{.type=ALT_PAD_KEY_PGUP,      .title="PGUP",              .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Page_Up},
		{.type=ALT_PAD_KEY_LEFT,      .title=LV_SYMBOL_LEFT,      .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Left},
		{.type=ALT_PAD_KEY_HOME,      .title="HOME",              .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Home},
		{.type=ALT_PAD_KEY_BACKSPACE, .title=LV_SYMBOL_BACKSPACE, .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_BackSpace},
		{.type=ALT_PAD_KEY_F3,        .title="F3",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F3},
		{.type=ALT_PAD_KEY_F9,        .title="F9",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F9},
	},{
		{.type=ALT_PAD_KEY_UP,        .title=LV_SYMBOL_UP,        .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Up},
		{.type=ALT_PAD_ACT_MOVE,      .title="\uf0b2",            .hold=false, .callback=pad_move_cb,   .attr=0},
		{.type=ALT_PAD_KEY_DOWN,      .title=LV_SYMBOL_DOWN,      .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Down},
		{.type=ALT_PAD_KEY_DEL,       .title="DEL",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Delete},
		{.type=ALT_PAD_KEY_F4,        .title="F4",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F4},
		{.type=ALT_PAD_KEY_F10,       .title="F10",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F10},
	},{
		{.type=ALT_PAD_KEY_PGDN,      .title="PGDN",              .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Page_Down},
		{.type=ALT_PAD_KEY_RIGHT,     .title=LV_SYMBOL_RIGHT,     .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Right},
		{.type=ALT_PAD_KEY_END,       .title="END",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_End},
		{.type=ALT_PAD_ACT_RESET,     .title=LV_SYMBOL_REFRESH,   .hold=false, .callback=pad_scr_cb,    .attr=0},
		{.type=ALT_PAD_KEY_F5,        .title="F5",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F5},
		{.type=ALT_PAD_KEY_F11,       .title="F11",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F11},
	},{
		{.type=ALT_PAD_KEY_ESC,       .title="ESC",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_Escape},
		{.type=ALT_PAD_ACT_COPY,      .title=LV_SYMBOL_COPY,      .hold=false, .callback=pad_copy_cb,   .attr=0},
		{.type=ALT_PAD_ACT_SCR_UP,    .title="\uf102",            .hold=false, .callback=pad_scr_cb,    .attr=1},
		{.type=ALT_PAD_ACT_SCR_DOWN,  .title="\uf103",            .hold=false, .callback=pad_scr_cb,    .attr=2},
		{.type=ALT_PAD_KEY_F6,        .title="F6",                .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F6},
		{.type=ALT_PAD_KEY_F12,       .title="F12",               .hold=false, .callback=pad_key_cb,    .attr=XKB_KEY_F12},
	}
};

static uint32_t get_key(){
	lv_indev_t*i=lv_indev_get_act();
	return i?lv_indev_get_key(i):0;
}

static void hand_key(struct console*con,uint32_t k){
	struct tsm_vte*vte=lv_termview_get_vte(con->termview);
	struct tsm_screen*scr=lv_termview_get_screen(con->termview);
	if(k==0)return;
	if(con->mods&TSM_SHIFT_MASK){
		if(!(con->mods&TSM_LOCK_MASK))k=toupper(k);
	}else if(con->mods&TSM_LOCK_MASK)k=toupper(k);
	if(tsm_vte_handle_keyboard(vte,0,k,con->mods,k))
		tsm_screen_sb_reset(scr);
	lv_termview_update(con->termview);
}

static void hand_key_event(lv_event_t*e){
	hand_key(e->user_data,get_key());
}

static void hand_no_ctrl_key_event(lv_event_t*e){
	uint32_t k=get_key();
	switch(k){
		case LV_KEY_UP:
		case LV_KEY_DOWN:
		case LV_KEY_RIGHT:
		case LV_KEY_LEFT:
		case LV_KEY_ENTER:
		case LV_KEY_NEXT:
		case LV_KEY_PREV:break;
		default:hand_key(e->user_data,k);
	}
}

static void con_drag_cb(lv_event_t*e){
	struct console*con=e->user_data;
	lv_drag_border(
		e->target,
		con->act->w,
		con->act->h,
		gui_font_size
	);
}

static void alt_pad_btn_cb(lv_event_t*e){
	struct alt_pad_btn*btn=e->user_data;
	btn->map->callback(btn);
}

static void alt_pad_draw_button(struct console*con){
	lv_obj_t*lo;
	size_t cw,x,y;
	lv_coord_t bh=gui_font_size*2;
	lv_coord_t pw=0,ph=0,bw,px,py;
	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,0);
	lv_style_set_pad_all(&btn_style,0);
	lv_style_set_bg_opa(&btn_style,LV_OPA_30);
	for(x=0;x<ARRLEN(btns);x++){
		cw=1;
		for(y=0;y<ARRLEN(btns[x]);y++)
			cw=MAX(cw,strlen(btns[x][y].title));
		bw=(cw+2)*gui_font_size/2;
		for(y=0;y<6;y++){
			px=0,py=0;
			con->btns[x][y].con=con;
			con->btns[x][y].map=&btns[x][y];
			con->btns[x][y].btn=lv_btn_create(con->alt_btns);
			if(x>0)lo=con->btns[x-1][y].btn,
				px=lv_obj_get_x(lo)+lv_obj_get_width(lo);
			if(y>0)lo=con->btns[x][y-1].btn,
				py=lv_obj_get_y(lo)+lv_obj_get_height(lo);
			lv_obj_add_style(con->btns[x][y].btn,&btn_style,LV_PART_MAIN);
			lv_obj_set_user_data(con->btns[x][y].btn,&con->btns[x][y]);
			if(btns[x][y].hold)lv_obj_add_flag(con->btns[x][y].btn,LV_OBJ_FLAG_CHECKABLE);
			lv_obj_add_event_cb(con->btns[x][y].btn,alt_pad_btn_cb,LV_EVENT_CLICKED,&con->btns[x][y]);
			lv_obj_add_event_cb(con->btns[x][y].btn,hand_no_ctrl_key_event,LV_EVENT_KEY,con);
			lv_obj_set_size(con->btns[x][y].btn,bw,bh);
			lv_obj_set_pos(con->btns[x][y].btn,px,py);
			con->btns[x][y].lbl=lv_label_create(con->btns[x][y].btn);
			lv_label_set_text(con->btns[x][y].lbl,btns[x][y].title);
			lv_obj_center(con->btns[x][y].lbl);
			lv_obj_update_layout(con->btns[x][y].btn);
			pw=MAX(pw,px+bw),ph=MAX(ph,py+bh);
		}
	}
	lv_obj_set_content_width(con->alt_btns,pw);
	lv_obj_set_content_height(con->alt_btns,ph);
}

int console_init(struct gui_activity*act){
	struct console*con=malloc(sizeof(struct console));
	if(!con)return -ENOMEM;
	memset(con,0,sizeof(struct console));
	act->data=con,con->act=act;
	return 0;
}

static void term_cb(lv_event_t*e){
	struct console*con=e->user_data;
	if(guiact_get_last()!=con->act)return;
	sysbar_focus_input(lv_termview_get_virtual_input(
		con->termview
	));
}

static void toggle_cb(lv_event_t*e){
	size_t x,y;
	struct console*con=e->user_data;
	if(guiact_get_last()!=con->act)return;
	lv_obj_add_flag(con->toggle_btn,LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(con->alt_btns,LV_OBJ_FLAG_HIDDEN);
	for(x=0;x<ARRLEN(btns);x++)for(y=0;y<ARRLEN(btns[x]);y++)
		lv_group_add_obj(gui_grp,con->btns[x][y].btn);
	lv_group_remove_obj(con->toggle_btn);
}

int console_draw(struct gui_activity*act){
	struct console*con=act->data;
	lv_obj_set_scrollbar_mode(act->page,LV_DIR_NONE);
	lv_obj_clear_flag(act->page,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(act->page,0,0);
	con->termview=lv_termview_create(act->page);
	lv_obj_set_user_data(con->termview,con);
	lv_obj_add_event_cb(con->termview,term_cb,LV_EVENT_CLICKED,con);
	lv_obj_add_event_cb(con->termview,hand_key_event,LV_EVENT_KEY,con);
	lv_termview_set_font(con->termview,gui_font_small);
	lv_obj_set_style_bg_color(con->termview,lv_color_black(),0);
	con->alt_btns=lv_obj_create(act->page);
	lv_obj_set_style_pad_all(con->alt_btns,0,0);
	lv_obj_set_scrollbar_mode(con->alt_btns,LV_DIR_NONE);
	lv_obj_add_event_cb(con->alt_btns,hand_no_ctrl_key_event,LV_EVENT_KEY,con);
	lv_obj_add_event_cb(con->alt_btns,con_drag_cb,LV_EVENT_SCROLL_END,con);
	lv_obj_add_flag(con->alt_btns,LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(con->alt_btns,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_opa(
		con->alt_btns,
		LV_OPA_0,0
	);
	alt_pad_draw_button(con);
	lv_obj_add_drag(con->alt_btns);
	con->toggle_btn=lv_btn_create(act->page);
	lv_obj_add_drag(con->toggle_btn);
	lv_obj_add_event_cb(con->toggle_btn,toggle_cb,LV_EVENT_CLICKED,con);
	lv_obj_add_event_cb(con->toggle_btn,hand_no_ctrl_key_event,LV_EVENT_KEY,con);
	lv_obj_set_style_radius(con->toggle_btn,LV_RADIUS_CIRCLE,0);
	lv_obj_set_checked(con->toggle_btn,true);
	lv_obj_set_style_shadow_color(
		con->toggle_btn,
		lv_color_black(),0
	);
	lv_obj_set_style_shadow_width(
		con->toggle_btn,
		gui_font_size,0
	);
	lv_obj_set_style_size(con->toggle_btn,gui_font_size*3,0);
	lv_obj_align_to(
		con->toggle_btn,NULL,
		LV_ALIGN_BOTTOM_RIGHT,
		-gui_font_size,-gui_font_size
	);
	lv_obj_t*lbl=lv_label_create(con->toggle_btn);
	lv_label_set_text(lbl,LV_SYMBOL_KEYBOARD);
	lv_obj_center(lbl);
	return 0;
}

int console_resize(struct gui_activity*d){
	struct console*con=d->data;
	if(!con)return 0;
	lv_obj_set_pos(con->termview,0,0);
	lv_obj_set_size(con->termview,d->w,d->h);
	lv_termview_resize(con->termview);
	return 0;
}

int console_get_focus(struct gui_activity*d){
	size_t x,y;
	struct console*con=d->data;
	if(!con)return 0;
	lv_group_add_obj(gui_grp,con->termview);
	lv_group_add_obj(gui_grp,con->toggle_btn);
	if(lv_obj_is_visible(con->alt_btns))
		for(x=0;x<ARRLEN(btns);x++)for(y=0;y<ARRLEN(btns[x]);y++)
			lv_group_add_obj(gui_grp,con->btns[x][y].btn);
	lv_termview_update(con->termview);
	lv_group_focus_obj(con->termview);
	return 0;
}

int console_lost_focus(struct gui_activity*d){
	size_t x,y;
	struct console*con=d->data;
	if(!con)return 0;
	lv_group_remove_obj(con->termview);
	lv_group_remove_obj(con->toggle_btn);
	if(lv_obj_is_visible(con->alt_btns))
		for(x=0;x<ARRLEN(btns);x++)for(y=0;y<ARRLEN(btns[x]);y++)
			lv_group_remove_obj(con->btns[x][y].btn);
	return 0;
}

int console_clean(struct gui_activity*d){
	struct console*con=d->data;
	if(con->termview)lv_obj_remove_event_cb(con->termview,NULL);
	memset(con,0,sizeof(struct console));
	free(con);
	d->data=NULL;
	return 0;
}

static bool back_cb(
	uint16_t id,
	const char*name __attribute__((unused)),
	void*data
){
	struct console*con=data;
	if(id==0){
		con->allow_exit=true;
		guiact_do_back();
		guiact_do_back();
		return true;
	}
	return false;
}

int console_do_back(struct gui_activity*act){
	struct console*con=act->data;
	if(con->allow_exit)return 0;
	msgbox_set_user_data(msgbox_create_yesno(
		back_cb,
		"Do you want to exit terminal emulator?"
	),con);
	return 1;
}

#endif
#endif

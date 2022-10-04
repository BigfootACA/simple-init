/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"defines.h"
#include"gui/activity.h"

struct screen_test{
	struct gui_activity*act;
	bool text_top;
	lv_obj_t*text;
	lv_timer_t*timer;
	uint32_t color;
	bool init;
};

static void color_swap_timer(lv_timer_t*timer);
static void reinit_timer(struct screen_test*scr,bool create){
	if(scr->timer)lv_timer_del(scr->timer);
	scr->timer=NULL;
	if(create){
		scr->timer=lv_timer_create(
			color_swap_timer,
			5000,scr
		);
		lv_timer_set_repeat_count(scr->timer,1);
	}
}

static void move_text(struct screen_test*scr){
	lv_obj_align(
		scr->text,
		scr->text_top?LV_ALIGN_TOP_MID:LV_ALIGN_BOTTOM_MID,
		0,scr->text_top?gui_dpi/3:-gui_dpi/3
	);
}

static void color_swap(struct screen_test*scr){
	char*text;
	if(!scr->init){
		scr->init=true;
		scr->color=0x00FF0000,text="red";// RED
	}else if(scr->color==0x00FF0000)scr->color=0x0000FF00,text="green"; // RED    -> GREEN
	else if(scr->color==0x0000FF00)scr->color=0x000000FF,text="blue";   // GREEN  -> BLUE
	else if(scr->color==0x000000FF)scr->color=0x0000FFFF,text="cyan";   // BLUE   -> CYAN
	else if(scr->color==0x0000FFFF)scr->color=0x00FFFF00,text="yellow"; // CYAN   -> YELLOW
	else if(scr->color==0x00FFFF00)scr->color=0x00FF00FF,text="purple"; // YELLOW -> PURPLE
	else if(scr->color==0x00FF00FF)scr->color=0x00000000,text="black";  // PURPLE -> BLACK
	else if(scr->color==0x00000000)scr->color=0x00FFFFFF,text="white";  // BLACK  -> WHITE
	else if(scr->color==0x00FFFFFF)scr->color=0x007F7F7F,text="gray";   // WHITE  -> GRAY
	else{
		guiact_do_back();
		reinit_timer(scr,false);
		return;
	}
	lv_label_set_text(scr->text,_(text));
	move_text(scr);
	lv_obj_set_style_bg_color(
		scr->act->page,
		lv_color_hex(scr->color),0
	);
	reinit_timer(scr,true);
}

static void color_swap_timer(lv_timer_t*timer){
	if(!timer)return;
	struct screen_test*scr=timer->user_data;
	if(!scr||scr->timer!=timer)return;
	color_swap(scr);
}

static int scr_test_get_focus(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr)reinit_timer(scr,true);
	return 0;
}

static int scr_test_lost_focus(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr)reinit_timer(scr,false);
	return 0;
}

static void click_cb(lv_event_t*e){
	color_swap(e->user_data);
}

static void text_click_cb(lv_event_t*e){
	struct screen_test*scr=e->user_data;
	scr->text_top=!scr->text_top;
	move_text(scr);
}

static int scr_test_draw(struct gui_activity*act){
	struct screen_test*scr=malloc(sizeof(struct screen_test));
	if(!scr)return -ENOMEM;
	memset(scr,0,sizeof(struct screen_test));
	act->data=scr,scr->act=act;
	lv_obj_add_flag(act->page,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(act->page,click_cb,LV_EVENT_CLICKED,scr);
	scr->text=lv_label_create(act->page);
	lv_obj_add_flag(scr->text,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(scr->text,text_click_cb,LV_EVENT_CLICKED,scr);
	lv_obj_set_style_radius(scr->text,gui_font_size,0);
	lv_obj_set_style_bg_color(scr->text,lv_color_white(),0);
	lv_obj_set_style_bg_opa(scr->text,LV_OPA_100,0);
	lv_obj_set_style_pad_hor(scr->text,gui_font_size,0);
	lv_obj_set_style_pad_ver(scr->text,gui_font_size/2,0);
	lv_obj_set_style_shadow_color(scr->text,lv_palette_main(LV_PALETTE_GREY),0);
	lv_obj_set_style_shadow_width(scr->text,gui_font_size,0);
	color_swap(scr);
	return 0;
}

static int cleanup(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr->timer)lv_timer_del(scr->timer);
	free(scr);
	act->data=NULL;
	return 0;
}

struct gui_register guireg_screen_test={
	.name="screen-test",
	.title="Screen Color Test",
	.show_app=true,
	.full_screen=true,
	.quiet_exit=cleanup,
	.draw=scr_test_draw,
	.lost_focus=scr_test_lost_focus,
	.get_focus=scr_test_get_focus,
	.back=true,
};
#endif

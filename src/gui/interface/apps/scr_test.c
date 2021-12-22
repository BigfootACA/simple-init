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
	lv_task_t*task;
	uint32_t color;
	bool init;
};

static void color_swap_task(lv_task_t*task);
static void reinit_task(struct screen_test*scr,bool create){
	if(scr->task)lv_task_del(scr->task);
	scr->task=NULL;
	if(create){
		scr->task=lv_task_create(
			color_swap_task,
			5000,
			LV_TASK_PRIO_LOWEST,
			scr
		);
		lv_task_once(scr->task);
	}
}

static void color_swap(struct screen_test*scr){
	if(!scr->init){
		scr->init=true;
		scr->color=0x00FF0000;// RED
	}else if(scr->color==0x00FF0000)scr->color=0x0000FF00; // RED   -> GREEN
	else if(scr->color==0x0000FF00)scr->color=0x000000FF;  // GREEN -> BLUE
	else if(scr->color==0x000000FF)scr->color=0x00000000;  // BLUE  -> BLACK
	else if(scr->color==0x00000000)scr->color=0x00FFFFFF;  // BLACK -> WHITE
	else if(scr->color==0x00FFFFFF)scr->color=0x007F7F7F;  // WHITE -> GRAY
	else{
		guiact_do_back();
		return;
	}
	lv_obj_set_style_local_bg_color(
		scr->act->page,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		lv_color_hex(scr->color)
	);
	reinit_task(scr,true);
}

static void color_swap_task(lv_task_t*task){
	if(!task)return;
	struct screen_test*scr=task->user_data;
	if(!scr||scr->task!=task)return;
	color_swap(scr);
}

static int scr_test_get_focus(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr)reinit_task(scr,true);
	return 0;
}

static int scr_test_lost_focus(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr)reinit_task(scr,false);
	return 0;
}

static void click_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct screen_test*scr=lv_obj_get_user_data(obj);
	if(!scr||!scr->act||scr->act->page!=obj)return;
	color_swap(scr);
}

static int scr_test_draw(struct gui_activity*act){
	struct screen_test*scr=malloc(sizeof(struct screen_test));
	if(!scr)return -ENOMEM;
	memset(scr,0,sizeof(struct screen_test));
	act->data=scr,scr->act=act;
	lv_obj_set_click(act->page,true);
	lv_obj_set_user_data(act->page,scr);
	lv_obj_set_event_cb(act->page,click_cb);
	color_swap(scr);
	return 0;
}

static int cleanup(struct gui_activity*act){
	struct screen_test*scr=act->data;
	if(scr->task)lv_task_del(scr->task);
	free(scr);
	act->data=NULL;
	return 0;
}

struct gui_register guireg_screen_test={
	.name="screen-test",
	.title="Screen Color Test",
	.icon="screen.png",
	.show_app=true,
	.full_screen=true,
	.quiet_exit=cleanup,
	.draw=scr_test_draw,
	.lost_focus=scr_test_lost_focus,
	.get_focus=scr_test_get_focus,
	.back=true,
};
#endif

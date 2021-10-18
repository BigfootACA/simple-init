/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<pthread.h>
#include<sys/time.h>
#include"gui.h"
#include"logger.h"
#include"hardware.h"
#include"init_internal.h"
#define TAG "charger"

static lv_obj_t*bc,*bb,*bh,*bl,*bi;
static time_t last_charge;
static bool exited=false,charging=false;
static int level=-1;

static void set_battery_level(int lvl){
	if(lvl<0||lvl>100)return;
	lv_obj_set_height(bc,lv_obj_get_height(bb)*lvl/100);
	lv_obj_align(bc,bb,LV_ALIGN_IN_BOTTOM_MID,0,0);
	lv_label_set_text_fmt(bl,"%d%%",lvl);
}

static void draw_charger(){
	lv_obj_t*bg=lv_scr_act();
	lv_obj_set_style_local_bg_color(bg,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLACK);
	lv_obj_set_style_local_image_recolor(gui_cursor,LV_IMG_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);

	int line_size=gui_font_size/3*2;
	int bbw=gui_w/5*2,bbh=gui_h/5*2;
	int bhw=bbw/5*2,bhh=bbh/8;

	// battery body
	bb=lv_obj_create(bg,NULL);
	lv_obj_set_size(bb,bbw,bbh);
	lv_obj_align(bb,NULL,LV_ALIGN_CENTER,0,0);
	lv_theme_apply(bb,LV_THEME_SCR);
	lv_obj_set_style_local_radius(bb,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,5);
	lv_obj_set_style_local_bg_color(bb,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLACK);
	lv_obj_set_style_local_border_width(bb,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,line_size);
	lv_obj_set_style_local_border_color(bb,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);

	// battery head
	bh=lv_obj_create(bg,NULL);
	lv_obj_set_size(bh,bhw,bhh);
	lv_obj_align(bh,bb,LV_ALIGN_OUT_TOP_MID,0,line_size);
	lv_theme_apply(bh,LV_THEME_SCR);
	lv_obj_set_style_local_radius(bh,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,5);
	lv_obj_set_style_local_bg_color(bh,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);

	// battery content
	bc=lv_obj_create(bg,NULL);
	lv_obj_set_size(bc,bbw,0);
	lv_obj_align(bc,bb,LV_ALIGN_IN_BOTTOM_MID,0,0);
	lv_theme_apply(bc,LV_THEME_SCR);
	lv_obj_set_style_local_radius(bc,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,5);
	lv_obj_set_style_local_bg_color(bc,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);

	// battery level label
	bl=lv_label_create(bg,NULL);
	lv_label_set_long_mode(bl,LV_LABEL_LONG_CROP);
	lv_label_set_align(bl,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(bl,"???");
	lv_obj_set_width(bl,bbw);
	lv_obj_align(bl,bb,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_set_style_local_text_color(bl,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);

	// charging icon
	bi=lv_img_create(bg,NULL);
	lv_obj_set_size(bi,bbw/5,bbw/5);
	lv_obj_set_style_local_image_recolor(bi,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_WHITE);
	lv_img_set_src(bi,LV_SYMBOL_CHARGE);
}

static void battery_watch(lv_task_t*t __attribute__((unused))){
	if(level>0)set_battery_level(level);
	lv_obj_align(
		bi,NULL,
		LV_ALIGN_IN_BOTTOM_MID,
		0,charging?
			-gui_font_size:
			lv_obj_get_height(bi)
	);
}

static void battery_read(){
	time_t cur;
	time(&cur);
	int bats[64]={0};
	if(pwr_scan_device(bats,63,true)<0)return;
	level=pwr_multi_get_capacity(bats);
	charging=pwr_multi_is_charging(bats);
	pwr_close_device(bats);
	if(charging)last_charge=cur;
	else if(cur-last_charge>60&&!exited){
		exited=true;
		tlog_notice("not charge in 60secs, shutdown");
		struct init_msg msg;
		init_initialize_msg(&msg,ACTION_POWEROFF);
		struct init_msg response;
		errno=0;
		init_send(&msg,&response);
		if(errno!=0||response.data.status.ret!=0){
			if(errno==0)errno=response.data.status.ret;
			telog_warn("shutdown failed");
		}
	}
}

static _Noreturn void*battery_thread(void*d __attribute__((unused))){
	for(;;){
		sleep(5);
		battery_read();
	}
}

int charger_main(){
	gui_pre_init();
	gui_screen_init();
	draw_charger();
	time(&last_charge);
	pthread_t bt;
	pthread_create(&bt,NULL,battery_thread,NULL);
	battery_read();
	battery_watch(lv_task_create(
		battery_watch,
		5000,
		LV_TASK_PRIO_MID,
		NULL
	));
	gui_main();
	return 0;
}
#endif

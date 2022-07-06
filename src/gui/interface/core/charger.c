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
#include<sys/prctl.h>
#include"gui.h"
#include"logger.h"
#include"hardware.h"
#include"proctitle.h"
#include"init_internal.h"
#define TAG "charger"

static lv_obj_t*bc,*bb,*bh,*bl,*bi;
static time_t last_charge;
static bool exited=false,charging=false;
static int level=-1;

static void set_battery_level(int lvl){
	if(lvl<0||lvl>100)return;
	lv_obj_set_height(bc,lv_obj_get_height(bb)*lvl/100);
	lv_obj_align_to(bc,bb,LV_ALIGN_BOTTOM_MID,0,0);
	lv_label_set_text_fmt(bl,"%d%%",lvl);
}

static void draw_charger(){
	lv_obj_t*bg=lv_scr_act();
	lv_obj_set_style_bg_color(bg,lv_color_black(),0);
	lv_obj_set_style_img_recolor(gui_cursor,lv_color_white(),0);

	int line_size=gui_font_size/3*2;
	int bbw=gui_w/5*2,bbh=gui_h/5*2;
	int bhw=bbw/5*2,bhh=bbh/8;

	// battery body
	bb=lv_obj_create(bg);
	lv_obj_set_size(bb,bbw,bbh);
	lv_obj_align_to(bb,NULL,LV_ALIGN_CENTER,0,0);
	lv_obj_set_style_radius(bb,5,0);
	lv_obj_set_style_bg_color(bb,lv_color_black(),0);
	lv_obj_set_style_border_width(bb,line_size,0);
	lv_obj_set_style_border_color(bb,lv_color_white(),0);

	// battery head
	bh=lv_obj_create(bg);
	lv_obj_set_size(bh,bhw,bhh);
	lv_obj_align_to(bh,bb,LV_ALIGN_OUT_TOP_MID,0,line_size);
	lv_obj_set_style_radius(bh,5,0);
	lv_obj_set_style_bg_color(bh,lv_color_white(),0);

	// battery content
	bc=lv_obj_create(bg);
	lv_obj_set_size(bc,bbw,0);
	lv_obj_align_to(bc,bb,LV_ALIGN_BOTTOM_MID,0,0);
	lv_obj_set_style_radius(bc,5,0);
	lv_obj_set_style_bg_color(bc,lv_color_white(),0);

	// battery level label
	bl=lv_label_create(bg);
	lv_label_set_long_mode(bl,LV_LABEL_LONG_CLIP);
	lv_obj_set_style_text_align(bl,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(bl,"???");
	lv_obj_set_width(bl,bbw);
	lv_obj_align_to(bl,bb,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_set_style_text_color(bl,lv_color_white(),0);

	// charging icon
	bi=lv_img_create(bg);
	lv_obj_set_size(bi,bbw/5,bbw/5);
	lv_obj_set_style_img_recolor(bi,lv_color_white(),0);
	lv_img_set_src(bi,LV_SYMBOL_CHARGE);
}

static void battery_watch(lv_timer_t*t __attribute__((unused))){
	if(level>0)set_battery_level(level);
	lv_obj_align_to(
		bi,NULL,
		LV_ALIGN_BOTTOM_MID,
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
	prctl(PR_SET_NAME,"GUI Charger");
	setproctitle("charger");
	gui_pre_init();
	gui_screen_init();
	draw_charger();
	time(&last_charge);
	pthread_t bt;
	pthread_create(&bt,NULL,battery_thread,NULL);
	battery_read();
	battery_watch(lv_timer_create(
		battery_watch,
		5000,NULL
	));
	gui_main();
	return 0;
}
#endif

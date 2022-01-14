/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdlib.h>
#include<pthread.h>
#include"str.h"
#include"gui.h"
#include"defines.h"
#include"hardware.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/inputbox.h"
#include"gui/activity.h"

static volatile bool thread_run=false;
static pthread_t run_t;
static lv_obj_t*view;
static lv_obj_t*btn_prev,*btn_delete,*btn_next;
static lv_obj_t*btn_create,*btn_clean,*btn_start;

enum vibrate_type{
	TYPE_VIBRATE = 1,
	TYPE_IDLE    = 2,
};

struct vibrate_step{
	enum vibrate_type type;
	int time;
	lv_obj_t*btn,*chk;
};

static struct vibrate_step*selected;
static list*steps=NULL;
static pthread_mutex_t lock;

static int vibrator_get_focus(struct gui_activity*d __attribute__((unused))){
	pthread_mutex_lock(&lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(vs->chk)lv_group_add_obj(gui_grp,vs->chk);
	}while((o=o->next));
	pthread_mutex_unlock(&lock);
	lv_group_add_obj(gui_grp,btn_prev);
	lv_group_add_obj(gui_grp,btn_next);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_create);
	lv_group_add_obj(gui_grp,btn_clean);
	lv_group_add_obj(gui_grp,btn_start);
	return 0;
}

static int vibrator_lost_focus(struct gui_activity*d __attribute__((unused))){
	pthread_mutex_lock(&lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(vs->chk)lv_group_add_obj(gui_grp,vs->chk);
	}while((o=o->next));
	pthread_mutex_unlock(&lock);
	lv_group_remove_obj(btn_prev);
	lv_group_remove_obj(btn_next);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_create);
	lv_group_remove_obj(btn_clean);
	lv_group_remove_obj(btn_start);
	return 0;
}

static void chk_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected&&obj!=selected->chk){
		lv_checkbox_set_checked(selected->chk,false);
		lv_obj_set_checked(selected->btn,false);
	}
	selected=lv_obj_get_user_data(obj);
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_prev,true);
	lv_obj_set_enabled(btn_next,true);
	lv_obj_set_enabled(btn_delete,true);
}

static void clean_view(){
	pthread_mutex_lock(&lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(!vs->btn)continue;
		lv_obj_del(vs->btn);
		vs->btn=NULL,vs->chk=NULL;
	}while((o=o->next));
	pthread_mutex_unlock(&lock);
}

static void redraw_view(){
	char string[256]={0};
	lv_obj_t*last=NULL;
	lv_coord_t bw=lv_page_get_scrl_width(view);
	pthread_mutex_lock(&lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);

		memset(string,0,256);
		switch(vs->type){
			case TYPE_VIBRATE:snprintf(string,255,_("Vibrate %dms"),vs->time);break;
			case TYPE_IDLE:snprintf(string,255,_("Sleep %dms"),vs->time);break;
			default:continue;
		}

		// option select button
		vs->btn=lv_btn_create(view,NULL);
		lv_obj_set_size(vs->btn,bw,gui_font_size*3);
		lv_obj_align(
			vs->btn,last,
			last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
			0,last?gui_dpi/8:0
		);
		lv_style_set_btn_item(vs->btn);
		lv_obj_set_click(vs->btn,false);
		last=vs->btn;

		// line for button text
		lv_obj_t*line=lv_line_create(vs->btn,NULL);
		lv_obj_set_width(line,bw);

		// function name and checkbox
		vs->chk=lv_checkbox_create(line,NULL);
		lv_obj_set_user_data(vs->chk,vs);
		lv_obj_set_event_cb(vs->chk,chk_click);
		lv_checkbox_set_text(vs->chk,string);
		lv_style_set_focus_checkbox(vs->chk);
		lv_obj_align(vs->chk,NULL,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
		lv_checkbox_ext_t*e=lv_obj_get_ext_attr(vs->chk);

		lv_coord_t lbl_w=bw-gui_font_size*2-lv_obj_get_width(e->bullet);
		if(lv_obj_get_width(e->label)>lbl_w){
			lv_label_set_long_mode(e->label,LV_LABEL_LONG_CROP);
			lv_obj_set_width(e->label,lbl_w);
		}
	}while((o=o->next));
	pthread_mutex_unlock(&lock);
}

static bool create_time_cb(bool ok,const char*text __attribute__((unused)),void*user_data){
	char*end=NULL;
	struct vibrate_step*vs=user_data;
	if(!ok||!vs)return false;
	errno=0,vs->time=strtol(text,&end,0);
	if(errno!=0||end==text||vs->time<=0||vs->time>0xFFFF){
		msgbox_alert("Invalid time number");
		vs->time=0;
		return true;
	}
	pthread_mutex_lock(&lock);
	list_obj_add_new(&steps,vs);
	pthread_mutex_unlock(&lock);
	clean_view();
	redraw_view();
	return false;
}

static bool create_type_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	struct vibrate_step*vs=user_data;
	if(!vs)return false;
	switch(id){
		case 0:vs->type=TYPE_VIBRATE;break;
		case 1:vs->type=TYPE_IDLE;break;
		default:return true;
	}
	struct inputbox*in=inputbox_create(create_time_cb,"Input step time (ms)");
	inputbox_set_max_length(in,5);
	inputbox_set_user_data(in,vs);
	inputbox_set_content(in,"1000");
	inputbox_set_accept(in,NUMBER);
	return false;
}

static void create_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_create||e!=LV_EVENT_CLICKED)return;
	static const char*btns[]={"Vibrate","Sleep",""};
	struct vibrate_step*vs=malloc(sizeof(struct vibrate_step));
	if(!vs)return;
	memset(vs,0,sizeof(struct vibrate_step));
	struct msgbox*msg=msgbox_create_custom(create_type_cb,btns,"Select step type");
	msgbox_set_user_data(msg,vs);
}
static int _datafree(void*d){
	if(!d)return 0;
	memset(d,0,sizeof(struct vibrate_step));
	free(d);
	return 0;
}

static void clean_steps(){
	clean_view();
	lv_obj_set_enabled(btn_prev,false);
	lv_obj_set_enabled(btn_next,false);
	lv_obj_set_enabled(btn_delete,false);
	pthread_mutex_lock(&lock);
	list_free_all(steps,_datafree);
	steps=NULL,selected=NULL;
	pthread_mutex_unlock(&lock);
}

static bool clean_confirm_cb(
	uint16_t id,
	const char*text __attribute__((unused)),
	void*user_data __attribute__((unused))
){
	if(id==0)clean_steps();
	return false;
}

static void clean_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_clean||e!=LV_EVENT_CLICKED)return;
	if(list_count(steps)<=0)return;
	msgbox_create_yesno(clean_confirm_cb,"Clean all items?");
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	clean_steps();
	view=NULL;
	pthread_mutex_destroy(&lock);
	return 0;
}

static void delete_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_delete||e!=LV_EVENT_CLICKED||!selected)return;
	clean_view();
	pthread_mutex_lock(&lock);
	list_obj_del_data(&steps,selected,_datafree);
	pthread_mutex_unlock(&lock);
	selected=NULL;
	lv_obj_set_enabled(btn_prev,false);
	lv_obj_set_enabled(btn_next,false);
	lv_obj_set_enabled(btn_delete,false);
	redraw_view();
}

static void move_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED||!selected)return;
	int r;
	pthread_mutex_lock(&lock);
	list*l=list_lookup_data(steps,selected);
	pthread_mutex_unlock(&lock);
	if(!l)return;
	pthread_mutex_lock(&lock);
	if(obj==btn_prev)r=list_swap_prev(l);
	else if(obj==btn_next)r=list_swap_next(l);
	else r=-1;
	pthread_mutex_unlock(&lock);
	if(r!=0)return;
	lv_obj_t*scrl=lv_page_get_scrl(view);
	lv_coord_t y=lv_obj_get_y(scrl);
	clean_view();
	redraw_view();
	lv_obj_set_y(scrl,y);
	lv_scroll_to(selected->chk,false);
	chk_click(selected->chk,LV_EVENT_VALUE_CHANGED);
}

static void*worker_thread(void*data  __attribute__((unused))){
	for(;;){
		pthread_mutex_lock(&lock);
		int i=0;
		list*o=list_first(steps),*n;
		if(o)do{
			n=o->next;
			LIST_DATA_DECLARE(d,o,struct vibrate_step*);
			if(!d)continue;
			enum vibrate_type type=d->type;
			int time=d->time;
			pthread_mutex_unlock(&lock);
			i++;
			if(!thread_run)return NULL;
			switch(type){
				case TYPE_VIBRATE:vibrate(time);break;
				case TYPE_IDLE:usleep(time*1000);break;
			}
			pthread_mutex_lock(&lock);
		}while((o=n));
		pthread_mutex_unlock(&lock);
		if(i<=0)break;
	}
	thread_run=false;
	return NULL;
}

static void start_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_start||e!=LV_EVENT_CLICKED)return;
	if(!thread_run){
		if(list_count(steps)<=0){
			msgbox_alert("No any steps configured");
			return;
		}
		errno=0;
		vibrate(10);
		if(errno>0){
			msgbox_alert("Call vibrator failed: %m");
			return;
		}
		errno=0;
		if(pthread_create(&run_t,NULL,worker_thread,NULL)!=0){
			msgbox_alert("Create thread failed: %m");
			return;
		}
	}
	thread_run=!thread_run;
}

static int vibrator_draw(struct gui_activity*act){
	lv_obj_t*txt=lv_label_create(act->page,NULL);
	lv_label_set_text(txt,_("Vibrator Tester"));
	lv_obj_set_width(txt,gui_sw);
	lv_obj_align(txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);

	lv_coord_t btx=gui_font_size,bts=gui_sw/6-btx,btm=btx/2,bth=btx*2;

	view=lv_page_create(act->page,NULL);
	lv_obj_set_width(view,gui_sw-gui_font_size);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_align(view,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_set_height(view,gui_sh-bth-btx-lv_obj_get_y(view));

	btn_prev=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_prev,false);
	lv_obj_set_size(btn_prev,bts,bth);
	lv_obj_set_event_cb(btn_prev,move_cb);
	lv_obj_set_user_data(btn_prev,"move up");
	lv_obj_align(btn_prev,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(btn_prev,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_prev,NULL),LV_SYMBOL_UP);

	btn_next=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_next,false);
	lv_obj_set_size(btn_next,bts,bth);
	lv_obj_set_event_cb(btn_next,move_cb);
	lv_obj_set_user_data(btn_next,"move down");
	lv_obj_align(btn_next,btn_prev,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_next,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_next,NULL),LV_SYMBOL_DOWN);

	btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_delete,false);
	lv_obj_set_size(btn_delete,bts,bth);
	lv_obj_set_event_cb(btn_delete,delete_cb);
	lv_obj_set_user_data(btn_delete,"delete");
	lv_obj_align(btn_delete,btn_next,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_delete,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_delete,NULL),LV_SYMBOL_CLOSE);

	btn_create=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_create,bts,bth);
	lv_obj_set_event_cb(btn_create,create_cb);
	lv_obj_set_user_data(btn_create,"create");
	lv_obj_align(btn_create,btn_delete,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_create,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_create,NULL),LV_SYMBOL_PLUS);

	btn_clean=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_clean,bts,bth);
	lv_obj_set_event_cb(btn_clean,clean_cb);
	lv_obj_set_user_data(btn_clean,"clean");
	lv_obj_align(btn_clean,btn_create,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_clean,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_clean,NULL),LV_SYMBOL_TRASH);

	btn_start=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_start,bts,bth);
	lv_obj_set_event_cb(btn_start,start_cb);
	lv_obj_set_user_data(btn_start,"start/stop");
	lv_obj_align(btn_start,btn_clean,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_start,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_start,NULL),LV_SYMBOL_OK);

	pthread_mutex_init(&lock,NULL);
	return 0;
}

struct gui_register guireg_vibrator={
	.name="vibrator",
	.title="Vibrator Tester",
	.icon="vibrator.svg",
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=vibrator_draw,
	.lost_focus=vibrator_lost_focus,
	.get_focus=vibrator_get_focus,
	.back=true,
};
#endif

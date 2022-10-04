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
	lv_obj_t*btn,*lbl;
};

static struct vibrate_step*selected;
static list*steps=NULL;
static mutex_t lock;

static int vibrator_get_focus(struct gui_activity*d __attribute__((unused))){
	MUTEX_LOCK(lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(vs->btn)lv_group_add_obj(gui_grp,vs->btn);
	}while((o=o->next));
	MUTEX_UNLOCK(lock);
	lv_group_add_obj(gui_grp,btn_prev);
	lv_group_add_obj(gui_grp,btn_next);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_create);
	lv_group_add_obj(gui_grp,btn_clean);
	lv_group_add_obj(gui_grp,btn_start);
	return 0;
}

static int vibrator_lost_focus(struct gui_activity*d __attribute__((unused))){
	MUTEX_LOCK(lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(vs->btn)lv_group_remove_obj(vs->btn);
	}while((o=o->next));
	MUTEX_UNLOCK(lock);
	lv_group_remove_obj(btn_prev);
	lv_group_remove_obj(btn_next);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_create);
	lv_group_remove_obj(btn_clean);
	lv_group_remove_obj(btn_start);
	return 0;
}

static void item_check(lv_event_t*e){
	lv_obj_set_checked(e->target,true);
	if(selected&&e->target!=selected->btn)
		lv_obj_set_checked(selected->btn,false);
	selected=e->user_data;
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_prev,true);
	lv_obj_set_enabled(btn_next,true);
	lv_obj_set_enabled(btn_delete,true);
}

static void clean_view(){
	MUTEX_LOCK(lock);
	list*o=list_first(steps);
	if(o)do{
		LIST_DATA_DECLARE(vs,o,struct vibrate_step*);
		if(!vs->btn)continue;
		lv_obj_del(vs->btn);
		vs->btn=NULL,vs->lbl=NULL;
	}while((o=o->next));
	MUTEX_UNLOCK(lock);
}

static void redraw_view(){
	char string[256]={0};
	MUTEX_LOCK(lock);
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
		vs->btn=lv_btn_create(view);
		lv_obj_set_width(vs->btn,lv_pct(100));
		lv_obj_add_event_cb(vs->btn,item_check,LV_EVENT_CLICKED,vs);
		lv_style_set_btn_item(vs->btn);

		// function name
		vs->lbl=lv_label_create(vs->btn);
		lv_label_set_text(vs->lbl,string);
	}while((o=o->next));
	MUTEX_UNLOCK(lock);
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
	MUTEX_LOCK(lock);
	list_obj_add_new(&steps,vs);
	MUTEX_UNLOCK(lock);
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

static void create_cb(lv_event_t*e __attribute__((unused))){
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
	MUTEX_LOCK(lock);
	list_free_all(steps,_datafree);
	steps=NULL,selected=NULL;
	MUTEX_UNLOCK(lock);
}

static bool clean_confirm_cb(
	uint16_t id,
	const char*text __attribute__((unused)),
	void*user_data __attribute__((unused))
){
	if(id==0)clean_steps();
	return false;
}

static void clean_cb(lv_event_t*e __attribute__((unused))){
	if(list_count(steps)<=0)return;
	msgbox_create_yesno(clean_confirm_cb,"Clean all items?");
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	clean_steps();
	view=NULL;
	MUTEX_DESTROY(lock);
	return 0;
}

static void delete_cb(lv_event_t*e __attribute__((unused))){
	if(!selected)return;
	clean_view();
	MUTEX_LOCK(lock);
	list_obj_del_data(&steps,selected,_datafree);
	MUTEX_UNLOCK(lock);
	selected=NULL;
	lv_obj_set_enabled(btn_prev,false);
	lv_obj_set_enabled(btn_next,false);
	lv_obj_set_enabled(btn_delete,false);
	redraw_view();
}

static void move_cb(lv_event_t*e){
	if(!selected)return;
	int r;
	MUTEX_LOCK(lock);
	list*l=list_lookup_data(steps,selected);
	MUTEX_UNLOCK(lock);
	if(!l)return;
	MUTEX_LOCK(lock);
	if(e->target==btn_prev)r=list_swap_prev(l);
	else if(e->target==btn_next)r=list_swap_next(l);
	else r=-1;
	MUTEX_UNLOCK(lock);
	if(r!=0)return;
	clean_view();
	redraw_view();
	lv_obj_set_checked(selected->btn,true);
	lv_obj_scroll_to_view(selected->btn,false);
}

static void*worker_thread(void*data  __attribute__((unused))){
	for(;;){
		MUTEX_LOCK(lock);
		int i=0;
		list*o=list_first(steps),*n;
		if(o)do{
			n=o->next;
			LIST_DATA_DECLARE(d,o,struct vibrate_step*);
			if(!d)continue;
			enum vibrate_type type=d->type;
			int time=d->time;
			MUTEX_UNLOCK(lock);
			i++;
			if(!thread_run)return NULL;
			switch(type){
				case TYPE_VIBRATE:vibrate(time);break;
				case TYPE_IDLE:usleep(time*1000);break;
			}
			MUTEX_UNLOCK(lock);
		}while((o=n));
		MUTEX_UNLOCK(lock);
		if(i<=0)break;
	}
	thread_run=false;
	return NULL;
}

static void start_cb(lv_event_t*e __attribute__((unused))){
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
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	lv_obj_t*txt=lv_label_create(act->page);
	lv_obj_set_width(txt,lv_pct(100));
	lv_obj_set_style_pad_all(txt,gui_font_size,0);
	lv_obj_set_style_text_align(txt,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(txt,_("Vibrator Tester"));

	view=lv_obj_create(act->page);
	lv_obj_set_width(view,lv_pct(100));
	lv_obj_set_flex_flow(view,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(view,1);

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,cb,title,x)&(struct button_dsc){\
			&tgt,en,title,cb,NULL,x,1,0,1,NULL\
		}
		BTN(btn_prev,   false, move_cb,   LV_SYMBOL_UP,    0),
		BTN(btn_next,   false, move_cb,   LV_SYMBOL_DOWN,  1),
		BTN(btn_delete, false, delete_cb, LV_SYMBOL_CLOSE, 2),
		BTN(btn_create, true,  create_cb, LV_SYMBOL_PLUS,  3),
		BTN(btn_clean,  true,  clean_cb,  LV_SYMBOL_TRASH, 4),
		BTN(btn_start,  true,  start_cb,  LV_SYMBOL_OK,    5),
		NULL
		#undef BTN
	);

	MUTEX_INIT(lock);
	return 0;
}

struct gui_register guireg_vibrator={
	.name="vibrator",
	.title="Vibrator Tester",
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=vibrator_draw,
	.lost_focus=vibrator_lost_focus,
	.get_focus=vibrator_get_focus,
	.back=true,
};
#endif

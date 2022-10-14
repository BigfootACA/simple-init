/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"str.h"
#include"gui.h"
#include"logger.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/xmlrender.h"
#define TAG "activity"

static list*activities=NULL;
static list*registers=NULL;
static struct gui_activity*exclusive=NULL;

int guiact_do_exit(){
	gui_run=false;
	return 0;
}

static int guiact_force_free(void*data){
	struct gui_activity*act=(struct gui_activity*)data;
	if(!act)return 0;
	if(act->page)lv_obj_del(act->page);
	free(data);
	return 0;
}

static int guireg_force_free(void*data){
	struct gui_register*reg=(struct gui_register*)data;
	if(!reg)return 0;
	if(reg->xml)free(reg->xml);
	free(data);
	return 0;
}

extern struct gui_register*guiact_register[];
void guiact_init(){
	list_free_all(activities,guiact_force_free);
	list_free_all(registers,guireg_force_free);
	activities=NULL;
	registers=NULL;
	exclusive=NULL;
	for(int i=0;guiact_register[i];i++){
		int r=guiact_add_register(guiact_register[i]);
		if(r!=0)telog_warn(
			"add gui register %s failed",
			guiact_register[i]->name
		);
	}
	#ifdef ENABLE_MXML
	xml_rootfs_load_all_activity(_PATH_USR"/share/simple-init/apps");
	#endif
}

list*guiact_get_registers(){
	if(!registers)return NULL;
	if(registers->prev)
		registers=list_first(registers);
	if(!registers){
		tlog_error("cannot goto registers head");
		abort();
	}
	return registers;
}

list*guiact_get_activities(){
	if(!activities)return NULL;
	if(activities->prev)
		activities=list_first(activities);
	if(!activities){
		tlog_error("cannot goto activities head");
		abort();
	}
	return activities;
}

bool guiact_is_alone(){
	list*acts=guiact_get_activities();
	return !acts||list_is_alone(acts);
}

static list*guiact_get_last_list(){
	return list_last(guiact_get_activities());
}

struct gui_activity*guiact_get_last(){
	list*last=guiact_get_last_list();
	return last&&last->data?last->data:NULL;
}

static int call_lost_focus(struct gui_activity*act){
	if(!act)return -1;
	tlog_debug("%s lost focus",act->name);
	return act->reg->lost_focus?act->reg->lost_focus(act):-1;
}

static int call_lost_focus_list(list*lst){
	return lst?call_lost_focus(lst->data):-1;
}

static int call_lost_focus_last(){
	return call_lost_focus_list(guiact_get_last_list());
}

static int call_load_data(struct gui_activity*act){
	return act&&act->reg&&act->reg->data_load?act->reg->data_load(act):-1;
}

static int call_load_data_list(list*lst){
	return lst?call_load_data(lst->data):-1;
}

static int call_load_data_last(){
	return call_load_data_list(guiact_get_last_list());
}

static int call_unload_data(struct gui_activity*act){
	return act&&act->reg&&act->reg->data_unload?act->reg->data_unload(act):-1;
}

static int call_unload_data_list(list*lst){
	return lst?call_unload_data(lst->data):-1;
}

static int call_unload_data_last(){
	return call_unload_data_list(guiact_get_last_list());
}

static int call_get_focus(struct gui_activity*act){
	if(!act)return -1;
	if(act->w!=gui_sw||act->h!=gui_sh){
		act->w=gui_sw,act->h=gui_sh;
		if(act->reg->resize)act->reg->resize(act);
	}
	tlog_debug("%s get focus",act->name);
	return act->reg->get_focus?act->reg->get_focus(act):-1;
}

static int call_get_focus_list(list*lst){
	return lst?call_get_focus(lst->data):-1;
}

static int call_get_focus_last(){
	return call_get_focus_list(guiact_get_last_list());
}

static int call_ask_exit(struct gui_activity*act){
	if(!act)return -1;
	return act->reg->ask_exit?act->reg->ask_exit(act):0;
}

static int call_quiet_exit(struct gui_activity*act){
	if(!act)return -1;
	int r=act->reg->quiet_exit?act->reg->quiet_exit(act):0;
	#ifdef ENABLE_MXML
	if(act->data&&act->reg->draw==render_activity_draw){
		render_free(act->data);
		act->data=NULL;
	}
	#endif
	return r;
}

static void guiact_remove_last_list(){
	list_remove_free_def(guiact_get_last_list());
}

int guiact_remove_last(bool focus){
	struct gui_activity*l=guiact_get_last();
	if(guiact_is_alone()||!l)return 0;
	if(l==exclusive)ERET(EPERM);
	tlog_debug("end activity %s",l->name);
	sysbar_focus_input(NULL);
	call_lost_focus_last();
	call_unload_data_last();
	if(l->page)lv_obj_del(l->page);
	bool reload=!l->reg->mask||!l->mask||l->data_changed;
	bool fs=l->reg->full_screen;
	guiact_remove_last_list();
	l=guiact_get_last();
	if(fs&&l&&!l->reg->full_screen)sysbar_set_full_screen(false);
	if(focus){
		if(reload)call_load_data_last();
		call_get_focus_last();
	}
	return 0;
}

static void guiact_back_task(void*d __attribute__((unused))){
	if(sysbar.keyboard){
		sysbar_keyboard_close();
		return;
	}
	struct gui_activity*c=guiact_get_last();
	if(!c){
		guiact_do_exit();
		return;
	}
	if(c==exclusive)return;
	if(!c->reg->back)return;
	if(guiact_is_alone())return;
	tlog_debug("do back");
	if(call_ask_exit(c)!=0)return;
	if(call_quiet_exit(c)!=0)return;
	guiact_remove_last(true);
}

int guiact_do_back(){
	lv_async_call(guiact_back_task,NULL);
	return 0;
}

static void guiact_home_task(void*data __attribute__((unused))){
	sysbar_keyboard_close();
	list*d;
	bool proc=false;
	if(guiact_is_alone())return;
	while((d=guiact_get_last_list())&&d->prev){
		LIST_DATA_DECLARE(z,d,struct gui_activity*);
		if(z==exclusive)break;
		if(call_quiet_exit(z)!=0)return;
		guiact_remove_last(false);
		proc=true;
	}
	if(proc){
		call_load_data_last();
		call_get_focus_last();
	}
}

int guiact_do_home(){
	lv_async_call(guiact_home_task,NULL);
	return 0;
}

int guiact_set_exclusive(struct gui_activity*target){
	bool found=false;
	struct list*acts=guiact_get_activities(),*next,*cur;
	if(!target){
		if(exclusive)tlog_notice("exit exclusive mode from %s",exclusive->name);
		exclusive=NULL;
		return 0;
	}
	if(exclusive){
		tlog_warn("already have exclusive mode activity");
		ERET(EEXIST);
	}
	if((next=acts))do{
		cur=next;
		LIST_DATA_DECLARE(d,cur,struct gui_activity*);
		if(d==target)found=true;
	}while((next=cur->next));
	if(!found)ERET(ENOENT);
	tlog_notice("enter exclusive mode with %s",target->name);
	exclusive=target;
	return 0;
}

static int guiact_add_activity(struct gui_activity*act){
	list*e=list_new(act);
	if(!e)ERET(ENOMEM);
	if(activities){
		call_lost_focus_last();
		if(
			!act->reg->mask&&
			!act->mask&&
			!act->data_changed
		)call_unload_data_last();
		list_push(activities,e);
	}else activities=e;
	call_load_data_last();
	call_get_focus_last();
	tlog_debug(
		"add activity %s to stack, now have %d",
		act->name,list_count(activities)
	);
	return 0;
}

int guiact_add_register(struct gui_register*reg){
	if(!reg||!reg->name[0])ERET(EINVAL);
	if(guiact_find_register(reg->name))ERET(EEXIST);
	struct gui_register*dup=malloc(sizeof(struct gui_register));
	if(!dup)ERET(ENOMEM);
	memcpy(dup,reg,sizeof(struct gui_register));
	if(reg->xml&&!(dup->xml=strdup(reg->xml))){
		free(dup);
		ERET(ENOMEM);
	}
	return list_obj_add_new(&registers,dup);
}

int guiact_register_activity(struct gui_activity*act){
	if(!act)ERET(EINVAL);
	return guiact_add_activity(memdup(act,sizeof(struct gui_activity)));
}

static bool reg_cmp(list*l,void*name){
	LIST_DATA_DECLARE(reg,l,struct gui_register*);
	return reg&&name&&strcmp((char*)name,reg->name)==0;
}

struct gui_register*guiact_find_register(char*name){
	list*l;
	if(!name)EPRET(EINVAL);
	if(!(l=list_search_one(
		guiact_get_registers(),
		reg_cmp,(void*)name
	)))EPRET(ENOENT);
	return LIST_DATA(l,struct gui_register*);
}

struct guiact_data{
	struct gui_register*reg;
	void*args;
};

static void guiact_start_task(void*data){
	int r;
	struct guiact_data*d=data;
	if(!d)return;
	struct gui_register*reg=d->reg;
	void*args=d->args;
	free(d);
	if(!reg)return;
	struct gui_activity*act=malloc(sizeof(struct gui_activity));
	if(!act)return;
	memset(act,0,sizeof(struct gui_activity));
	act->reg=reg,act->args=args,act->mask=reg->mask;
	if(reg->full_screen)sysbar_set_full_screen(true);
	sysbar_keyboard_close();
	act->w=gui_sw,act->h=gui_sh;
	strcpy(act->name,reg->name);
	#ifdef ENABLE_MXML
	if(reg->xml){
		if(!(act->data=render_create(NULL)))
			EDONE(tlog_error("cannot allocate xml render"));
		if(!reg->draw)reg->draw=render_activity_draw;
		if(!reg->resize)reg->resize=render_activity_resize;
		if(!reg->get_focus)reg->get_focus=render_activity_get_focus;
		if(!reg->lost_focus)reg->lost_focus=render_activity_lost_focus;
	}
	#endif
	if(!reg->draw)EDONE(tlog_error("invalid activity"));
	if(reg->init&&(r=reg->init(act))<0)
		EDONE(tlog_warn("activity %s init failed: %d",act->name,r));
	act->page=lv_obj_create(sysbar.content);
	lv_obj_set_style_radius(act->page,0,0);
	lv_obj_set_style_pad_all(act->page,gui_font_size/2,0);
	lv_obj_set_style_border_width(act->page,0,0);
	if(act->mask){
		lv_obj_set_style_bg_color(act->page,lv_color_black(),0);
		lv_obj_set_style_bg_opa(act->page,LV_OPA_50,0);
	}
	lv_obj_set_size(act->page,act->w,act->h);
	lv_obj_set_pos(act->page,gui_sx,gui_sy);
	if((r=reg->draw(act))<0){
		if(r!=-10)tlog_warn("activity %s draw failed: %d",act->name,r);
		goto done;
	}
	if(reg->resize)reg->resize(act);
	guiact_add_activity(act);
	return;
	done:
	if(act){
		#ifdef ENABLE_MXML
		if(
			act->data&&
			reg->draw==render_activity_draw
		)render_free(act->data);
		#endif
		if(act->page)lv_obj_del(act->page);
		free(act);
	}
}

int guiact_start_activity(struct gui_register*reg,void*args){
	if(!reg)ERET(EINVAL);
	if(!reg->draw&&!reg->xml){
		tlog_warn("invalid activity %s",reg->name);
		ERET(EINVAL);
	}
	if(exclusive&&!reg->allow_exclusive){
		tlog_warn("target activity not allow in exclusive mode");
		ERET(EPERM);
	}
	struct guiact_data*d=malloc(sizeof(struct guiact_data));
	if(!d)ERET(ENOMEM);
	memset(d,0,sizeof(struct guiact_data));
	d->reg=reg,d->args=args;
	lv_async_call(guiact_start_task,d);
	return 0;
}

int guiact_start_activity_by_name(char*name,void*args){
	struct gui_register*reg=guiact_find_register(name);
	if(!reg){
		tlog_warn("activity %s not found",name);
		ERET(ENOENT);
	}
	return guiact_start_activity(reg,args);
}

bool guiact_is_name(struct gui_activity*act,const char*name){
	return act&&name&&strcmp(act->name,name)==0;
}

bool guiact_is_page(struct gui_activity*act,lv_obj_t*page){
	return act&&page&&act->page==page;
}

bool guiact_has_activity_name(const char*name){
	struct list*acts=guiact_get_activities(),*next,*cur;
	if((next=acts))do{
		cur=next;
		LIST_DATA_DECLARE(d,cur,struct gui_activity*);
		if(guiact_is_name(d,name))return true;
	}while((next=cur->next));
	return false;
}

bool guiact_has_activity_page(lv_obj_t*page){
	struct list*acts=guiact_get_activities(),*next,*cur;
	if((next=acts))do{
		cur=next;
		LIST_DATA_DECLARE(d,cur,struct gui_activity*);
		if(guiact_is_page(d,page))return true;
	}while((next=cur->next));
	return false;
}

bool guiact_is_active_name(const char*name){
	return guiact_is_name(guiact_get_last(),name);
}

bool guiact_is_active_page(lv_obj_t*page){
	return guiact_is_page(guiact_get_last(),page);
}
#endif

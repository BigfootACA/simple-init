#ifdef ENABLE_GUI
#include<stdlib.h>
#include"str.h"
#include"gui.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#define TAG "activity"

static list*activities=NULL;

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

void guiact_init(){
	list_free_all(activities,guiact_force_free);
	activities=NULL;
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

static int call_get_focus(struct gui_activity*act){
	if(!act)return -1;
	tlog_debug("%s get focus",act->name);
	return act->reg->get_focus?act->reg->get_focus(act):-1;
}

static int call_get_focus_list(list*lst){
	return lst?call_get_focus(lst->data):-1;
}

static int call_get_focus_last(){
	return call_get_focus_list(guiact_get_last_list());
}

static void guiact_remove_last_list(){
	list_remove_free_def(guiact_get_last_list());
}

int guiact_remove_last(){
	struct gui_activity*l=guiact_get_last();
	if(guiact_is_alone()||!l)return 0;
	tlog_debug("end activity %s",l->name);
	call_lost_focus_last();
	if(l->page)lv_obj_del_async(l->page);
	guiact_remove_last_list();
	call_get_focus_last();
	return 0;
}

int guiact_do_back(){
	if(sysbar.keyboard){
		sysbar_keyboard_close();
		return 0;
	}
	struct gui_activity*c=guiact_get_last();
	if(!c)return guiact_do_exit();
	if(!c->reg->back)return 0;
	if(guiact_is_alone())return 0;
	tlog_debug("do back");
	if(c->reg->ask_exit&&c->reg->ask_exit(c)!=0)return 0;
	if(c->reg->quiet_exit&&c->reg->quiet_exit(c)!=0)return 0;
	return guiact_remove_last();
}

int guiact_do_home(){
	sysbar_keyboard_close();
	list*acts=guiact_get_activities(),*d;
	if(!acts||list_is_alone(acts))return 0;
	LIST_DATA_DECLARE(c,list_last(acts),struct gui_activity*);
	if(c->reg->back&&c->reg->ask_exit)c->reg->ask_exit(c);
	if(c->reg->quiet_exit)c->reg->quiet_exit(c);
	guiact_remove_last();
	while((d=list_last(acts))&&d->prev){
		LIST_DATA_DECLARE(z,d,struct gui_activity*);
		if(z->reg->quiet_exit)z->reg->quiet_exit(c);
		guiact_remove_last();
	}
	return 0;
}

static int guiact_add_activity(struct gui_activity*act){
	list*e=list_new(act);
	if(!e)ERET(ENOMEM);
	if(activities){
		call_lost_focus_last();
		list_push(activities,e);
	}else activities=e;
	call_get_focus_last();
	tlog_debug(
		"add activity %s to stack, now have %d",
		act->name,list_count(activities)
	);
	return 0;
}

int guiact_register_activity(struct gui_activity*act){
	if(!act)ERET(EINVAL);
	return guiact_add_activity(memdup(act,sizeof(struct gui_activity)));
}

struct gui_register*guiact_find_register(char*name){
	if(!name)EPRET(EINVAL);
	struct gui_register*reg=NULL;
	for(int i=0;(reg=guiact_register[i]);i++){
		if(strcmp(name,reg->name)!=0)continue;
		return reg;
	}
	EPRET(ENOENT);
}

int guiact_start_activity(struct gui_register*reg,void*args){
	int r;
	if(!reg->draw){
		tlog_warn("invalid activity %s",reg->name);
		ERET(EINVAL);
	}
	struct gui_activity*act=malloc(sizeof(struct gui_activity));
	if(!act)ERET(ENOMEM);
	act->reg=reg,act->args=args,act->mask=reg->mask;
	strcpy(act->name,reg->name);
	if(reg->init&&(r=reg->init(act))<0){
		tlog_warn("activity %s init failed: %d",act->name,r);
		free(act);
		return r;
	}
	if(act->mask){
		act->page=lv_objmask_create(sysbar.content,NULL);
		lv_obj_add_style(act->page,LV_OBJMASK_PART_MAIN,lv_style_opa_mask());
	}else{
		act->page=lv_obj_create(sysbar.content,NULL);
		lv_theme_apply(act->page,LV_THEME_SCR);
	}
	lv_obj_set_size(act->page,gui_sw,gui_sh);
	lv_obj_set_pos(act->page,gui_sx,gui_sy);
	if((r=reg->draw(act))<0){
		tlog_warn("activity %s draw failed: %d",act->name,r);
		lv_obj_del(act->page);
		free(act);
		return r;
	}
	return guiact_add_activity(act);
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

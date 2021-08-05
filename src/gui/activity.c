#include<stdlib.h>
#include"str.h"
#include"lvgl.h"
#include"gui.h"
#include"logger.h"
#include"sysbar.h"
#include"activity.h"
#define TAG "activity"

static list*activities=NULL;

int guiact_do_exit(){
	gui_run=false;
	return 0;
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
static void guiact_remove_last_list(){
	list_remove_free_def(guiact_get_last_list());
}

int guiact_remove_last(){
	struct gui_activity*l=guiact_get_last();
	if(guiact_is_alone()||!l)return 0;
	tlog_debug("end activity %s",l->name);
	if(l->page)lv_obj_del_async(l->page);
	guiact_remove_last_list();
	return 0;
}

int guiact_do_back(){
	if(sysbar.keyboard){
		sysbar_keyboard_close();
		return 0;
	}
	struct gui_activity*c=guiact_get_last();
	if(!c)return guiact_do_exit();
	tlog_debug("do back");
	if(!c->back)return 0;
	if(guiact_is_alone())return 0;
	if(c->ask_exit&&c->ask_exit(NULL)!=0)return 0;
	if(c->quiet_exit&&c->quiet_exit(NULL)!=0)return 0;
	return guiact_remove_last();
}

int guiact_do_home(){
	sysbar_keyboard_close();
	list*acts=guiact_get_activities(),*d;
	if(!acts||list_is_alone(acts))return 0;
	LIST_DATA_DECLARE(c,list_last(acts),struct gui_activity*);
	if(c->back&&c->ask_exit)c->ask_exit(NULL);
	if(c->quiet_exit)c->quiet_exit(NULL);
	guiact_remove_last();
	while((d=list_last(acts))&&d->prev){
		LIST_DATA_DECLARE(z,d,struct gui_activity*);
		if(z->quiet_exit)z->quiet_exit(NULL);
		guiact_remove_last();
	}
	return 0;
}

int guiact_register_activity(struct gui_activity*act){
	if(!act)ERET(EINVAL);
	list*e=list_new_dup(act,sizeof(struct gui_activity));
	if(!e)ERET(ENOMEM);
	if(activities)list_push(activities,e);
	else activities=e;
	tlog_debug(
		"add activity %s to stack, now have %d",
		act->name,list_count(activities)
	);
	return 0;
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

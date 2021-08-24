#ifndef _ACTIVITY_H
#define _ACTIVITY_H
#include"list.h"
#include"lvgl.h"
#include"gui.h"
struct gui_register;
struct gui_activity;
typedef int guiact_base(struct gui_activity*);
struct gui_register{
	char name[256];
	bool back;
	bool mask;
	guiact_base*ask_exit;
	guiact_base*quiet_exit;
	guiact_base*get_focus;
	guiact_base*lost_focus;
	guiact_base*init;
	guiact_base*draw;
};
struct gui_activity{
	char name[256];
	void*args;
	void*data;
	bool mask;
	lv_obj_t*page;
	struct gui_register*reg;
};
extern int guiact_do_exit();
extern list*guiact_get_activities();
extern bool guiact_is_alone();
extern struct gui_activity*guiact_get_last();
extern int guiact_remove_last();
extern int guiact_do_back();
extern int guiact_do_home();
extern struct gui_register*guiact_find_register(char*name);
extern int guiact_start_activity(struct gui_register*reg,void*args);
extern int guiact_start_activity_by_name(char*name,void*args);
extern int guiact_register_activity(struct gui_activity*act);
extern bool guiact_is_name(struct gui_activity*act,const char*name);
extern bool guiact_is_page(struct gui_activity*act,lv_obj_t*page);
extern bool guiact_has_activity_name(const char*name);
extern bool guiact_has_activity_page(lv_obj_t*page);
extern bool guiact_is_active_name(const char*name);
extern bool guiact_is_active_page(lv_obj_t*page);
#endif

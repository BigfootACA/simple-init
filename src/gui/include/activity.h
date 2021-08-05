#ifndef _ACTIVITY_H
#define _ACTIVITY_H
#include"list.h"
struct gui_activity{
	char name[256];
	bool back;
	runnable_t*ask_exit;
	runnable_t*quiet_exit;
	lv_obj_t*page;
};
extern int guiact_do_exit();
extern list*guiact_get_activities();
extern bool guiact_is_alone();
extern struct gui_activity*guiact_get_last();
extern int guiact_remove_last();
extern int guiact_do_back();
extern int guiact_do_home();
extern int guiact_register_activity(struct gui_activity*act);
extern bool guiact_has_activity_name(const char*name);
extern bool guiact_has_activity_page(lv_obj_t*page);
#endif

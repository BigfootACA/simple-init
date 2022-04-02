/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H
#include"gui.h"
#include"list.h"
struct gui_register;
struct gui_activity;
typedef int guiact_base(struct gui_activity*);
struct gui_register{
	char name[256];
	char title[512];
	char icon[256];
	char**open_regex;
	char*xml;
	bool show_app;
	bool back;
	bool mask;
	bool open_file;
	bool full_screen;
	bool allow_exclusive;
	guiact_base*ask_exit;
	guiact_base*quiet_exit;
	guiact_base*get_focus;
	guiact_base*lost_focus;
	guiact_base*init;
	guiact_base*draw;
	guiact_base*resize;
	guiact_base*data_load;
	guiact_base*data_unload;
};
struct gui_activity{
	char name[256];
	void*args;
	void*data;
	bool mask;
	bool data_changed;
	lv_coord_t w,h;
	lv_obj_t*page;
	struct gui_register*reg;
};

// src/gui/activity.c: init activity manager
extern void guiact_init(void);

// src/gui/activity.c: do activity exit
extern int guiact_do_exit(void);

// src/gui/activity.c: get activity list
extern list*guiact_get_activities(void);

// src/gui/activity.c: get register list
extern list*guiact_get_registers(void);

// src/gui/activity.c: only have one activity
extern bool guiact_is_alone(void);

// src/gui/activity.c: get last activity in activity list
extern struct gui_activity*guiact_get_last(void);

// src/gui/activity.c: remove last activity in activity list
extern int guiact_remove_last(bool focus);

// src/gui/activity.c: call activity manager go back action
extern int guiact_do_back(void);

// src/gui/activity.c: call activity manager go home action
extern int guiact_do_home(void);

// src/gui/activity.c: get activity register by name
extern struct gui_register*guiact_find_register(char*name);

// src/gui/activity.c: start a new activity by register with arguments
extern int guiact_start_activity(struct gui_register*reg,void*args);

// src/gui/activity.c: start a new activity by name with arguments
extern int guiact_start_activity_by_name(char*name,void*args);

// src/gui/activity.c: add register to register list
extern int guiact_add_register(struct gui_register*reg);

// src/gui/activity.c: add activity to activity list
extern int guiact_register_activity(struct gui_activity*act);

// src/gui/activity.c: check activity name is equals
extern bool guiact_is_name(struct gui_activity*act,const char*name);

// src/gui/activity.c: check activity page is equals
extern bool guiact_is_page(struct gui_activity*act,lv_obj_t*page);

// src/gui/activity.c: check name is in activity list
extern bool guiact_has_activity_name(const char*name);

// src/gui/activity.c: check page is in activity list
extern bool guiact_has_activity_page(lv_obj_t*page);

// src/gui/activity.c: check active activity name is equals
extern bool guiact_is_active_name(const char*name);

// src/gui/activity.c: check active activity page is equals
extern bool guiact_is_active_page(lv_obj_t*page);
#endif

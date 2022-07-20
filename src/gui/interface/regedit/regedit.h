/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#ifndef REGEDIT_H
#define REGEDIT_H
#include<hivex.h>
#include"gui.h"
#include"gui/activity.h"
#include"list.h"
extern struct gui_register guireg_regedit;
extern struct gui_register guireg_regedit_value;
struct regedit{
	bool changed;
	lv_obj_t*view,*scr,*info,*lbl_path,*last_btn;
	lv_obj_t*btn_add,*btn_reload,*btn_delete,*btn_edit,*btn_home,*btn_load,*btn_save;
	list*path,*items;
	hive_h*hive;
	hive_node_h root,node;
};
struct regedit_value{
	struct regedit*reg;
	hive_h*hive;
	hive_node_h node;
	hive_value_h value;
};
extern const char*hivex_type_to_string(hive_type type);
extern char*hivex_value_to_string(char*buf,size_t len,hive_h*h,hive_value_h val);
#endif
#endif
#endif

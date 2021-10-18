/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FILEMGR_H
#define _FILEMGR_H
#include<stdbool.h>
#include"defines.h"
#include"gui.h"
#include"gui/fsext.h"
struct filetab;
typedef void(*filetab_on_item_select)(struct filetab*,char*item,enum item_type type,bool checked,uint16_t cnt);
typedef bool(*filetab_on_item_click)(struct filetab*,char*item,enum item_type type);
typedef void(*filetab_on_change_dir)(struct filetab*,char*old,char*new);
extern struct filetab*filetab_create(lv_obj_t*view,char*path);
extern void filetab_add_group(struct filetab*tab,lv_group_t*grp);
extern void filetab_remove_group(struct filetab*tab);
extern void filetab_free(struct filetab*tab);;
extern uint16_t filetab_get_id(struct filetab*tab);
extern lv_obj_t*filetab_get_tab(struct filetab*tab);
extern char*filetab_get_path(struct filetab*tab);
extern char*filetab_get_lvgl_path(struct filetab*tab);
extern char**filetab_get_checked(struct filetab*tab);
extern uint16_t filetab_get_checked_count(struct filetab*tab);
extern void filetab_set_path(struct filetab*tab,char*path);
extern void filetab_set_show_parent(struct filetab*tab,bool parent);
extern bool filetab_is_active(struct filetab*tab);
extern bool filetab_is_top(struct filetab*tab);
extern void filetab_go_back(struct filetab*tab);
extern void filetab_set_on_item_select(struct filetab*tab,filetab_on_item_select cb);
extern void filetab_set_on_item_click(struct filetab*tab,filetab_on_item_click cb);
extern void filetab_set_on_change_dir(struct filetab*tab,filetab_on_change_dir cb);
#endif

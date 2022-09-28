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
#include"filesystem.h"
#include"defines.h"
#include"gui.h"
struct filetab;
typedef void(*filetab_on_item_select)(struct filetab*,char*item,fs_type type,bool checked,uint16_t cnt);
typedef bool(*filetab_on_item_click)(struct filetab*,char*item,fs_type type);
typedef void(*filetab_on_change_dir)(struct filetab*,url*old,url*new);

// src/gui/interface/filemgr/filetab.c: create file tab
extern struct filetab*filetab_create(lv_obj_t*view,char*path);

// src/gui/interface/filemgr/filetab.c: add filetab and children to group
extern void filetab_add_group(struct filetab*tab,lv_group_t*grp);

// src/gui/interface/filemgr/filetab.c: remove filetab and children from group
extern void filetab_remove_group(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: release filetab
extern void filetab_free(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab id
extern uint16_t filetab_get_id(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab object
extern lv_obj_t*filetab_get_tab(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab path
extern char*filetab_get_path(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab fs handler
extern fsh*filetab_get_fsh(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab checked items
extern char**filetab_get_checked(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: get filetab checked items count
extern uint16_t filetab_get_checked_count(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: set filetab path and refresh
extern void filetab_set_path(struct filetab*tab,char*path);

// src/gui/interface/filemgr/filetab.c: set filetab show parent item
extern void filetab_set_show_parent(struct filetab*tab,bool parent);

// src/gui/interface/filemgr/filetab.c: is filetab active in tabview
extern bool filetab_is_active(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: is filetab in folder top
extern bool filetab_is_top(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: filetab go back
extern void filetab_go_back(struct filetab*tab);

// src/gui/interface/filemgr/filetab.c: set items select callback
extern void filetab_set_on_item_select(struct filetab*tab,filetab_on_item_select cb);

// src/gui/interface/filemgr/filetab.c: set item click callback
extern void filetab_set_on_item_click(struct filetab*tab,filetab_on_item_click cb);

// src/gui/interface/filemgr/filetab.c: set change dir callback
extern void filetab_set_on_change_dir(struct filetab*tab,filetab_on_change_dir cb);
#endif

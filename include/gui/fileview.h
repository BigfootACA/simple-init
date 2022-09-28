/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FILEVIEW_H
#define _FILEVIEW_H
#include<stdbool.h>
#include"gui.h"
#include"filesystem.h"
struct fileview;
typedef void(*fileview_on_item_select)(struct fileview*,char*item,fs_type type,bool checked,uint16_t cnt);
typedef bool(*fileview_on_item_click)(struct fileview*,char*item,fs_type type);
typedef void(*fileview_on_change_dir)(struct fileview*,url*old,url*new);

// src/gui/interface/filemgr/fileview.c: create fileview
extern struct fileview*fileview_create(lv_obj_t*screen);

// src/gui/interface/filemgr/fileview.c: add fileview and children to group
extern void fileview_add_group(struct fileview*view,lv_group_t*grp);

// src/gui/interface/filemgr/fileview.c: remove fileview and children from group
extern void fileview_remove_group(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: set fileview user data
extern void fileview_set_data(struct fileview*view,void*data);

// src/gui/interface/filemgr/fileview.c: set fileview path use url
extern void fileview_set_url(struct fileview*view,url*u);

// src/gui/interface/filemgr/fileview.c: set fileview path and refresh
extern void fileview_set_path(struct fileview*view,char*path);

// src/gui/interface/filemgr/fileview.c: set fileview show verbose info
extern void fileview_set_verbose(struct fileview*view,bool verbose);

// src/gui/interface/filemgr/fileview.c: set fileview show parent item
extern void fileview_set_show_parent(struct fileview*view,bool parent);

// src/gui/interface/filemgr/fileview.c: set change dir callback
extern void fileview_set_on_change_dir(struct fileview*view,fileview_on_change_dir cb);

// src/gui/interface/filemgr/fileview.c: set item click callback
extern void fileview_set_on_item_click(struct fileview*view,fileview_on_item_click cb);

// src/gui/interface/filemgr/fileview.c: set items select callback
extern void fileview_set_on_item_select(struct fileview*view,fileview_on_item_select cb);

// src/gui/interface/filemgr/filetab.c: get filetab checked items count
extern uint16_t fileview_get_checked_count(struct fileview*view);

// src/gui/interface/filemgr/filetab.c: get filetab checked items
extern char**fileview_get_checked(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: set fileview user data
extern void*fileview_get_data(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: get fileview path
extern char*fileview_get_path(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: get fileview fs handler
extern fsh*fileview_get_fsh(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: is fileview in folder top
extern bool fileview_is_top(struct fileview*view);

// src/gui/interface/filemgr/fileview.c: click an item in fileview
extern bool fileview_click_item(struct fileview*view,const char*name);

// src/gui/interface/filemgr/fileview.c: check an item in fileview
extern bool fileview_check_item(struct fileview*view,const char*name,bool checked);

// src/gui/interface/filemgr/fileview.c: fileview go back
extern void fileview_go_back(struct fileview*fv);

// src/gui/interface/filemgr/fileview.c: release fileview
extern void fileview_free(struct fileview*view);
#endif

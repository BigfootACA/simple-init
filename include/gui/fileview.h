#ifndef _FILEVIEW_H
#define _FILEVIEW_H
#include<stdbool.h>
#include"gui.h"
#include"gui/fsext.h"
struct fileview;
typedef bool(*fileview_on_item_click)(struct fileview*,char*item,enum item_type type);
typedef void(*fileview_on_change_dir)(struct fileview*,char*old,char*new);
extern struct fileview*fileview_create(lv_obj_t*screen);
extern void fileview_add_group(struct fileview*view,lv_group_t*grp);
extern void fileview_remove_group(struct fileview*view);
extern void fileview_set_data(struct fileview*view,void*data);
extern void fileview_set_path(struct fileview*view,char*path);
extern void fileview_set_show_parent(struct fileview*view,bool parent);
extern void fileview_set_on_change_dir(struct fileview*view,fileview_on_change_dir cb);
extern void fileview_set_on_item_click(struct fileview*view,fileview_on_item_click cb);
extern void*fileview_get_data(struct fileview*view);
extern char*fileview_get_path(struct fileview*view);
extern char*fileview_get_lvgl_path(struct fileview*view);
extern bool fileview_is_top(struct fileview*view);
extern void fileview_go_back(struct fileview*fv);
extern void fileview_free(struct fileview*view);
#endif

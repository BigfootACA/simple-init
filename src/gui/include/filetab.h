#ifndef FILEMGR_H
#define FILEMGR_H
#include"defines.h"
struct filetab;
typedef bool(*filetab_on_item_click)(struct filetab*,char*item,bool dir);
typedef void(*filetab_on_change_dir)(struct filetab*,char*old,char*new);
extern struct filetab*filetab_create(lv_obj_t*view,char*path);
extern void filetab_add_group(struct filetab*tab,lv_group_t*grp);
extern void filetab_remove_group(struct filetab*tab);
extern void filetab_free(struct filetab*tab);;
extern uint16_t filetab_get_id(struct filetab*tab);
extern lv_obj_t*filetab_get_tab(struct filetab*tab);
extern char*filetab_get_path(struct filetab*tab);
extern void filetab_set_path(struct filetab*tab,char*path);
extern void filetab_set_show_parent(struct filetab*tab,bool parent);
extern bool filetab_is_active(struct filetab*tab);
extern bool filetab_is_top(struct filetab*tab);
extern void filetab_go_back(struct filetab*tab);
extern void filetab_set_on_item_click(struct filetab*tab,filetab_on_item_click cb);
extern void filetab_set_on_change_dir(struct filetab*tab,filetab_on_change_dir cb);
#endif

#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#ifndef REGEDIT_H
#define REGEDIT_H
#include<hivex.h>
#include"gui.h"
#include"list.h"
struct regedit{
	bool changed;
	lv_obj_t*view,*scr,*info,*lbl_path,*last_btn;
	lv_obj_t*btn_add,*btn_reload,*btn_delete,*btn_edit,*btn_home,*btn_load,*btn_save;
	list*path,*items;
	lv_coord_t bm,bw,bh,si;
	lv_style_t img_s;
	hive_h*hive;
	hive_node_h root,node;
};
extern const char*hivex_type_to_string(hive_type type);
extern char*hivex_value_to_string(char*buf,size_t len,hive_h*h,hive_value_h val);
#endif
#endif
#endif

#ifndef _FSEXT_H
#define _FSEXT_H
#include"gui.h"
enum item_type{
	TYPE_DISK=1,
	TYPE_FILE,
	TYPE_DIR,
	TYPE_BLOCK,
	TYPE_CHAR,
	TYPE_SOCK,
	TYPE_LINK,
	TYPE_FIFO,
};
struct fsext{
	void*user_data;
	lv_res_t(*get_type_cb)(struct _lv_fs_drv_t*,const char*,enum item_type*);
	lv_res_t(*get_volume_label)(struct _lv_fs_drv_t*,char*,size_t);
	bool(*is_dir_cb)(struct _lv_fs_drv_t*,const char*);
};
extern bool fsext_is_multi;
extern lv_res_t lv_fs_get_type(enum item_type*type,const char*path);
extern lv_res_t lv_fs_get_volume_label(lv_fs_drv_t*drv,char*label,size_t len);
extern bool lv_fs_is_dir(const char*path);
#endif

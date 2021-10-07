#include"lvgl.h"
#include"array.h"
#include"defines.h"

static const char*str_lv_fs_res[]={
	[LV_FS_RES_OK]         = "Normal",
	[LV_FS_RES_HW_ERR]     = "Hardware error",
	[LV_FS_RES_FS_ERR]     = "Filesystem structure error",
	[LV_FS_RES_NOT_EX]     = "Not exists",
	[LV_FS_RES_FULL]       = "Disk full",
	[LV_FS_RES_LOCKED]     = "File locked",
	[LV_FS_RES_DENIED]     = "Access denied",
	[LV_FS_RES_BUSY]       = "Filesystem busy",
	[LV_FS_RES_TOUT]       = "Process timeout",
	[LV_FS_RES_NOT_IMP]    = "Not implemented",
	[LV_FS_RES_OUT_OF_MEM] = "Out of memory",
	[LV_FS_RES_INV_PARAM]  = "Invalid parameter",
	[LV_FS_RES_UNKNOWN]    = "Unknown error",
};

const char*lv_fs_res_to_string(lv_fs_res_t res){
	return res>=ARRLEN(str_lv_fs_res)?NULL:str_lv_fs_res[res];
}

const char*lv_fs_res_to_i18n_string(lv_fs_res_t res){
	return res>=ARRLEN(str_lv_fs_res)?NULL:_(str_lv_fs_res[res]);
}

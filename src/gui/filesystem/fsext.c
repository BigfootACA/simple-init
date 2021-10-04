#include"defines.h"
#include"lvgl.h"
#include"fsext.h"

static const char*lv_fs_get_real_path(const char*path){
	path++;
	while(*path)switch(*path){
		case ':':case '\\':case '/':path++;break;
		default:return path;
	}
	return path;
}

lv_res_t lv_fs_get_type(enum item_type*type,const char*path){
	if(!path||!type)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->get_type_cb)return LV_FS_RES_NOT_IMP;
	return fse->get_type_cb(drv,lv_fs_get_real_path(path),type);
}

lv_res_t lv_fs_get_volume_label(lv_fs_drv_t*drv,char*label,size_t len){
	if(!drv||!label)return LV_FS_RES_INV_PARAM;
	memset(label,0,len);
	strncpy(label,_("Unknown"),len-1);
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->get_volume_label)return LV_FS_RES_NOT_IMP;
	return fse->get_volume_label(drv,label,len);

}

bool lv_fs_is_dir(const char*path){
	if(!path)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->is_dir_cb)return LV_FS_RES_NOT_IMP;
	return fse->is_dir_cb(drv,lv_fs_get_real_path(path));
}

lv_res_t lv_fs_get_label(enum item_type*type,const char*path){
	if(!path||!type)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->get_type_cb)return LV_FS_RES_NOT_IMP;
	return fse->get_type_cb(drv,lv_fs_get_real_path(path),type);
}

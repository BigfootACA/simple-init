/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"lvgl.h"
#include"defines.h"
#include"gui/fsext.h"

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

lv_res_t lv_fs_mkdir(const char*path){
	if(!path)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->mkdir)return LV_FS_RES_NOT_IMP;
	return fse->mkdir(drv,lv_fs_get_real_path(path));
}

lv_res_t lv_fs_creat(const char*path){
	if(!path)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse||!fse->creat)return LV_FS_RES_NOT_IMP;
	return fse->creat(drv,lv_fs_get_real_path(path));
}

lv_fs_res_t lv_fs_size(lv_fs_file_t*file_p,uint32_t*size){
	if(!size)return LV_FS_RES_INV_PARAM;
	if(!file_p->drv)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=file_p->drv->user_data;
	if(!fse->size)return LV_FS_RES_NOT_IMP;
	return fse->size(file_p->drv,file_p->file_d,size);
}

lv_fs_res_t lv_fs_rename(const char*oldname,const char*newname){
	if(!oldname||!newname)return LV_FS_RES_INV_PARAM;
	char letter=oldname[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse->rename)return LV_FS_RES_NOT_IMP;
	return fse->rename(drv,
		lv_fs_get_real_path(oldname),
		lv_fs_get_real_path(newname)
	);
}

lv_fs_res_t lv_fs_remove(const char*path){
	if(!path)return LV_FS_RES_INV_PARAM;
	char letter=path[0];
	lv_fs_drv_t*drv= lv_fs_get_drv(letter);
	if(drv == NULL) return LV_FS_RES_NOT_EX;
	if(drv->ready_cb&&!drv->ready_cb(drv))return LV_FS_RES_HW_ERR;
	struct fsext*fse=drv->user_data;
	if(!fse->remove)return LV_FS_RES_NOT_IMP;
	return fse->remove(drv,lv_fs_get_real_path(path));
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
#endif

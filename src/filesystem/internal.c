/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"str.h"
#include"fs_internal.h"

static bool fsdrv_initialized=false;

bool fs_file_info_check(fs_file_info*f){
	if(!f)return false;
	if(memcmp(
		f->magic,FS_INFO_MAGIC,
		sizeof(f->magic)
	)!=0)return false;
	if(!fsh_check(f->parent))return false;
	return true;
}

bool fsdrv_check(const fsdrv*drv){
	if(!drv)return false;
	if(memcmp(
		drv->magic,FS_DRIVER_MAGIC,
		sizeof(drv->magic)
	)!=0)return false;
	if(!drv->is_compatible&&!drv->protocol[0])return false;
	if(!drv->readonly_fs&&!drv->write)return false;
	if(!drv->open)return false;
	if(!drv->read)return false;
	return true;
}

bool fsh_check(fsh*f){
	if(!f)return false;
	if(memcmp(
		f->magic,FS_HANDLE_MAGIC,
		sizeof(f->magic)
	)!=0)return false;
	if(!f->uri)return false;
	if(!f->data&&f->fd<=0)return false;
	if(!f->driver)return false;
	if(!fsdrv_check(f->driver))return false;
	return true;
}

bool fsvol_check(const fsvol*drv){
	if(!drv)return false;
	if(memcmp(
		drv->magic,FS_VOLUME_MAGIC,
		sizeof(drv->magic)
	)!=0)return false;
	if(!drv->name[0])return false;
	if(!drv->scan)return false;
	return true;
}

bool fsvol_private_info_check(const fsvol_private_info*info){
	if(!info)return false;
	if(memcmp(
		info->info.magic,FS_VOL_INFO_MAGIC,
		sizeof(info->info.magic)
	)!=0)return false;
	if(!info->vol)return false;
	if(!info->info.id[0])return false;
	if(!info->info.name[0])return false;
	if(strcmp(info->info.driver,info->vol->name)!=0)return false;
	return true;
}

bool fsvol_info_check(const fsvol_info*info){
	return fsvol_private_info_check((const fsvol_private_info*)info);
}

void fsh_free(fsh**h){
	if(!h||!*h)return;
	if((*h)->uri)url_free((*h)->uri);
	if((*h)->url)free((*h)->url);
	if((*h)->data)free((*h)->data);
	list_free_all_def((*h)->on_close);
	MUTEX_DESTROY((*h)->lock);
	memset((*h),0,sizeof(fsh));
	free(*h);
	*h=NULL;
}

fsh*fsh_get_new(
	const fsdrv*drv,
	url*uri,
	void**data,
	size_t ds,
	fs_file_flag flag
){
	char*p;
	fsh*h=NULL;
	list*l=NULL;
	size_t len,i;
	if(!drv)EPRET(EINVAL);
	if(!(h=malloc(sizeof(fsh))))EDONE();
	memset(h,0,sizeof(fsh));
	memcpy(h->magic,FS_HANDLE_MAGIC,sizeof(h->magic));
	h->flags=flag,h->driver=drv;
	if(uri){
		if(!(h->uri=url_dup(uri)))EDONE();
		if((p=h->uri->path)){
			if((len=strlen(p))<=0)EDONE();
			for(i=0;i<len;i++)if(p[i]=='\\')p[i]='/';
			if((l=path2list(p,true))){
				if(!(p=malloc(len*2)))EDONE();
				memset(p,0,len*2);
				list_string_append(l,p,len*2,"/");
				list_free_all_def(l);
				free(h->uri->path);
				l=NULL,h->uri->path=p;
			}
		}else if(!(h->uri->path=strdup("/")))EDONE();
		if(!(h->url=url_generate_alloc(h->uri)))EDONE();
	}
	if(ds>0){
		if(!(h->data=malloc(ds)))EDONE();
		memset(h->data,0,ds);
		if(data)*data=h->data;
	}
	errno=0;
	return h;
	done:fsh_free(&h);
	if(l)list_free_all_def(l);
	EPRET(ENOMEM);
}

void fsdrv_initialize(){
	static bool run=false;
	if(!run){
		MUTEX_INIT(fsdrv_lock);
		MUTEX_INIT(fsvol_lock);
		MUTEX_INIT(fsvol_info_lock);
	}
	run=true;
	if(!fsdrv_initialized){
		fsdrv_initialized=true;
		for(size_t i=0;fs_initiator[i];i++)
			fs_initiator[i](false);
	}
}

__attribute__((destructor)) void fsdrv_deinitialize(){
	MUTEX_LOCK(fsdrv_lock);
	if(fsdrv_initialized){
		fsdrv_initialized=false;
		for(size_t i=0;fs_initiator[i];i++)
			fs_initiator[i](true);
		list_free_all_def(fs_drivers);
		fs_drivers=NULL;
	}
	MUTEX_UNLOCK(fsdrv_lock);
}

const fsdrv*fsdrv_lookup(url*u){
	list*l;
	fs_drv_is_compatible h;
	if(!u)EPRET(EINVAL);
	fsdrv_initialize();
	MUTEX_LOCK(fsdrv_lock);
	if((l=list_first(fs_drivers)))do{
		LIST_DATA_DECLARE(d,l,fsdrv*);
		if(!fsdrv_check(d))continue;
		if(!(h=d->is_compatible))
			h=fsdrv_template.is_compatible;
		if(!h(d,u))continue;
		MUTEX_UNLOCK(fsdrv_lock);
		errno=0;
		return d;
	}while((l=l->next));
	MUTEX_UNLOCK(fsdrv_lock);
	EPRET(ENOENT);
}

static bool fsdrv_proto_cmp(list*l,void*d){
	return l&&d&&strcasecmp(l->data,d)==0;
}

const fsdrv*fsdrv_lookup_by_protocol(const char*name){
	list*l;
	if(!name)EPRET(EINVAL);
	fsdrv_initialize();
	MUTEX_LOCK(fsdrv_lock);
	l=list_search_one(
		fs_drivers,
		fsdrv_proto_cmp,
		(void*)name
	);
	MUTEX_UNLOCK(fsdrv_lock);
	if(l)return LIST_DATA(l,const fsdrv*);
	EPRET(ENOENT);
}

int fsdrv_register(fsdrv*drv){
	if(!fsdrv_check(drv))EXRET(EINVAL);
	if(drv->protocol[0]&&list_search_one(
		fs_drivers,fsdrv_proto_cmp,
		(void*)drv->protocol
	))RET(EEXIST);
	MUTEX_LOCK(fsdrv_lock);
	int r=list_obj_add_new(
		&fs_drivers,drv
	);
	MUTEX_UNLOCK(fsdrv_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

int fsdrv_register_dup(fsdrv*drv){
	if(!fsdrv_check(drv))EXRET(EINVAL);
	if(drv->protocol[0]&&list_search_one(
		fs_drivers,fsdrv_proto_cmp,
		(void*)drv->protocol
	))RET(EEXIST);
	MUTEX_LOCK(fsdrv_lock);
	int r=list_obj_add_new_dup(
		&fs_drivers,drv,sizeof(fsdrv)
	);
	MUTEX_UNLOCK(fsdrv_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

void fsvol_info_free(fsvol_private_info*info){
	if(!info)return;
	if(info->data)free(info->data);
	free(info);
}

fsvol_private_info*fsvol_info_new(
	const fsvol*vol,
	void**data,
	const char*id,
	const char*name,
	const char*title,
	fsvol_feature features
){
	fsvol_private_info*info;
	size_t s=sizeof(fsvol_private_info);
	if(!vol)EPRET(EINVAL);
	if(!(info=malloc(s)))EPRET(ENOMEM);
	memset(info,0,s);
	if(
		vol->info_data_size>0&&
		!(info->data=malloc(vol->info_data_size))
	)DONE(ENOMEM);
	info->info.features=features,info->vol=vol;
	memcpy(info->info.magic,FS_VOL_INFO_MAGIC,sizeof(info->info.magic));
	strncpy(info->info.driver,vol->name,sizeof(info->info.driver)-1);
	if(title||name)strncpy(info->info.title,title?:_(name),sizeof(info->info.title)-1);
	if(name)strncpy(info->info.name,name,sizeof(info->info.name)-1);
	if(id)strncpy(info->info.id,id,sizeof(info->info.id)-1);
	if(data)*data=info->data;
	return info;
	done:
	fsvol_info_free(info);
	return NULL;
}

void fsvol_info_delete(fsvol_private_info*info){
	if(!info)return;
	MUTEX_LOCK(fsvol_info_lock);
	list_obj_del_data(&fs_volume_infos,info,NULL);
	MUTEX_UNLOCK(fsvol_info_lock);
	fsvol_info_free(info);
}

int fsvol_add_volume(fsvol_private_info*info){
	if(!fsvol_private_info_check(info))RET(EINVAL);
	MUTEX_LOCK(fsvol_info_lock);
	int r=list_obj_add_new(
		&fs_volume_infos,info
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

int fsvol_add_volume_dup(fsvol_private_info*info){
	if(!fsvol_private_info_check(info))RET(EINVAL);
	MUTEX_LOCK(fsvol_info_lock);
	int r=list_obj_add_new_dup(
		&fs_volume_infos,info,
		sizeof(fsvol_private_info)
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

int fsvol_register(fsvol*drv){
	if(!fsvol_check(drv))RET(EINVAL);
	MUTEX_LOCK(fsvol_lock);
	int r=list_obj_add_new(
		&fs_volumes,drv
	);
	MUTEX_UNLOCK(fsvol_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

int fsvol_register_dup(fsvol*drv){
	if(!fsvol_check(drv))RET(EINVAL);
	MUTEX_LOCK(fsvol_lock);
	int r=list_obj_add_new_dup(
		&fs_volumes,drv,
		sizeof(fsvol)
	);
	MUTEX_UNLOCK(fsvol_lock);
	if(r!=0)EXRET(ENOMEM);
	RET(0);
}

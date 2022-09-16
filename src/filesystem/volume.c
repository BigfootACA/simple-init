/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"

static bool cmp_fsid(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(f,l,fsvol_private_info*);
	struct fsvol_info_fsid*id1=d,*id2=&f->info.fsid;
	if(!fsvol_private_info_check(f)||!id1)return false;
	if(id1->id1==0||id1->id2==0)return false;
	if(id2->id1==0||id2->id2==0)return false;
	return id1->id1==id2->id1&&id1->id2==id2->id2;
}

static bool cmp_id(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(f,l,fsvol_private_info*);
	if(!fsvol_private_info_check(f))return false;
	if(!f->info.id[0])return false;
	return strcmp(d,f->info.id)==0;
}

static bool cmp_name(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(f,l,fsvol_private_info*);
	if(!fsvol_private_info_check(f))return false;
	if(!f->info.name[0])return false;
	return strcmp(d,f->info.name)==0;
}

static bool cmp_fs_label(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(f,l,fsvol_private_info*);
	if(!fsvol_private_info_check(f))return false;
	if(!f->info.fs.label[0])return false;
	return strcasecmp(d,f->info.fs.label)==0;
}

static bool cmp_part_label(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(f,l,fsvol_private_info*);
	if(!fsvol_private_info_check(f))return false;
	if(!f->info.part.label[0])return false;
	return strcasecmp(d,f->info.part.label)==0;
}

static bool fsvol_info_initialize(){
	static bool first_scan=false;
	if(fs_volume_infos)return true;
	if(first_scan)return true;
	first_scan=true;
	fsvol_rescan();
	return fs_volume_infos!=NULL;
}

fsvol_info*fsvol_lookup_by_id(const char*id){
	if(!id)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	MUTEX_LOCK(fsvol_info_lock);
	list*l=list_search_one(
		fs_volume_infos,
		cmp_id,(void*)id
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,fsvol_info*);
}

fsvol_info*fsvol_lookup_by_name(const char*name){
	if(!name)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	MUTEX_LOCK(fsvol_info_lock);
	list*l=list_search_one(
		fs_volume_infos,
		cmp_name,(void*)name
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,fsvol_info*);
}

fsvol_info*fsvol_lookup_by_fsid(int64_t id1,int64_t id2){
	if(id1==0||id2==0)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	struct fsvol_info_fsid fsid={.id1=id1,.id2=id2};
	MUTEX_LOCK(fsvol_info_lock);
	list*l=list_search_one(
		fs_volume_infos,
		cmp_fsid,&fsid
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,fsvol_info*);
}

fsvol_info*fsvol_lookup_by_fs_label(const char*name){
	if(!name)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	MUTEX_LOCK(fsvol_info_lock);
	list*l=list_search_one(
		fs_volume_infos,
		cmp_fs_label,
		(void*)name
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,fsvol_info*);
}

fsvol_info*fsvol_lookup_by_part_label(const char*name){
	if(!name)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	MUTEX_LOCK(fsvol_info_lock);
	list*l=list_search_one(
		fs_volume_infos,
		cmp_part_label,
		(void*)name
	);
	MUTEX_UNLOCK(fsvol_info_lock);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,fsvol_info*);
}

fsvol_info**fsvol_get_by_driver_name(const char*name){
	list*l;
	size_t cnt=0,size,i=0;
	fsvol_info**infos=NULL;
	if(!name)EPRET(EINVAL);
	if(!fsvol_info_initialize())EPRET(ENOENT);
	MUTEX_LOCK(fsvol_info_lock);
	if((l=list_first(fs_volume_infos)))do{
		LIST_DATA_DECLARE(info,l,fsvol_info*);
		if(!fsvol_info_check(info))continue;
		if(strcmp(name,info->name)==0)cnt++;
	}while((l=l->next));
	size=(cnt+1)*sizeof(fsvol_info*);
	if(cnt>0&&(infos=malloc(size))){
		memset(infos,0,size);
		if((l=list_first(fs_volume_infos)))do{
			LIST_DATA_DECLARE(info,l,fsvol_info*);
			if(!fsvol_info_check(info))continue;
			if(strcmp(name,info->name)==0)infos[i++]=info;
			if(i>=cnt)break;
		}while((l=l->next));
	}
	MUTEX_UNLOCK(fsvol_info_lock);
	return infos;
}

void fsvol_rescan(){
	list*l;
	fsdrv_initialize();
	if(!fs_volumes)return;
	MUTEX_LOCK(fsvol_lock);
	if((l=list_first(fs_volumes)))do{
		LIST_DATA_DECLARE(vol,l,fsvol*);
		if(!fsvol_check(vol))continue;
		if(!vol->scan)continue;
		vol->scan(vol);
	}while((l=l->next));
	MUTEX_UNLOCK(fsvol_lock);
}

void fsvol_update(){
	list*l,*n;
	if(!fs_volume_infos){
		fsvol_rescan();
		return;
	}
	MUTEX_LOCK(fsvol_info_lock);
	if((l=list_first(fs_volume_infos)))do{
		n=l->next;
		LIST_DATA_DECLARE(info,l,fsvol_private_info*);
		if(!fsvol_private_info_check(info))continue;
		if(!info->vol->update)continue;
		info->vol->update(info->vol,info);
	}while((l=n));
	MUTEX_UNLOCK(fsvol_info_lock);
}

int fsvol_open_volume(fsvol_info*info,fsh**nf){
	fsvol_private_info*p=(fsvol_private_info*)info;
	if(!info||!nf||!p->vol)RET(EINVAL);
	if(!p->vol->open)RET(ENOSYS);
	RET(p->vol->open(p->vol,p,nf));
}

fsvol_info**fsvol_get_volumes(){
	list*l;
	int cnt,i=0;
	size_t size=0;
	fsvol_info**infos=NULL;
	fsvol_info_initialize();
	MUTEX_LOCK(fsvol_info_lock);
	cnt=list_count(fs_volume_infos);
	if(cnt>0)size=(cnt+1)*sizeof(fsvol_info*);
	if(cnt>0&&size>0&&(infos=malloc(size))){
		memset(infos,0,size);
		if((l=list_first(fs_volume_infos)))do{
			LIST_DATA_DECLARE(info,l,fsvol_info*);
			if(!fsvol_info_check(info))continue;
			infos[i++]=info;
			if(i>=cnt)break;
		}while((l=l->next));
	}
	MUTEX_UNLOCK(fsvol_info_lock);
	return infos;
}

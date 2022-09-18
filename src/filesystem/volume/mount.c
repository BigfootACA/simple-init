/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<sys/statfs.h>
#include<libblkid/blkid.h>
#include"system.h"
#include"linux.h"
#include"../fs_internal.h"

struct mnt_info{
	bool found;
	struct mount_item*mnt;
};

static int fs_volume_update_core(fsvol_private_info*info,struct statfs*st){
	struct mnt_info*mi;
	if(!info||!(mi=info->data))RET(EINVAL);
	if(st)fill_from_statfs(info,st);
	fill_from_mount_item(info,mi->mnt);
	if(strncmp(mi->mnt->source,"/dev/",5)==0)
		fill_from_block_path(info,mi->mnt->source);
	RET(0);
}

static int fs_volume_update(const fsvol*vol,fsvol_private_info*info){
	bool stf=false;
	struct statfs st;
	struct mnt_info*mi;
	if(!info||!(mi=info->data)||!mi->mnt)RET(EINVAL);
	if(!mi->mnt->source||!vol||info->vol!=vol)RET(EINVAL);
	if(statfs(mi->mnt->source,&st)==0)stf=true;
	return fs_volume_update_core(info,stf?&st:NULL);
}

static int scan_mount(const fsvol*vol,struct mount_item*m){
	int64_t fsid=0;
	bool stf;
	char id[64];
	struct statfs st;
	struct mnt_info*mi;
	fsvol_feature feature;
	fsvol_private_info*info;
	feature=0,stf=false,mi=NULL;
	if(!gen_id_from_mount_item(m,id,sizeof(id)))RET(ENOTSUP);
	if((info=fsvolp_lookup_by_id(id))){
		if(info->vol==vol&&(mi=info->data))
			mi->found=true;
		RET(EEXIST);
	}
	if(statfs(m->target,&st)==0){
		stf=true;
		memcpy(&fsid,&st.f_fsid,sizeof(int64_t));
		if(fsid!=0&&(info=fsvolp_lookup_by_fsid(1,fsid))){
			if(info->vol==vol&&(mi=info->data))
				mi->found=true;
			RET(EEXIST);
		}
	}
	if(strncmp(m->source,"/dev/",5)!=0){
		feature|=FSVOL_VIRTUAL;
		feature|=FSVOL_HIDDEN;
	}
	feature|=FSVOL_FILES;
	info=fsvol_info_new(
		vol,(void**)&mi,id,
		m->source,m->source,feature
	);
	if(info){
		info->info.fsid.id1=1;
		info->info.fsid.id2=fsid;
		mi->mnt=m,mi->found=true;
		if(fsvol_add_volume(info)==0){
			fs_volume_update_core(info,stf?&st:NULL);
			RET(0);
		}
		fsvol_info_delete(info);
	}
	RET(ENOMEM);
}

static void renew_all(const fsvol*vol,bool found,bool delete){
	struct mnt_info*mi;
	fsvol_private_info*info,**infos;
	if(!(infos=fsvolp_get_by_driver_name(vol->name)))return;
	for(size_t i=0;(info=infos[i]);i++){
		if(!(mi=info->data))continue;
		if(mi->found==found&&delete)
			fsvol_info_delete(info);
		else mi->found=found;
	}
	free(infos);
}

static int fs_volume_scan(const fsvol*vol){
	struct mount_item**ms,*m;
	if(!(ms=read_proc_mounts()))EXRET(ENOENT);
	renew_all(vol,false,false);
	for(size_t i=0;(m=ms[i]);i++)
		if(scan_mount(vol,m)!=0)
			free_mount_item(m);
	free(ms);
	renew_all(vol,false,true);
	RET(0);
}

static int fs_volume_open(const fsvol*vol,fsvol_private_info*info,fsh**hand){
	url*u;
	int r;
	struct mnt_info*mi;
	if(!info||!hand)RET(EINVAL);
	if(!vol||info->vol!=vol)RET(EINVAL);
	if(!(mi=info->data))RET(EINVAL);
	if(!mi->mnt||!mi->mnt->target)RET(EINVAL);
	if(!(u=url_new()))RET(ENOMEM);
	url_set_scheme(u,"file",0);
	url_set_path(u,mi->mnt->target,0);
	r=fs_open_uri(hand,u,FILE_FLAG_FOLDER);
	url_free(u);
	return r;
}

static fsvol vol_mount={
	.magic=FS_VOLUME_MAGIC,
	.name="mount",
	.info_data_size=sizeof(struct mnt_info),
	.update=fs_volume_update,
	.scan=fs_volume_scan,
	.open=fs_volume_open,
};

void fsvol_register_mount(bool deinit){
	if(!deinit)fsvol_register_dup(&vol_mount);
}

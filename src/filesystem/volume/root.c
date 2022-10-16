/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<linux/fs.h>
#include<sys/statfs.h>
#include<libblkid/blkid.h>
#include"system.h"
#include"linux.h"
#include"../fs_internal.h"

static int fs_volume_update_core(fsvol_private_info*info,struct statfs*st){
	struct mount_item**ms,*mnt;
	char block[PATH_MAX];
	if(!info)RET(EINVAL);
	memset(block,0,sizeof(block));
	if(st)fill_from_statfs(info,st);
	if((ms=read_proc_mounts())){
		for(size_t i=0;(mnt=ms[i]);i++){
			if(strcmp(mnt->target,"/")!=0)continue;
			if(block[0]&&strncmp(mnt->source,"/dev/",5)!=0)continue;
			strncpy(block,mnt->source,sizeof(block)-1);
			fill_from_mount_item(info,mnt);
			break;
		}
		free_mounts(ms);
	}
	if(strncmp(block,"/dev/",5)==0){
		fill_from_block_path(info,block);
		info->info.features&=~FSVOL_VIRTUAL;
	}else info->info.features|=FSVOL_VIRTUAL;
	info->info.features|=FSVOL_FILES;
	RET(0);
}

static int fs_volume_update(const fsvol*vol,fsvol_private_info*info){
	bool stf=false;
	struct statfs st;
	if(!info||!vol||info->vol!=vol)RET(EINVAL);
	if(statfs("/",&st)==0)stf=true;
	return fs_volume_update_core(info,stf?&st:NULL);
}

static int fs_volume_scan(const fsvol*vol){
	int64_t fsid=0;
	struct statfs st;
	fsvol_private_info*info;
	if(fsvol_lookup_by_id("root"))RET(0);
	if(statfs("/",&st)==0){
		memcpy(&fsid,&st.f_fsid,sizeof(int64_t));
		if(fsid!=0&&fsvolp_lookup_by_fsid(1,fsid))RET(0);
	}
	info=fsvol_info_new(vol,NULL,"root","Root",_("Root"),0);
	if(!info)EXRET(ENOMEM);
	info->info.fsid.id1=1;
	info->info.fsid.id2=fsid;
	if(fsvol_add_volume(info)!=0){
		fsvol_info_delete(info);
		EXRET(ENOMEM);
	}
	RET(fs_volume_update_core(info,&st));
}

static int fs_volume_open(const fsvol*vol,fsvol_private_info*info,fsh**hand){
	url*u;
	int r;
	if(!info||!hand)RET(EINVAL);
	if(!vol||info->vol!=vol)RET(EINVAL);
	if(!(u=url_new()))RET(ENOMEM);
	url_set_scheme(u,"file",0);
	url_set_path(u,"/",0);
	r=fs_open_uri(hand,u,FILE_FLAG_FOLDER);
	url_free(u);
	return r;
}

static fsvol vol_root={
	.magic=FS_VOLUME_MAGIC,
	.name="root",
	.update=fs_volume_update,
	.scan=fs_volume_scan,
	.open=fs_volume_open,
};

void fsvol_register_root(bool deinit){
	if(!deinit)fsvol_register_dup(&vol_root);
}

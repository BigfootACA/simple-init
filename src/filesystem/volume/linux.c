/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<linux/fs.h>
#include<sys/ioctl.h>
#include<sys/statfs.h>
#include<libblkid/blkid.h>
#include"system.h"
#include"linux.h"
#include"md5.h"
#include"str.h"
#include"../fs_internal.h"

void fill_from_mount_item(fsvol_private_info*info,struct mount_item*mnt){
	strncpy(info->info.fs.type,mnt->type,sizeof(info->info.fs.type)-1);
	if(mnt->options)for(size_t o=0;mnt->options[o];o++){
		if(strcmp(mnt->options[o],"ro")==0)
			info->info.features|=FSVOL_READONLY;
		if(strcmp(mnt->options[o],"rw")==0)
			info->info.features&=~FSVOL_READONLY;
	}
}

void fill_from_block_path(fsvol_private_info*info,char*block){
	char*value;
	struct fsvol_info_fs*fs=&info->info.fs;
	struct fsvol_info_part*p=&info->info.part;
	if((value=blkid_get_tag_value(NULL,"TYPE",block))){
		strncpy(fs->type,value,sizeof(fs->type)-1);
		free(value);
	}
	if((value=blkid_get_tag_value(NULL,"UUID",block))){
		strncpy(fs->uuid,value,sizeof(fs->uuid)-1);
		free(value);
	}
	if((value=blkid_get_tag_value(NULL,"PARTUUID",block))){
		strncpy(p->uuid,value,sizeof(p->uuid)-1);
		free(value);
	}
	if((value=blkid_get_tag_value(NULL,"LABEL",block))){
		strncpy(fs->label,value,sizeof(fs->label)-1);
		free(value);
	}
	if((value=blkid_get_tag_value(NULL,"PARTLABEL",block))){
		strncpy(p->label,value,sizeof(p->label)-1);
		free(value);
	}
	int fd=open(block,O_RDONLY);
	if(fd>=0){
		ioctl(fd,BLKGETSIZE64,&p->size);
		ioctl(fd,BLKGETSIZE,&p->sector_count);
		ioctl(fd,BLKSSZGET,&p->sector_size);
	}
}

void fill_from_statfs(fsvol_private_info*info,struct statfs*st){
	int64_t fsid=0;
	struct fsvol_info_fs*fs=&info->info.fs;
	fs->size=st->f_bsize*st->f_blocks;
	fs->avail=st->f_bsize*st->f_bavail;
	fs->used=fs->size-fs->avail;
	memcpy(&fsid,&st->f_fsid,sizeof(int64_t));
	if(fsid!=0){
		info->info.fsid.id1=1;
		info->info.fsid.id2=fsid;
	}
}

char*gen_id_from_mount_item(struct mount_item*item,char*buff,size_t len){
	ssize_t l;
	char*p=NULL;
	struct MD5Context md5;
	uint8_t hash[MD5_DIGEST_LENGTH];
	if(!item||!buff||len<=32)return NULL;
	if(!item->source||!item->type||!item->target)return NULL;
	l=asprintf(&p,"%s#%s#%s",item->source,item->type,item->target);
	if(!p||l<=0)return NULL;
	memset(hash,0,sizeof(hash));
	memset(buff,0,len);
	MD5Init(&md5);
	MD5Update(&md5,(uint8_t*)p,(size_t)l);
	MD5Final(hash,&md5);
	free(p);
	return bin2hexstr(buff,hash,sizeof(hash),false);
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<sys/stat.h>
#include"assets.h"
#include"../fs_internal.h"

static fsdrv fsdrv_assets;

struct fsd{
	size_t pos;
	fs_type type;
	entry*info;
	union{
		entry_file*file;
		entry_dir*dir;
	};
};

static bool assets_info_to_file_info(fs_file_info*info,entry*en){
	if(!info||!en)return false;
	memcpy(info->magic,FS_INFO_MAGIC,sizeof(info->magic));
	strncpy(info->name,en->name,sizeof(info->name)-1);
	info->owner=en->owner;
	info->group=en->group;
	info->atime=en->atime.tv_sec;
	info->ctime=en->mtime.tv_sec;
	info->mtime=en->mtime.tv_sec;
	if(S_ISREG(en->mode))
		info->type=FS_TYPE_FILE_REG;
	else if(S_ISDIR(en->mode))
		info->type=FS_TYPE_FILE_FOLDER;
	else if(S_ISLNK(en->mode))
		info->type=FS_TYPE_FILE_LINK;
	else if(S_ISSOCK(en->mode))
		info->type=FS_TYPE_FILE_SOCKET;
	else if(S_ISBLK(en->mode))
		info->type=FS_TYPE_FILE_BLOCK;
	else if(S_ISCHR(en->mode))
		info->type=FS_TYPE_FILE_CHAR;
	else if(S_ISFIFO(en->mode))
		info->type=FS_TYPE_FILE_FIFO;
	else return false;
	info->mode=en->mode;
	return true;
}

static bool assets_file_to_file_info(
	fs_file_info*info,
	entry_file*en
){
	if(!info||!en)return false;
	if(!assets_info_to_file_info(
		info,&en->info
	))return false;
	info->size=en->length;
	info->device=en->dev;
	return true;
}

static bool assets_dir_to_file_info(
	fs_file_info*info,
	entry_dir*en
){
	if(!info||!en)return false;
	return assets_info_to_file_info(
		info,&en->info
	);
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flag
){
	size_t l;
	struct fsd*nd=NULL;
	entry_dir*dir=NULL;
	if(!drv||!nf||!uri||!(nd=nf->data))RET(EINVAL);
	if(fs_has_flag(flag,FILE_FLAG_WRITE))RET(EROFS);
	if(drv->data)dir=drv->data;
	else{
		dir=&assets_rootfs;
		if(uri->host){
			if(strcasecmp(uri->host,"rootfs")==0)
				dir=&assets_rootfs;
			else dir=NULL;
		}
	}
	if(!dir)RET(EHOSTUNREACH);
	const char*path=uri->path?:"/";
	if(fs_has_flag(flag,FILE_FLAG_FOLDER)){
		nd->type=FS_TYPE_FILE_FOLDER;
		if(!(nd->dir=get_assets_dir(
			dir,path
		)))EXRET(ENOENT);
		l=strlen(nf->uri->path);
		if(l<=0)EXRET(EINVAL);
		if(nf->uri->path[l-1]!='/')
			strcat(nf->uri->path,"/");
	}else{
		nd->type=FS_TYPE_FILE_REG;
		if(!(nd->file=get_assets_file(
			dir,path
		)))EXRET(ENOENT);
	}
	nd->info=&nd->file->info;
	RET(0);
}

static int fsdrv_read(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	errno=0;
	struct fsd*d;
	if(!f||!(d=f->data)||!buffer)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(br)*br=0;
	if(fs_has_flag(
		f->flags,
		FILE_FLAG_FOLDER
	))RET(EISDIR);
	if(!d->file)RET(EBADF);
	if(!d->file->content)RET(EFAULT);
	if(d->pos>d->file->length)RET(0);
	size_t ms=MIN(btr,d->file->length-d->pos);
	memcpy(buffer,d->file->content,ms);
	if(br)*br=ms;
	RET(0);
}

static int fsdrv_readdir(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	struct fsd*d;
	size_t off=0,s;
	if(!f||!drv||!f->data||!info)RET(EINVAL);
	if(f->driver!=drv||!(d=f->data))RET(EINVAL);
	if(!(fs_has_flag(f->flags,FILE_FLAG_FOLDER)))RET(ENOTDIR);
	if(d->type!=FS_TYPE_FILE_FOLDER)RET(ENOTDIR);
	memset(info,0,sizeof(fs_file_info));
	info->features=drv->features;
	if(d->dir->subdirs)for(s=0;d->dir->subdirs[s];s++,off++){
		if(off!=d->pos)continue;
		d->pos++;
		if(!assets_dir_to_file_info(
			info,d->dir->subdirs[s]
		))EXRET(EIO);
		info->parent=f;
		RET(0);
	}
	if(d->dir->subfiles)for(s=0;d->dir->subfiles[s];s++,off++){
		if(off!=d->pos)continue;
		d->pos++;
		if(!assets_file_to_file_info(
			info,d->dir->subfiles[s]
		))EXRET(EIO);
		info->parent=f;
		RET(0);
	}
	errno=0;
	RET(EOF);
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	errno=0;
	if(!f||!f->data)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	struct fsd*d=f->data;
	switch(whence){
		case SEEK_SET:d->pos=pos;break;
		case SEEK_CUR:d->pos+=pos;break;
		case SEEK_END:
			if(d->type==FS_TYPE_FILE_REG&&d->file)
				d->pos=d->file->length+pos;
		break;
	}
	RET(0);
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	errno=0;
	if(!f||!f->data)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	struct fsd*d=f->data;
	*pos=d->pos;
	RET(0);
}

static int fsdrv_map(
	const fsdrv*drv,
	fsh*f,
	void**buffer,
	size_t off,
	size_t*size,
	fs_file_flag flag
){
	struct fsd*d;
	if(!f||!(d=f->data))RET(EINVAL);
	if(!buffer||!*size)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(fs_has_flag(flag,FILE_FLAG_WRITE))RET(EROFS);
	if(!(fs_has_flag(f->flags,FILE_FLAG_FOLDER)))RET(ENOTDIR);
	if(d->type!=FS_TYPE_FILE_REG)RET(EISDIR);
	if(*size==0&&fs_get_size(f,size)!=0)EXRET(EINVAL);
	if(!d->file||*size==0)RET(EBADF);
	if(off+*size>=d->file->length)RET(EFAULT);
	if(!d->file->content)RET(EFAULT);
	*buffer=d->file->content+off;
	*size=d->file->length;
	RET(0);
}

static int fsdrv_unmap(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t size
){
	struct fsd*d;
	if(!f||!(d=f->data)||size<=0)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(!(fs_has_flag(f->flags,FILE_FLAG_FOLDER)))RET(ENOTDIR);
	if(d->type!=FS_TYPE_FILE_REG)RET(EISDIR);
	if(!d->file)RET(EBADF);
	if(buffer!=d->file->content)RET(EFAULT);
	RET(0);
}

static int fsdrv_get_type(
	const fsdrv*drv,
	fsh*f,
	fs_type*type
){
	struct fsd*d;
	if(!f||!(d=f->data)||!type)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	*type=d->type;
	RET(0);
}

static int fsdrv_get_size(
	const fsdrv*drv,
	fsh*f,
	size_t*out
){
	struct fsd*d;
	if(!f||!(d=f->data)||!out)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(d->type!=FS_TYPE_FILE_REG)RET(EISDIR);
	if(!d->file)RET(EBADF);
	*out=d->file->length;
	RET(0);
}

static int fsdrv_get_name(
	const fsdrv*drv,
	fsh*f,
	char*buff,
	size_t buff_len
){
	struct fsd*d;
	if(!f||!(d=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(!buff||buff_len<=0)RET(EINVAL);
	memset(buff,0,buff_len);
	if(!d->info)RET(EBADF);
	strncpy(buff,d->info->name,buff_len-1);
	RET(0);
}

static int fsdrv_get_info(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	struct fsd*d;
	if(!f||!(d=f->data)||!info)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	memset(info,0,sizeof(fs_file_info));
	info->features=drv->features;
	bool x=fs_has_flag(f->flags,FILE_FLAG_FOLDER)?
		assets_dir_to_file_info(info,d->dir):
		assets_file_to_file_info(info,d->file);
	if(!x)EXRET(EIO);
	info->parent=f;
	RET(0);
}

static int fsdrv_add_assets(entry_dir*root,char*offset,const char*name){
	fsdrv*drv=NULL;
	if(!root)root=&assets_rootfs;
	if(offset&&!(root=get_assets_dir(root,offset))){
		tlog_error("folder %s not found",offset);
		RET(ENOENT);
	}
	if(!(drv=malloc(sizeof(fsdrv))))RET(ENOMEM);
	memcpy(drv,&fsdrv_assets,sizeof(fsdrv));
	strncpy(
		drv->protocol,name,
		sizeof(drv->protocol)-1
	);
	drv->data=root;
	if(fsdrv_register(drv)==0)RET(0);
	free(drv);
	EXRET(ENOMEM);
}

static fsdrv fsdrv_assets={
	.magic=FS_DRIVER_MAGIC,
	.base=&fsdrv_template,
	.hand_data_size=sizeof(struct fsd),
	.cache_info_time=INT_MAX,
	.readonly_fs=true,
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_SEEKABLE|
		FS_FEATURE_UNIX_PERM|
		FS_FEATURE_UNIX_DEVICE|
		FS_FEATURE_HAVE_STAT|
		FS_FEATURE_HAVE_SIZE|
		FS_FEATURE_HAVE_PATH|
		FS_FEATURE_HAVE_TIME|
		FS_FEATURE_HAVE_FOLDER,
	.open=fsdrv_open,
	.read=fsdrv_read,
	.readdir=fsdrv_readdir,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.map=fsdrv_map,
	.unmap=fsdrv_unmap,
	.get_info=fsdrv_get_info,
	.get_type=fsdrv_get_type,
	.get_size=fsdrv_get_size,
	.get_name=fsdrv_get_name,
};

void fsdrv_register_assets(bool deinit){
	if(deinit)return;
	fsdrv_add_assets(&assets_rootfs,NULL,"rootfs");
	fsdrv_add_assets(NULL,NULL,"assets");
}

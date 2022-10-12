/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LIBZIP
#include<stdbool.h>
#include<zip.h>
#include<zip_source_file.h>
#include"../fs_internal.h"
#include"str.h"

static mutex_t lock;
static list*opened_zip=NULL;

struct zip_file_ctx{
	fsh*file;
	struct zip_ctx*ctx;
	zip_int64_t index;
	zip_file_t*zip;
	zip_int64_t offset;
};

struct zip_ctx{
	char name[
		1024-
		sizeof(zip_t*)-
		sizeof(fsh*)-
		sizeof(list*)
	];
	mutex_t lock;
	int zip_err;
	zip_t*zip;
	fsh*file;
	list*opened;
};

static int zip_error_to_errno(int err){
	switch(err){
		case ZIP_ER_OK:RET(0);
		case ZIP_ER_RENAME:
		case ZIP_ER_CLOSE:
		case ZIP_ER_OPEN:
		case ZIP_ER_SEEK:
		case ZIP_ER_TELL:
		case ZIP_ER_READ:
		case ZIP_ER_WRITE:
		case ZIP_ER_REMOVE:RET(EIO);
		case ZIP_ER_CRC:
		case ZIP_ER_INCONS:
		case ZIP_ER_COMPRESSED_DATA:RET(EUCLEAN);
		case ZIP_ER_NOPASSWD:;
		case ZIP_ER_WRONGPASSWD:RET(EPERM);
		case ZIP_ER_DELETED:
		case ZIP_ER_NOENT:RET(ENOENT);
		case ZIP_ER_CHANGED:
		case ZIP_ER_RDONLY:RET(EROFS);
		case ZIP_ER_ENCRNOTSUPP:
		case ZIP_ER_COMPNOTSUPP:
		case ZIP_ER_OPNOTSUPP:
		case ZIP_ER_MULTIDISK:
		case ZIP_ER_NOZIP:RET(ENOTSUP);
		case ZIP_ER_ZLIB:
		case ZIP_ER_INTERNAL:RET(EFAULT);
		case ZIP_ER_EOF:RET(ERANGE);
		case ZIP_ER_INUSE:RET(EBUSY);
		case ZIP_ER_INVAL:RET(EINVAL);
		case ZIP_ER_EXISTS:RET(EEXIST);
		case ZIP_ER_MEMORY:RET(ENOMEM);
		case ZIP_ER_TMPOPEN:RET(ENOSPC);
		case ZIP_ER_ZIPCLOSED:RET(EBADF);
		case ZIP_ER_CANCELLED:RET(ECANCELED);
		default:RET(EBADMSG);
	}
}

static bool zip_stat_to_file_info(zip_stat_t*st,fs_file_info*info){
	if(!st||!info)return false;
	memset(info,0,sizeof(fs_file_info));
	memcpy(info->magic,FS_INFO_MAGIC,sizeof(info->magic));
	info->atime=info->mtime=info->ctime=st->mtime;
	info->owner=0,info->group=0,info->device=0;
	info->size=st->size,info->mode=0644;
	info->type=FS_TYPE_FILE_REG;
	if(st->name){
		char*name=strdup(st->name),*c;
		if(!name)return false;
		size_t l=strlen(name);
		if(name[l-1]=='/'){
			info->mode=0755;
			info->type=FS_TYPE_FILE_FOLDER;
			name[--l]=0;
		}
		strncpy(
			info->name,
			(c=strrchr(name,'/'))?c+1:name,
			sizeof(info->name)-1
		);
		free(name);
	}
	return true;
}

static int close_zip_ctx(
	struct zip_ctx*ctx,
	bool close_file,
	bool free_list
){
	list*l;
	if(!ctx)RET(EINVAL);
	MUTEX_LOCK(ctx->lock);
	if((l=list_first(ctx->opened)))do{
		LIST_DATA_DECLARE(c,l,struct zip_file_ctx*);
		if(!c)continue;
		if(c->file)MUTEX_LOCK(c->file->lock);
		if(c->zip)zip_fclose(c->zip);
		c->zip=NULL,c->ctx=NULL;
		if(c->file)MUTEX_UNLOCK(c->file->lock);
	}while((l=l->next));
	if(close_file&&ctx->file)fs_close(&ctx->file);
	if(ctx->zip)zip_close(ctx->zip);
	if(free_list){
		MUTEX_LOCK(lock);
		list_obj_del_data(&opened_zip,ctx,NULL);
		MUTEX_UNLOCK(lock);
	}
	list_free_all(ctx->opened,NULL);
	MUTEX_UNLOCK(ctx->lock);
	MUTEX_DESTROY(ctx->lock);
	free(ctx);
	RET(0);
}

static int free_zip_ctx(void*c){
	return close_zip_ctx(c,true,false);
}

static bool zip_ctx_cmp(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(p,l,struct zip_ctx*);
	if(!p||!p->name[0])return false;
	return strcmp(p->name,d)==0;
}

static void on_base_file_close(
	const char*name __attribute__((unused)),
	fsh*file __attribute__((unused)),
	void*data
){
	close_zip_ctx(data,false,true);
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	list*l;
	size_t len=0;
	int e=0,fl=0;
	char*path=NULL;
	zip_int64_t index=-1;
	struct zip_ctx*c=NULL;
	struct zip_file_ctx*ctx=NULL;
	if(fs_has_flag(flags,FILE_FLAG_WRITE))RET(EROFS);
	if(fs_has_flag(flags,FILE_FLAG_CREATE))RET(EROFS);
	if(fs_has_flag(flags,FILE_FLAG_NON_BLOCK))RET(ENOTSUP);
	if(!uri||!uri->host||!uri->path||uri->path[0]!='/'||!drv)RET(EINVAL);
	if(!fs_has_flag(flags,FILE_FLAG_ACCESS)){
		if(!nf||!(ctx=nf->data))RET(EINVAL);
	}
	MUTEX_LOCK(lock);fl++;
	if(!(l=list_search_one(
		opened_zip,
		zip_ctx_cmp,
		uri->host
	)))DONE(EHOSTUNREACH);
	if(!(c=LIST_DATA(l,struct zip_ctx*)))DONE(EHOSTDOWN);
	MUTEX_LOCK(c->lock);fl++;
	if(!c->zip||!c->file)DONE(EHOSTDOWN);
	if(uri->path[1]){
		if(fs_has_flag(flags,FILE_FLAG_FOLDER)){
			if((len=strlen(uri->path))<=0)DONE(EINVAL);
			if(!(path=malloc(len+8)))DONE(ENOMEM);
			memset(path,0,len+8);
			strcpy(path,uri->path);
			if(path[len-1]!='/')strcat(path,"/");
			if(nf){
				if(nf->uri->path)free(nf->uri->path);
				nf->uri->path=path;
			}
		}else path=uri->path;
		if((index=zip_name_locate(c->zip,path+1,0))<0)
			DONE(zip_error_to_errno(c->zip_err));
	}
	if(!fs_has_flag(flags,FILE_FLAG_ACCESS)){
		if(!ctx||!nf)DONE(EBADF);
		ctx->file=nf,ctx->ctx=c,ctx->index=index;
		if(
			!fs_has_flag(flags,FILE_FLAG_FOLDER)&&
			(index<0||!(ctx->zip=zip_fopen_index(c->zip,index,0)))
		)DONE(zip_error_to_errno(c->zip_err));
		list_obj_add_new(&c->opened,ctx);
	}
	MUTEX_UNLOCK(c->lock);
	MUTEX_UNLOCK(lock);
	RET(0);
	done:e=errno;
	if(c&&(fl--))MUTEX_UNLOCK(c->lock);
	if((fl--))MUTEX_UNLOCK(lock);
	if(ctx&&ctx->zip)zip_fclose(ctx->zip);
	XRET(e,ENOENT);
}

static int fsdrv_get_info(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	int e;
	zip_stat_t st;
	struct zip_ctx*c;
	struct zip_file_ctx*ctx;
	if(!f||!(ctx=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv||!info)RET(EINVAL);
	if(!(c=ctx->ctx)||!c->zip)RET(ESTALE);
	memset(info,0,sizeof(fs_file_info));
	MUTEX_LOCK(c->lock);
	int r=zip_stat_index(c->zip,ctx->index,0,&st);
	if(r!=0)DONE(zip_error_to_errno(c->zip_err));
	if(!zip_stat_to_file_info(&st,info))DONE(EIO);
	info->features=drv->features;
	info->parent=f;
	MUTEX_UNLOCK(c->lock);
	RET(0);
	done:e=errno;
	MUTEX_UNLOCK(c->lock);
	XRET(e,EIO);
}

static int fsdrv_readdir(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	int e;
	zip_stat_t st;
	zip_int64_t cnt;
	size_t len=0,fl;
	struct zip_ctx*c;
	const char*name,*fn;
	struct zip_file_ctx*ctx;
	if(!f||!(ctx=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(!(c=ctx->ctx)||!c->zip)RET(ESTALE);
	if(!fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(ENOTDIR);
	memset(info,0,sizeof(fs_file_info));
	MUTEX_LOCK(c->lock);
	cnt=zip_get_num_entries(c->zip,0);
	if(cnt<0)DONE(EFAULT);
	if(ctx->offset>=cnt)DONE(EOF);
	if((name=zip_get_name(c->zip,ctx->index,0))){
		if((len=strlen(name))<=0)name=NULL;
	}
	do{
		if(zip_stat_index(c->zip,ctx->offset,0,&st)!=0)continue;
		if(!(fn=st.name)||(fl=strlen(fn))<=0)continue;
		if(name&&strncmp(fn,name,len)!=0)continue;
		fn+=len,fl-=len;
		if(fl<=0||memchr(fn,'/',fl-1))continue;
		ctx->offset++;
		if(!zip_stat_to_file_info(&st,info))DONE(EFAULT);
		info->parent=f;
		MUTEX_UNLOCK(c->lock);
		RET(0);
	}while(++ctx->offset<cnt);
	MUTEX_UNLOCK(c->lock);
	RET(EOF);
	done:e=errno;
	MUTEX_UNLOCK(c->lock);
	XRET(e,EIO);
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	struct zip_ctx*c;
	struct zip_file_ctx*ctx;
	if(!f||!(ctx=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(!(c=ctx->ctx)||!c->zip)RET(ESTALE);
	MUTEX_LOCK(c->lock);
	if(!zip_file_is_seekable(ctx->zip))DONE(ENOTSUP);
	if(ctx->zip)zip_fseek(ctx->zip,pos,whence);
	else switch(whence){
		case SEEK_SET:ctx->offset=pos;break;
		case SEEK_CUR:ctx->offset+=pos;break;
		case SEEK_END:ctx->offset=zip_get_num_entries(c->zip,0)+pos;break;
	}
	MUTEX_UNLOCK(c->lock);
	RET(0);
	done:
	MUTEX_UNLOCK(c->lock);
	return errno;
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	struct zip_ctx*c;
	struct zip_file_ctx*ctx;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!(ctx=f->data)||!pos)RET(EINVAL);
	if(!(c=ctx->ctx)||!c->zip)RET(ESTALE);
	MUTEX_LOCK(c->lock);
	if(ctx->zip)*pos=zip_ftell(ctx->zip);
	else *pos=ctx->offset;
	MUTEX_UNLOCK(c->lock);
	return errno;
}

static int fsdrv_read(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	zip_int64_t r;
	struct zip_ctx*c;
	struct zip_file_ctx*ctx;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!(ctx=f->data)||!buffer)RET(EINVAL);
	if(!(c=ctx->ctx)||!c->zip)RET(ESTALE);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	MUTEX_LOCK(c->lock);
	r=zip_fread(ctx->zip,buffer,(zip_uint64_t)btr);
	if(r<0)errno=zip_error_to_errno(c->zip_err);
	else if(br)*br=(size_t)r;
	MUTEX_UNLOCK(c->lock);
	return errno;
}

static void fsdrv_close(const fsdrv*drv,fsh*f){
	struct zip_file_ctx*ctx;
	if(!f||!drv||f->driver!=drv||!(ctx=f->data))return;
	if(ctx->ctx)list_obj_del_data(&ctx->ctx->opened,ctx,NULL);
	if(ctx->zip)zip_fclose(ctx->zip);
}

static fsdrv fsdrv_zip={
	.magic=FS_DRIVER_MAGIC,
	.protocol="zip",
	.readonly_fs=true,
	.hand_data_size=sizeof(struct zip_file_ctx),
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_SEEKABLE|
		FS_FEATURE_HAVE_STAT|
		FS_FEATURE_HAVE_SIZE|
		FS_FEATURE_HAVE_PATH|
		FS_FEATURE_HAVE_TIME|
		FS_FEATURE_HAVE_FOLDER,
	.base=&fsdrv_template,
	.close=fsdrv_close,
	.open=fsdrv_open,
	.readdir=fsdrv_readdir,
	.read=fsdrv_read,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.get_info=fsdrv_get_info,
};

static void zip_src_op_close(zip_source_file_context_t*ctx __attribute__((unused))){}

static zip_int64_t zip_src_op_read(
	zip_source_file_context_t*ctx,
	void*buf,
	zip_uint64_t len
){
	size_t size=0;
	int r=fs_read(ctx->f,buf,len,&size);
	if(r!=0){
		zip_error_set(&ctx->error,ZIP_ER_READ,r);
		return -1;
	}
	return size;
}

static bool zip_src_op_seek(
	zip_source_file_context_t*ctx,
	void*f,
	zip_int64_t offset,
	int whence
){
	int ret=fs_seek(f,offset,whence);
	if(ret!=0){
		zip_error_set(&ctx->error,ZIP_ER_SEEK,ret);
		return false;
	}
	return true;
}

static zip_int64_t zip_src_op_tell(
	zip_source_file_context_t*ctx,
	void*f
){
	size_t pos=0;
	int ret=fs_tell(f,&pos);
	if(ret!=0){
		zip_error_set(&ctx->error,ZIP_ER_TELL,ret);
		return -1;
	}
	return pos;
}

static bool zip_src_op_stat(
	zip_source_file_context_t*ctx,
	zip_source_file_stat_t*st
){
	fs_file_info info;
	int ret=fs_get_info(ctx->f,&info);
	if(ret!=0){
		zip_error_set(&ctx->error,ZIP_ER_READ,ret);
		return false;
	}
	st->regular_file=info.type==FS_TYPE_FILE_REG;
	st->mtime=info.mtime,st->size=info.size,st->exists=true;
	return true;
}

static zip_source_file_operations_t ops_read={
	.close=zip_src_op_close,
	.read=zip_src_op_read,
	.seek=zip_src_op_seek,
	.stat=zip_src_op_stat,
	.tell=zip_src_op_tell,
};

int fs_register_zip(fsh*f,const char*name){
	size_t c=0;
	int r=0,e=0;
	zip_t*zip=NULL;
	fs_file_info info;
	zip_source_t*src=NULL;
	struct zip_error error;
	struct zip_ctx*ctx=NULL;
	char pn[sizeof(ctx->name)];
	if(!fsh_check(f))RET(EBADF);
	if(name&&!name[0])RET(EINVAL);
	MUTEX_LOCK(lock);
	if(name){
		if(strlen(name)>=sizeof(pn))
			DONE(ENAMETOOLONG);
		strncpy(pn,name,sizeof(pn)-1);
		if(list_search_one(
			opened_zip,
			zip_ctx_cmp,
			(void*)name
		))DONE(EEXIST);
	}else do{
		memset(pn,0,sizeof(pn));
		snprintf(pn,sizeof(pn)-1,"zip-%zu",c++);
	}while(list_search_one(opened_zip,zip_ctx_cmp,pn));
	if((r=fs_get_info(f,&info))!=0)DONE(r);
	if(
		info.type!=FS_TYPE_FILE_REG&&
		info.type!=FS_TYPE_FILE_LINK
	)DONE(EISDIR);
	zip_error_init(&error);
	if(!(src=zip_source_file_common_new(
		NULL,f,0,info.size,NULL,
		&ops_read,NULL,&error
	))){
		tlog_warn(
			"open zip source failed: %s",
			zip_error_strerror(&error)
		);
		DONE(zip_error_to_errno(error.zip_err));
	}
	if(!(zip=zip_open_from_source(src,0,&error))){
		tlog_warn(
			"open zip from source failed: %s",
			zip_error_strerror(&error)
		);
		DONE(zip_error_to_errno(error.zip_err));
	}
	if(!(ctx=malloc(sizeof(struct zip_ctx))))DONE(ENOMEM);
	memset(ctx,0,sizeof(struct zip_ctx));
	strncpy(ctx->name,pn,sizeof(ctx->name)-1);
	ctx->file=f,ctx->zip=zip;
	MUTEX_INIT(ctx->lock);
	fs_add_on_close(f,NULL,on_base_file_close,ctx);
	list_obj_add_new(&opened_zip,ctx);
	MUTEX_UNLOCK(lock);
	RET(0);
	done:e=errno;
	if(zip)zip_close(zip);
	else if(src)zip_source_free(src);
	zip_error_fini(&error);
	MUTEX_UNLOCK(lock);
	XRET(e,EIO);
}

void fsdrv_register_zip(bool deinit){
	if(deinit){
		MUTEX_LOCK(lock);
		list_free_all(opened_zip,free_zip_ctx);
		opened_zip=NULL;
		MUTEX_UNLOCK(lock);
		MUTEX_DESTROY(lock);
	}else{
		MUTEX_INIT(lock);
		fsdrv_register_dup(&fsdrv_zip);
	}
}

#endif

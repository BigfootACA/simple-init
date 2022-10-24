/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"

static bool fs_debug=false;

void fs_set_debug(bool debug){
	tlog_debug("set file system debug to %s",BOOL2STR(debug));
	fs_debug=debug;
}

void fs_close(fsh**f){
	list*l;
	if(!f||!*f)return;
	MUTEX_LOCK((*f)->lock);
	if(!fsh_check(*f)){
		MUTEX_UNLOCK((*f)->lock);
		return;
	}
	const fsdrv*drv=(*f)->driver,*use=drv;
	while(use&&!use->close)use=use->base;
	if((l=list_first((*f)->on_close)))do{
		LIST_DATA_DECLARE(p,l,struct fsh_hand_on_close*);
		if(!p||!p->callback)continue;
		p->callback(p->name,*f,p->user_data);
	}while((l=l->next));
	if(use&&use->close)use->close(drv,*f);
	memset((*f)->magic,0,sizeof((*f)->magic));
	MUTEX_UNLOCK((*f)->lock);
	fsh_free(f);
}

int fs_flush_locked(fsh*f){
	if(!fsh_check(f))RET(EBADF);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->flush)use=use->base;
	if(!use||use->readonly_fs)RET(EROFS);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(!drv->flush)RET(ENOSYS);
	RET(use->flush(drv,f));
}

int fs_exists_locked(
	fsh*f,
	const char*uri,
	bool*exists,
	bool lock
){
	int r;
	if(!uri||!exists)RET(EINVAL);
	if(f){
		if(lock)MUTEX_LOCK(f->lock);
		if(!fsh_check(f)){
			if(lock)MUTEX_UNLOCK(f->lock);
			RET(EBADF);
		}
	}
	r=fs_open_locked(
		f,NULL,uri,
		FILE_FLAG_ACCESS,
		false
	);
	if(r==ENOENT)errno=0,*exists=false;
	else if(r==0)*exists=true;
	if(f&&lock)MUTEX_UNLOCK(f->lock);
	RET(r);
}

int fs_exists_uri(url*uri,bool*exists){
	if(!uri||!exists)RET(EINVAL);
	const fsdrv*drv=fsdrv_lookup(uri),*use=drv;
	if(!drv)RET(EPROTONOSUPPORT);
	while(use&&!use->open)use=use->base;
	if(!use||!use->open)RET(ENOSYS);
	int r=use->open(drv,NULL,uri,FILE_FLAG_ACCESS);
	if(r==ENOENT)errno=0,*exists=false;
	else if(r==0)*exists=true;
	RET(r);
}

static int uri_parse(fsh*f,const char*uri,url*u){
	int r;
	char buff[PATH_MAX];
	if(!strstr(uri,"://")){
		if(f){
			url_copy(f->uri,u);
			url_set_path(u,NULL,0);
			url_set_query(u,NULL,0);
			url_set_fragment(u,NULL,0);
		}else url_set_scheme(u,"file",0);
		if(uri[0]!='/'){
			if(!f){
				const fsdrv*drv=fsdrv_lookup(u),*use=drv;
				if(!drv)RET(EPROTONOSUPPORT);
				while(use&&!use->getcwd)use=use->base;
				if(!use)RET(ENOTSUP);
				memset(buff,0,sizeof(buff));
				r=use->getcwd(drv,buff,sizeof(buff));
				if(r!=0)RET(r);
				url_set_path_decoded(u,buff,0);
			}
			if(!url_parse_relative_path(
				f?f->uri:u,u,uri,0
			))RET(EINVAL);
		}else url_set_path_decoded(u,uri,0);
	}else if(!url_parse(u,uri,0))EXRET(EINVAL);
	if(u->path)trim_path(u->path);
	RET(0);
}

static int path_to_uri(fsh*f,const char*uri,url**u){
	int r=-1;
	list*l;
	if(!uri||!u)RET(EINVAL);
	if(!(*u=url_new()))DONE(ENOMEM);
	MUTEX_LOCK(fsdrv_lock);
	if((l=list_first(fs_drivers)))do{
		const fsdrv*drv=LIST_DATA(l,fsdrv*),*use=drv;
		if(!drv)continue;
		while(use&&!use->uri_parse)use=use->base;
		if(!use)continue;
		r=use->uri_parse(drv,f,uri,*u);
	}while((l=l->next)&&r==-1);
	MUTEX_UNLOCK(fsdrv_lock);
	if(r==-1)r=uri_parse(f,uri,*u);
	if(r!=0)DONE(r);
	RET(0);
	done:
	if(*u)url_free(*u);
	*u=NULL;
	EXRET(EINVAL);
}

int fs_open_locked(
	fsh*f,
	fsh**nf,
	const char*uri,
	fs_file_flag flag,
	bool lock
){
	int r=-1;
	url*u=NULL;
	if(!uri)RET(EINVAL);
	if(f){
		if(!f->uri||!f->uri->scheme)RET(EINVAL);
		if(lock)MUTEX_LOCK(f->lock);
		if(!fsh_check(f)){
			if(lock)MUTEX_UNLOCK(f->lock);
			RET(EBADF);
		}
	}
	if(!fs_has_flag(flag,FILE_FLAG_ACCESS)&&!nf)DONE(EINVAL);
	if(
		fs_has_flag(flag,FILE_FLAG_WRITE)&&
		fs_has_flag(flag,FILE_FLAG_FOLDER)
	)DONE(EINVAL);
	r=path_to_uri(f,uri,&u);
	if(r!=0)DONE(r);
	r=fs_open_uri(nf,u,flag);
	url_free(u);
	if(f&&lock)MUTEX_UNLOCK(f->lock);
	RET(r);
	done:
	if(u)url_free(u);
	if(f&&lock)MUTEX_UNLOCK(f->lock);
	EXRET(EINVAL);
}

int fs_open_uri(fsh**nf,url*uri,fs_file_flag flag){
	int r;
	char*d=NULL;
	if(!uri)RET(EINVAL);
	const fsdrv*drv=fsdrv_lookup(uri),*use=drv;
	if(!drv)RET(EPROTONOSUPPORT);
	while(use&&!use->open)use=use->base;
	if(!use)RET(ENOTSUP);
	if(
		fs_has_flag(flag,FILE_FLAG_WRITE)&&
		fs_has_flag(flag,FILE_FLAG_FOLDER)
	)RET(EINVAL);
	if(use->readonly_fs||drv->readonly_fs){
		if(fs_has_flag(flag,FILE_FLAG_WRITE))RET(EROFS);
		if(fs_has_flag(flag,FILE_FLAG_CREATE))RET(EROFS);
	}
	if(!use->open)RET(ENOSYS);
	if(!fs_has_flag(flag,FILE_FLAG_ACCESS)){
		if(!nf)RET(EINVAL);
		*nf=fsh_get_new(drv,uri,NULL,drv->hand_data_size,flag);
		if(!*nf)EXRET(ENOMEM);
	}else if(nf)RET(EINVAL);
	r=use->open(drv,nf?*nf:NULL,uri,flag);
	if(r!=0){
		if(r!=ENOENT){
			tlog_warn(
				"open from driver %p (%s) failed: %s",
				drv,drv->protocol,strerror(r)
			);
			if(fs_debug){
				d=url_dump_alloc(uri);
				logger_print(LEVEL_WARNING,TAG,d);
				free(d);
			}
		}
		if(nf)fsh_free(nf);
	}
	RET(r);
}

int fs_readdir_locked(fsh*f,fs_file_info*info){
	if(!fsh_check(f))RET(EBADF);
	if(!info)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->readdir)use=use->base;
	if(!use||!use->readdir)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(ENOTDIR);
	RET(use->readdir(drv,f,info));
}

int fs_read_locked(fsh*f,void*buffer,size_t btr,size_t*br){
	if(!fsh_check(f))RET(EBADF);
	if(!buffer||!br)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->read)use=use->base;
	if(!use||!use->read)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->read(drv,f,buffer,btr,br));
}

int fs_read_all_locked(fsh*f,void**buffer,size_t*br){
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->read_all)use=use->base;
	if(!use||!use->read_all)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->read_all(drv,f,buffer,br));
}

int fs_write_locked(fsh*f,void*buffer,size_t btw,size_t*bw){
	if(!fsh_check(f))RET(EBADF);
	if(!buffer||!bw)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->write)use=use->base;
	if(!use||!use->write)RET(ENOSYS);
	if(use->readonly_fs||drv->readonly_fs)RET(EROFS);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->write(drv,f,buffer,btw,bw));
}

int fs_wait_locked(
	fsh**gots,
	fsh**waits,
	size_t cnt,
	long timeout,
	fs_wait_flag flag,
	bool lock
){
	fsh*f=NULL;
	const fsdrv*drv=NULL;
	if(!waits)RET(EINVAL);
	if(cnt==0)for(size_t i=0;waits[i];i++)cnt++;
	if(cnt<=0)RET(EINVAL);
	if(gots)memset(gots,0,sizeof(fsh*)*(cnt+1));
	for(size_t i=0;i<cnt&&(f=waits[i]);i++){
		if(!fsh_check(f))RET(EBADF);
		if(!fs_has_flag(f->flags,FILE_FLAG_NON_BLOCK))RET(EPERM);
		if(i!=0&&drv!=f->driver)RET(EINVAL);
		if(!(drv=f->driver))RET(EINVAL);
	}
	if(!drv)RET(EBADF);
	if(lock){
		while(true){
			bool found=false;
			for(size_t i=0;i<cnt&&(f=waits[i]);i++){
				if(MUTEX_TRYLOCK(f->lock)!=0){
					found=true;
					break;
				}
				MUTEX_UNLOCK(f->lock);
			}
			if(!found)break;
			MUTEX_LOCK(f->lock);
		}
		for(size_t i=0;i<cnt&&(f=waits[i]);i++)
			MUTEX_LOCK(f->lock);
	}
	const fsdrv*use=drv;
	while(use&&!use->wait)use=use->base;
	int r=0;
	if(!use||!use->wait)r=ENOSYS;
	else r=use->wait(drv,gots,waits,cnt,timeout,flag);
	if(lock)for(size_t i=0;i<cnt&&(f=waits[i]);i++)
		MUTEX_UNLOCK(f->lock);
	RET(r);
}

int fs_seek_locked(fsh*f,size_t pos,int whence){
	if(!fsh_check(f))RET(EBADF);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->seek)use=use->base;
	if(!use||!use->seek)RET(ENOSYS);
	RET(use->seek(drv,f,pos,whence));
}

int fs_tell_locked(fsh*f,size_t*pos){
	if(!fsh_check(f))RET(EBADF);
	if(!pos)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->tell)use=use->base;
	if(!use||!use->tell)RET(ENOSYS);
	RET(use->tell(drv,f,pos));
}

int fs_map_locked(
	fsh*f,
	void**buffer,
	size_t off,
	size_t*size,
	fs_file_flag flag
){
	if(!fsh_check(f))RET(EBADF);
	if(!buffer||!size)RET(EINVAL);
	if(fs_has_flag(flag,FILE_FLAG_CREATE))RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->map)use=use->base;
	if(!use)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(!fs_has_flag(flag,FILE_FLAG_READ))RET(EINVAL);
	if(fs_has_flag(flag,FILE_FLAG_WRITE)){
		if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EINVAL);
		if(use->readonly_fs||drv->readonly_fs)RET(EROFS);
	}
	if(!use->map)RET(ENOSYS);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->map(drv,f,buffer,off,size,flag));
}

int fs_unmap_locked(fsh*f,void*buffer,size_t size){
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->unmap)use=use->base;
	if(!use||!use->unmap)RET(ENOSYS);
	RET(use->unmap(drv,f,buffer,size));
}

int fs_get_info_locked(fsh*f,fs_file_info*info){
	int r=0;
	time_t t;
	if(!fsh_check(f))RET(EBADF);
	if(!info)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->get_info)use=use->base;
	if(!use||!use->get_info)RET(ENOSYS);
	t=time(NULL);
	if(t-f->cache_info_time>=use->cache_info_time)
		r=use->get_info(drv,f,&f->cached_info);
	if(r==0){
		f->cache_info_time=t;
		memcpy(
			info,&f->cached_info,
			sizeof(fs_file_info)
		);
	}
	RET(r);
}

int fs_get_type_locked(fsh*f,fs_type*type){
	if(!fsh_check(f))RET(EBADF);
	if(!type)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->get_type)use=use->base;
	if(!use||!use->get_type)RET(ENOSYS);
	RET(use->get_type(drv,f,type));
}

int fs_get_size_locked(fsh*f,size_t*out){
	if(!fsh_check(f))RET(EBADF);
	if(!out)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->get_size)use=use->base;
	if(!use||!use->get_size)RET(ENOSYS);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->get_size(drv,f,out));
}

int fs_get_name_locked(fsh*f,char*buff,size_t buff_len){
	if(!fsh_check(f))RET(EBADF);
	if(!buff||buff_len<=0)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->get_name)use=use->base;
	if(!use||!use->get_name)RET(ENOSYS);
	RET(use->get_name(drv,f,buff,buff_len));
}

int fs_set_size_locked(fsh*f,size_t size){
	if(!fsh_check(f))RET(EBADF);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->resize)use=use->base;
	if(!use||use->readonly_fs||drv->readonly_fs)RET(EROFS);
	if(!use->resize)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	RET(use->resize(drv,f,size));
}

int fs_get_url_locked(fsh*f,url**out){
	if(!fsh_check(f))RET(EBADF);
	if(!out)RET(EINVAL);
	*out=url_dup(f->uri);
	if(!*out)EXRET(ENOMEM);
	return 0;
}

int fs_get_path_locked(fsh*f,char*buff,size_t len){
	if(!fsh_check(f))RET(EBADF);
	if(!buff||len<=0)RET(EINVAL);
	if(!url_generate(buff,len,f->uri))EXRET(ENOMEM);
	RET(0);
}

int fs_get_path_alloc_locked(fsh*f,char**buff){
	if(!fsh_check(f))RET(EBADF);
	if(!buff)RET(EINVAL);
	*buff=url_generate_alloc(f->uri);
	if(!*buff)EXRET(ENOMEM);
	RET(0);
}

bool fs_file_has_feature(fsh*f,fs_feature feature){
	fs_feature features=0;
	if(fs_get_features(f,&features)!=0)return false;
	return (features&feature)==feature;
}

int fs_get_features_locked(fsh*f,fs_feature*features){
	if(!fsh_check(f))RET(EBADF);
	if(!features)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->get_features)use=use->base;
	if(!use||!use->get_features)RET(ENOSYS);
	RET(use->get_features(drv,f,features));
}

int fs_rename_uri_locked(fsh*f,url*to){
	if(!fsh_check(f))RET(EBADF);
	if(!to||!to->path)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->rename)use=use->base;
	if(!use||use->readonly_fs||drv->readonly_fs)RET(EROFS);
	if(!use->rename)RET(ENOSYS);
	RET(use->rename(drv,f,to));
}

int fs_rename_locked(fsh*f,const char*to){
	int r=0;
	url*u=NULL;
	if(!fsh_check(f))RET(EBADF);
	if(!to)RET(EINVAL);
	r=path_to_uri(f,to,&u);
	if(r!=0)RET(r);
	r=fs_rename_uri_locked(f,u);
	url_free(u);
	RET(r);
}

int fs_rename_at_locked(fsh*f,const char*from,const char*to){
	int r=0;
	fsh*nf=NULL;
	if(!fsh_check(f))RET(EBADF);
	if(!to)RET(EINVAL);
	if(!from)return fs_rename_locked(f,to);
	r=fs_open_locked(f,&nf,from,FILE_FLAG_READ,false);
	if(r!=0)RET(r);
	r=fs_rename_locked(nf,to);
	fs_close(&f);
	RET(r);
}

int fs_ioctl_va_locked(fsh*f,fs_ioctl_id id,va_list args){
	if(!fsh_check(f))RET(EBADF);
	if(id<=_FS_IOCTL_NONE)RET(EINVAL);
	const fsdrv*drv=f->driver,*use=drv;
	while(use&&!use->ioctl)use=use->base;
	if(!use||!use->ioctl)RET(ENOSYS);
	RET(use->ioctl(drv,f,id,args));
}

int fs_ioctl_locked(fsh*f,fs_ioctl_id id,...){
	va_list args;
	va_start(args,id);
	int r=fs_ioctl_va_locked(f,id,args);
	va_end(args);
	return r;
}

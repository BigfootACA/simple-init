/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"../fs_internal.h"

#define MAP_INFO_MAGIC "!FSMAP!!"
struct map_info{
	char magic[8];
	fs_file_flag flag;
	fsh*hand;
	size_t offset;
	size_t size;
	char data[];
};

static int fsdrv_map(
	const fsdrv*drv,
	fsh*f,
	void**buffer,
	size_t off,
	size_t*size,
	fs_file_flag flag
){
	int e;
	size_t bs=0,read=0,fs=0,lp=0;
	struct map_info*map=NULL;
	if(!fsh_check(f))RET(EBADF);
	if(!drv||!f->driver||!buffer||!size)RET(EINVAL);
	*buffer=NULL;
	if(fs_has_flag(flag,FILE_FLAG_CREATE))RET(EINVAL);
	else if(fs_has_flag(flag,FILE_FLAG_FOLDER))RET(EISDIR);
	else if(fs_has_flag(flag,FILE_FLAG_WRITE)){
		if(!f->driver->write)DONE(trlog_verbose(
			EROFS,
			"hand %p driver %p unsupported write",
			f,f->driver
		));
	}else if(!fs_has_flag(flag,FILE_FLAG_READ))goto done;
	if((errno=fs_get_size_locked(f,&fs))!=0)
		EDONE(telog_verbose("hand %p get file size for map failed",f));
	if(*size==0)*size=fs;
	else if(off+*size>fs)EDONE(telog_verbose(
		"hand %p request map range out of file %zu",
		f,*size
	));
	bs=sizeof(struct map_info)+(*size)+1;
	if(!(map=malloc(bs)))EDONE(telog_verbose(
		"hand %p alloc buffer %zu bytes for map failed",
		f,bs
	));
	memcpy(map->magic,MAP_INFO_MAGIC,sizeof(map->magic));
	map->hand=f,map->size=*size,map->flag=flag,map->offset=off;
	if(!(errno=fs_tell_locked(f,&lp)))
		EDONE(telog_verbose("hand %p tell for map failed",f));
	if(!(errno=fs_seek_locked(f,off,SEEK_SET)))
		EDONE(telog_verbose("hand %p seek for map failed",f));
	if(!(errno=fs_read_locked(f,map->data,*size,&read)))
		EDONE(telog_verbose("hand %p read for map failed",f));
	if(!(errno=fs_seek_locked(f,lp,SEEK_SET)))
		EDONE(telog_verbose("hand %p seek back for map failed",f));
	if(read!=*size)DONE(trlog_verbose(
		EIO,
		"hand %p read file size mismatch %zu != %zu",
		f,read,*size
	));
	map->data[*size]=0,*buffer=map->data;
	RET(0);
	done:
	e=errno;
	if(map)free(map);
	*buffer=NULL,*size=0;
	XRET(e,EINVAL);
}

static int fsdrv_unmap(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t size
){
	size_t lp=0,write=0;
	if(!fsh_check(f))RET(EBADF);
	if(!drv||!buffer)RET(EINVAL);
	struct map_info*map=buffer-sizeof(struct map_info);
	if(memcmp(map->magic,MAP_INFO_MAGIC,sizeof(map->magic)))
		RET(trlog_verbose(EINVAL,"hand %p buffer %p map info magic mismatch",f,buffer));
	if(map->size!=size)RET(trlog_verbose(
		EINVAL,"hand %p buffer %p map info size mismatch %zu != %zu",
		f,buffer,size,map->size
	));
	if(map->hand!=f)RET(trlog_verbose(
		EINVAL,"hand %p buffer %p map info handler mismatch %p",
		f,buffer,map->hand
	));
	if(fs_has_flag(map->flag,FILE_FLAG_WRITE)){
		if(!(errno=fs_tell_locked(f,&lp)))
			RET(terlog_verbose(errno,"hand %p tell for unmap write back failed",f));
		if(!(errno=fs_seek_locked(f,map->offset,SEEK_SET)))
			RET(terlog_verbose(errno,"hand %p seek for unmap write back failed",f));
		if(!(errno=fs_write_locked(f,map->data,map->size,&write)))
			RET(terlog_verbose(errno,"hand %p unmap write back failed",f));
		if(!(errno=fs_seek_locked(f,lp,SEEK_SET)))
			RET(terlog_verbose(errno,"hand %p seek back for unmap write back failed",f));
		if(write!=map->size)RET(trlog_verbose(
			EIO,"hand %p unmap write back mismatch %zu != %zu",f,write,map->size
		));
	}
	free(map);
	RET(0);
}

static bool fsdrv_is_compatible(const fsdrv*drv,url*uri){
	if(!uri||!drv||!uri->scheme||!drv->protocol[0])return false;
	return strcasecmp(uri->scheme,drv->protocol)==0;
}

static int fsdrv_get_size(const fsdrv*drv,fsh*f,size_t*out){
	fs_file_info info;
	if(!fsh_check(f))RET(EBADF);
	if(!drv||!out)RET(EINVAL);
	memset(&info,0,sizeof(info));
	if((errno=fs_get_info_locked(f,&info))!=0)return terlog_verbose(
		errno,"hand %p get info for get size failed",f
	);
	*out=info.size;
	RET(0);
}

static int fsdrv_get_name(
	const fsdrv*drv,
	fsh*f,
	char*buff,
	size_t buff_len
){
	fs_file_info info;
	if(!fsh_check(f))RET(EBADF);
	if(!drv||!buff||buff_len<=0)RET(EINVAL);
	memset(&info,0,sizeof(info));
	if((errno=fs_get_info_locked(f,&info))!=0)return terlog_verbose(
		errno,"hand %p get info for get name failed",f
	);
	memset(buff,0,buff_len);
	strncpy(buff,info.name,buff_len-1);
	RET(0);
}

static int fsdrv_get_type(const fsdrv*drv,fsh*f,fs_type*type){
	fs_file_info info;
	if(!fsh_check(f))RET(EBADF);
	if(!drv||!type)RET(EINVAL);
	memset(&info,0,sizeof(info));
	if((errno=fs_get_info_locked(f,&info))!=0)return terlog_verbose(
		errno,"hand %p get info for get type failed",f
	);
	*type=info.type;
	RET(0);
}

static int fsdrv_read_all(const fsdrv*drv,fsh*f,void**buffer,size_t*size){
	int r;
	void*buff=NULL,*b;
	size_t len=0,bs=FS_BUF_SIZE,buf=0;
	if(!fsh_check(f))RET(EBADF);
	if(!buffer||!drv)RET(EINVAL);
	fs_seek_locked(f,0,SEEK_SET);
	if(fs_get_size_locked(f,&len)==0&&len!=0){
		r=fs_full_read_alloc_locked(f,buffer,len);
		if(size)*size=len;
	}else{
		if(!(buff=malloc(bs)))RET(ENOMEM);
		while(true){
			len=0;
			if(!(b=realloc(buff,bs))){
				free(buff);
				RET(ENOMEM);
			}else buff=b;
			if((r=fs_read_locked(
				f,buff+buf,
				FS_BUF_SIZE,
				&len
			))!=0)break;
			if(len==0)break;
			buf+=len;
			if(buf>=bs)bs+=FS_BUF_SIZE;
		}
		if(size)(*size)+=len;
		*buffer=buff;
	}
	RET(r);
}

static int fsdrv_get_features(
	const fsdrv*drv,
	fsh*f __attribute__((unused)),
	fs_feature*features
){
	if(!drv||!features)RET(EINVAL);
	*features=drv->features;
	RET(0);
}

const fsdrv fsdrv_template={
	.magic=FS_DRIVER_MAGIC,
	.cache_info_time=60,
	.is_compatible=fsdrv_is_compatible,
	.read_all=fsdrv_read_all,
	.map=fsdrv_map,
	.unmap=fsdrv_unmap,
	.get_size=fsdrv_get_size,
	.get_name=fsdrv_get_name,
	.get_type=fsdrv_get_type,
	.get_features=fsdrv_get_features,
};

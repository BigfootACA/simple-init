/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"str.h"
#include"../fs_internal.h"

static void fsdrv_close(const fsdrv*drv,fsh*f){
	if(!f||!drv||drv!=f->driver)return;
	fs_close((fsh**)&f->data);
}

static int fsdrv_flush(const fsdrv*drv,fsh*f){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_flush(f->data));
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	int r=0;
	fsh*f=NULL;
	if(!drv||!uri||!uri->path)RET(EINVAL);
	if(uri->path[0]!='/'||!uri->path[1])RET(EINVAL);
	r=fs_open(drv->data,&f,uri->path+1,flags);
	if(r==0&&f&&nf)nf->data=f;
	RET(r);
}

static int fsdrv_read(
	const fsdrv*drv,fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_read(f->data,buffer,btr,br));
}

static int fsdrv_read_all(
	const fsdrv*drv,
	fsh*f,
	void**buffer,
	size_t*size
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_read_all(f->data,buffer,size));
}

static int fsdrv_readdir(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_readdir(f->data,info));
}

static int fsdrv_write(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btw,
	size_t*bw
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_write(f->data,buffer,btw,bw));
}

static int fsdrv_wait(
	const fsdrv*drv,
	fsh**gots,
	fsh**waits,
	size_t cnt,
	long timeout,
	fs_wait_flag flag
){
	int r=0;
	void*d=NULL;
	size_t size,i,c,p;
	fsh**ws=NULL,**gs=NULL;
	if(!drv||!waits)RET(EINVAL);
	if(cnt==0)for(i=0;waits[i];i++)cnt++;
	if(cnt<=0)RET(EINVAL);
	if(gots)memset(gots,0,sizeof(fsh*)*(cnt+1));
	size=sizeof(fsh*)*(cnt+1);
	if(!(d=malloc(size*2)))RET(ENOMEM);
	memset(d,0,size*2);
	ws=d,gs=d+size;
	for(i=0;i<cnt&&waits[i];i++)
		ws[i]=waits[i]->data;
	r=fs_wait(gs,ws,cnt,timeout,flag);
	if(gots)for(i=0,c=0;i<cnt&&c<cnt;i++)
		for(p=0;p<cnt&&waits[p];p++)
			if(gs[i]==waits[p]->data)
				gots[c++]=waits[p];
	free(d);
	RET(r);
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_seek(f->data,pos,whence));
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_tell(f->data,pos));
}

static int fsdrv_map(
	const fsdrv*drv,
	fsh*f,
	void**buffer,
	size_t off,
	size_t*size,
	fs_file_flag flag
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_map(f->data,buffer,off,size,flag));
}

static int fsdrv_unmap(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t len
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_unmap(f->data,buffer,len));
}

static int fsdrv_get_info(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_get_info(f->data,info));
}

static int fsdrv_get_type(
	const fsdrv*drv,
	fsh*f,
	fs_type*type
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_get_type(f->data,type));
}

static int fsdrv_get_size(
	const fsdrv*drv,
	fsh*f,
	size_t*out
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_get_size(f->data,out));
}

static int fsdrv_get_name(
	const fsdrv*drv,
	fsh*f,
	char*buff,
	size_t buff_len
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_get_name(f->data,buff,buff_len));
}

static int fsdrv_get_features(
	const fsdrv*drv,
	fsh*f,
	fs_feature*features
){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_get_features(f->data,features));
}

static int fsdrv_resize(const fsdrv*drv,fsh*f,size_t size){
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	RET(fs_set_size(f->data,size));
}

static int fsdrv_getcwd(
	const fsdrv*drv,
	char*buff,
	size_t buff_len
){
	fsh*f;
	const fsdrv*tgt,*use;
	if(!drv||!(f=drv->data))RET(EINVAL);
	tgt=f->driver,use=tgt;
	while(tgt&&!tgt->getcwd)tgt=tgt->base;
	if(!tgt||!use)RET(ENOSYS);
	RET(use->getcwd(tgt,buff,buff_len));
}

static fsdrv fsdrv_overlay={
	.magic=FS_DRIVER_MAGIC,
	.cache_info_time=0,
	.base=&fsdrv_template,
	.close=fsdrv_close,
	.flush=fsdrv_flush,
	.open=fsdrv_open,
	.read=fsdrv_read,
	.read_all=fsdrv_read_all,
	.readdir=fsdrv_readdir,
	.write=fsdrv_write,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.map=fsdrv_map,
	.unmap=fsdrv_unmap,
	.get_info=fsdrv_get_info,
	.get_type=fsdrv_get_type,
	.get_size=fsdrv_get_size,
	.get_name=fsdrv_get_name,
	.get_features=fsdrv_get_features,
	.resize=fsdrv_resize,
	.wait=fsdrv_wait,
	.getcwd=fsdrv_getcwd,
};

static void on_base_file_close(
	const char*name,
	fsh*f,
	void*data
){
	fsdrv*d=data;
	if(!d||!f||!name)return;
	d->data=NULL;
	memset(d->protocol,0,sizeof(d->protocol));
}

int fsdrv_add_overlay(fsh*f,const char*proto){
	fsdrv*drv;
	if(!fsh_check(f))RET(EBADF);
	if(!proto)RET(EINVAL);
	if(fsdrv_lookup_by_protocol(proto))RET(EEXIST);
	if(!(drv=malloc(sizeof(fsdrv))))RET(ENOMEM);
	memcpy(drv,&fsdrv_overlay,sizeof(fsdrv));
	strncpy(drv->protocol,proto,sizeof(drv->protocol)-1);
	drv->features=f->driver->features;
	fs_add_on_close(f,proto,on_base_file_close,drv);
	drv->data=f;
	return fsdrv_register(drv);
}

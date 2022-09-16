/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<sys/mman.h>
#include<sys/stat.h>
#include<dirent.h>
#include"str.h"
#include"system.h"
#include"../fs_internal.h"

static fsdrv fsdrv_posix;
struct fsd{
	DIR*dir;
	char name[256];
};

static void init_info(fs_file_info*info){
	memset(info,0,sizeof(fs_file_info));
	memcpy(
		info->magic,
		FS_INFO_MAGIC,
		sizeof(info->magic)
	);
}

static void dirent_to_info(struct dirent*r,fs_file_info*info){
	strncpy(info->name,r->d_name,sizeof(info->name)-1);
	switch(r->d_type){
		case DT_FIFO:info->type=FS_TYPE_FILE_FIFO;break;
		case DT_CHR:info->type=FS_TYPE_FILE_CHAR;break;
		case DT_DIR:info->type=FS_TYPE_FILE_FOLDER;break;
		case DT_BLK:info->type=FS_TYPE_FILE_BLOCK;break;
		case DT_REG:info->type=FS_TYPE_FILE_REG;break;
		case DT_LNK:info->type=FS_TYPE_FILE_LINK;break;
		case DT_SOCK:info->type=FS_TYPE_FILE_SOCKET;break;
		case DT_WHT:info->type=FS_TYPE_FILE_WHITEOUT;break;
		default:info->type=FS_TYPE_NONE;break;
	}
}

static void stat_to_info(struct stat*s,fs_file_info*info){
	if(S_ISFIFO(s->st_mode))
		info->type=FS_TYPE_FILE_FIFO;
	else if(S_ISCHR(s->st_mode))
		info->type=FS_TYPE_FILE_CHAR;
	else if(S_ISDIR(s->st_mode))
		info->type=FS_TYPE_FILE_FOLDER;
	else if(S_ISBLK(s->st_mode))
		info->type=FS_TYPE_FILE_BLOCK;
	else if(S_ISREG(s->st_mode))
		info->type=FS_TYPE_FILE_REG;
	else if(S_ISLNK(s->st_mode))
		info->type=FS_TYPE_FILE_LINK;
	else if(S_ISSOCK(s->st_mode))
		info->type=FS_TYPE_FILE_SOCKET;
	else info->type=FS_TYPE_NONE;
	info->size=s->st_size;
	info->mode=s->st_mode&0xFFFF;
	info->owner=s->st_uid;
	info->group=s->st_gid;
	info->device=s->st_dev;
	info->atime=s->st_atime;
	info->ctime=s->st_ctime;
	info->mtime=s->st_mtime;
}

static void readlink_info(fs_file_info*info){
	if(!info||!info->name[0]||!info->parent)return;
	if(info->type!=FS_TYPE_FILE_LINK)return;
	if(info->parent->fd<0)return;
	readlinkat(
		info->parent->fd,info->name,
		info->target,sizeof(info->target)
	);
}

void fsdrv_posix_close(const fsdrv*drv,fsh*f){
	errno=0;
	if(!f||!drv||drv!=f->driver)return;
	struct fsd*d=f->data;
	if(d&&d->dir)closedir(d->dir);
	else if(f->fd>0)close(f->fd);
}

static int fsdrv_flush(const fsdrv*drv,fsh*f){
	errno=0;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EROFS);
	if(f->fd<0)RET(EBADF);
	if(fdatasync(f->fd)!=0)EXRET(EIO);
	ERET(0);
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	size_t l;
	struct fsd*nd=NULL;
	int flag=O_CLOEXEC,e;
	char buf[PATH_MAX],*c;
	if(!drv||!uri||!uri->path)RET(EINVAL);
	if(fs_has_flag(flags,FILE_FLAG_ACCESS)){
		int r=access(uri->path,F_OK);
		if(r!=0)EXRET(ENOENT);
		RET(0);
	}
	if(!nf||!(nd=nf->data))RET(EINVAL);
	memset(buf,0,sizeof(buf));
	strncpy(buf,uri->path,sizeof(buf)-1);
	for(l=strlen(buf);buf[l-1]=='/';l--)buf[l-1]=0;
	if((c=strrchr(buf,'/')))strncpy(
		nd->name,c+1,sizeof(nd->name)-1
	);
	if(fs_has_flag(flags,FILE_FLAG_FOLDER)){
		flag|=O_DIRECTORY|O_RDONLY;
		if(fs_has_flag(flags,FILE_FLAG_CREATE)){
			mode_t mode=flags&_FILE_FLAG_MODE_MASK;
			if(mkdir(uri->path,mode)!=0)goto fail;
		}
		if((nf->fd=open(uri->path,flag))<=0)goto fail;
		if(!(nd->dir=fdopendir(nf->fd)))goto fail;
		if((l=strlen(nf->uri->path))<=0)goto fail;
		if(nf->uri->path[l-1]!='/')
			strcat(nf->uri->path,"/");
	}else{
		if(fs_has_flag(flags,FILE_FLAG_READWRITE))flag|=O_RDWR;
		else{
			if(fs_has_flag(flags,FILE_FLAG_READ))flag|=O_RDONLY;
			if(fs_has_flag(flags,FILE_FLAG_WRITE))flag|=O_WRONLY;
		}
		if(fs_has_flag(flags,FILE_FLAG_CREATE))flag|=O_CREAT;
		if(fs_has_flag(flags,FILE_FLAG_TRUNCATE))flag|=O_TRUNC;
		if((nf->fd=open(uri->path,flag))<0)goto fail;
	}
	RET(0);
	fail:
	e=errno;
	fsdrv_posix_close(drv,nf);
	XRET(e,EIO);
}

int fsdrv_posix_read(
	const fsdrv*drv,fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	if(!f||!drv||!buffer)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	if(f->fd<0)RET(EBADF);
	ssize_t rs=read(f->fd,buffer,btr);
	if(rs<0)EXRET(EIO);
	else if(br)*br=rs;
	ERET(0);
}

static int fsdrv_readdir(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	struct fsd*d;
	struct stat st;
	struct dirent*r;
	if(!f||!drv||!(d=f->data))RET(EINVAL);
	if(f->driver!=drv||!info)RET(EINVAL);
	if(!fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(ENOTDIR);
	if(!d->dir||f->fd<=0)RET(EBADF);
	init_info(info);
	errno=0;
	while((r=readdir(d->dir))){
		if(strcmp(r->d_name,".")==0)continue;
		if(strcmp(r->d_name,"..")==0)continue;
		dirent_to_info(r,info);
		if(fstatat(f->fd,r->d_name,&st,AT_SYMLINK_NOFOLLOW)==0)
			stat_to_info(&st,info);
		info->features=drv->features;
		info->parent=f;
		readlink_info(info);
		RET(0);
	}
	EXRET(EOF);
}

int fsdrv_posix_write(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btw,
	size_t*bw
){
	errno=0;
	if(!drv||!buffer)RET(EINVAL);
	if(!f||f->driver!=drv)RET(EINVAL);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	if(f->fd<0)RET(EBADF);
	ssize_t ws=write(f->fd,buffer,btw);
	if(ws<0)EXRET(EIO);
	else if(bw)*bw=ws;
	RET(0);
}

int fsdrv_posix_wait(
	const fsdrv*drv,
	fsh**gots,
	fsh**waits,
	size_t cnt,
	long timeout,
	fs_wait_flag flag
){
	int r=0;
	fsh*h=NULL;
	bool has=false;
	struct timeval t;
	fd_set infds,outfds,errfds;
	FD_ZERO(&infds);
	FD_ZERO(&outfds);
	FD_ZERO(&errfds);
	if(!drv||!waits)RET(EINVAL);
	if(
		!(flag&FILE_WAIT_READ)&&
		!(flag&FILE_WAIT_WRITE)&&
		!(flag&FILE_WAIT_ERROR)
	)RET(EINVAL);
	if(gots)memset(gots,0,sizeof(fsh*)*(cnt+1));
	if(timeout>=0){
		t.tv_sec=timeout/1000;
		t.tv_usec=(timeout%1000)*1000;
	}
	for(size_t i=0;i<cnt&&(h=waits[i]);i++){
		if(!fsh_check(h))RET(EINVAL);
		if(h->driver!=drv)RET(EINVAL);
		if(h->fd<0)RET(EBADF);
		if(flag&FILE_WAIT_READ)
			FD_SET(h->fd,&infds);
		if(flag&FILE_WAIT_WRITE)
			FD_SET(h->fd,&outfds);
		if(flag&FILE_WAIT_ERROR)
			FD_SET(h->fd,&errfds);
		has=true;
	}
	if(!has)RET(EINVAL);
	errno=0,r=select(
		FD_SETSIZE,
		flag&FILE_WAIT_READ?&infds:NULL,
		flag&FILE_WAIT_WRITE?&outfds:NULL,
		flag&FILE_WAIT_ERROR?&errfds:NULL,
		timeout>=0?&t:NULL
	);
	if(r<0)EXRET(EIO);
	if(gots)for(size_t i=0,c=0;i<cnt&&c<cnt;i++){
		bool found=false;
		if(!(h=waits[i]))EXRET(EINVAL);
		if(FD_ISSET(h->fd,&infds))found=true;
		if(FD_ISSET(h->fd,&outfds))found=true;
		if(FD_ISSET(h->fd,&errfds))found=true;
		if(found)gots[c++]=h;
	}
	RET(0);
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	errno=0;
	struct fsd*d;
	if(!f||!(d=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(f->fd>0)lseek(f->fd,pos,whence);
	else if(d->dir&&whence==SEEK_SET)seekdir(d->dir,pos);
	else RET(EBADF);
	return errno;
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	errno=0;
	struct fsd*d;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!(d=f->data)||!pos)RET(EINVAL);
	if(d->dir)*pos=telldir(d->dir);
	else if(f->fd>0)*pos=lseek(f->fd,0,SEEK_CUR);
	else RET(EBADF);
	return errno;
}

static int fsdrv_map(
	const fsdrv*drv,
	fsh*f,
	void**buffer,
	size_t off,
	size_t*size,
	fs_file_flag flag
){
	errno=0;
	void*buf;
	int protect=PROT_READ,flags=MAP_SHARED;
	if(!drv||!buffer||!size)RET(EINVAL);
	if(!f||f->driver!=drv)RET(EINVAL);
	if(!fs_has_flag(flag,FILE_FLAG_READ))RET(EINVAL);
	if(fs_has_flag(flag,FILE_FLAG_WRITE))protect|=PROT_WRITE;
	if(fs_has_flag(flag,FILE_FLAG_EXECUTE))protect|=PROT_EXEC;
	if(fs_has_flag(flag,FILE_FLAG_SHARED))flags=MAP_SHARED;
	if(fs_has_flag(flag,FILE_FLAG_PRIVATE))flags=MAP_PRIVATE;
	if(fs_has_flag(flag,FILE_FLAG_FIXED))flags=MAP_FIXED;
	if(*size==0&&fs_get_size(f,size)!=0)EXRET(EINVAL);
	if(*size==0||f->fd<=0)RET(EBADF);
	buf=mmap(NULL,*size,protect,flags,f->fd,off);
	if(!buf||buf==MAP_FAILED)EXRET(EFAULT);
	*buffer=buf;
	RET(0);
}

static int fsdrv_unmap(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t len
){
	errno=0;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!buffer||len<=0)RET(EINVAL);
	if(munmap(buffer,len)!=0)EXRET(EIO);
	RET(0);
}

static int fsdrv_get_info(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	errno=0;
	struct fsd*d;
	struct stat st;
	if(!f||!(d=f->data)||!info)RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(f->fd<=0)RET(EBADF);
	if(fstat(f->fd,&st)!=0)EXRET(EIO);
	init_info(info);
	stat_to_info(&st,info);
	info->features=drv->features;
	strncpy(
		info->name,d->name,
		sizeof(info->name)-1
	);
	info->parent=f;
	readlink_info(info);
	RET(0);
}

static int fsdrv_resize(const fsdrv*drv,fsh*f,size_t pos){
	errno=0;
	struct fsd*d;
	if(!f||!(d=f->data))RET(EINVAL);
	if(!drv||f->driver!=drv)RET(EINVAL);
	if(d->dir)seekdir(d->dir,pos);
	else if(f->fd>0)lseek(f->fd,pos,SEEK_SET);
	else RET(EBADF);
	return errno;
}

static int fsdrv_rename(const fsdrv*drv,fsh*f,url*to){
	if(!f||!f->uri||!f->uri->path||!to)RET(EINVAL);
	if(!to->path||!drv||f->driver!=drv)RET(EINVAL);
	if(rename(f->uri->path,to->path)!=0)EXRET(EINVAL);
	RET(0);
}

static int fsdrv_getcwd(
	const fsdrv*drv,
	char*buff,
	size_t buff_len
){
	size_t len;
	if(!drv||!buff||buff_len<=0)RET(EINVAL);
	memset(buff,0,buff_len);
	if(!getcwd(buff,buff_len))EXRET(EIO);
	if((len=strlen(buff))<=0)RET(ENOENT);
	if(buff[len-1]!='/')strlcat(buff,"/",buff_len);
	RET(0);
}

void fsdrv_register_posix(bool deinit){
	if(!deinit)fsdrv_register_dup(&fsdrv_posix);
}

static fsdrv fsdrv_posix={
	.magic=FS_DRIVER_MAGIC,
	.base=&fsdrv_template,
	.cache_info_time=5,
	.hand_data_size=sizeof(struct fsd),
	.protocol="file",
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_WRITABLE|
		FS_FEATURE_SEEKABLE|
		FS_FEATURE_UNIX_PERM|
		FS_FEATURE_UNIX_DEVICE|
		FS_FEATURE_NON_BLOCK|
		FS_FEATURE_HAVE_STAT|
		FS_FEATURE_HAVE_SIZE|
		FS_FEATURE_HAVE_PATH|
		FS_FEATURE_HAVE_TIME|
		FS_FEATURE_HAVE_FOLDER,
	.flush=fsdrv_flush,
	.close=fsdrv_posix_close,
	.open=fsdrv_open,
	.read=fsdrv_posix_read,
	.readdir=fsdrv_readdir,
	.write=fsdrv_posix_write,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.map=fsdrv_map,
	.unmap=fsdrv_unmap,
	.get_info=fsdrv_get_info,
	.resize=fsdrv_resize,
	.rename=fsdrv_rename,
	.wait=fsdrv_posix_wait,
	.getcwd=fsdrv_getcwd,
};

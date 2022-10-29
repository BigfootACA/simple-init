/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/DiskIo.h>
#include<Protocol/BlockIo.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include"str.h"
#include"uefi.h"
#include"locate.h"
#include"array.h"
#include"filesystem.h"
#include"compatible.h"
#include"../extern.h"
#include"../fs_internal.h"

static fsdrv fsdrv_uefi;

struct fsd{
	EFI_FILE_HANDLE hand;
	EFI_DEVICE_PATH_PROTOCOL*dp;
	enum fsd_type{
		TYPE_NONE,
		TYPE_FILE,
		TYPE_BLOCK,
	}type;
	union{
		struct{
			CHAR16*path;
			EFI_FILE_PROTOCOL*file;
			EFI_FILE_PROTOCOL*root;
			EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs;
		}file;
		struct{
			UINT64 offset;
			EFI_BLOCK_IO_PROTOCOL*blk;
			EFI_DISK_IO_PROTOCOL*disk;
		}block;
	};
};

static void file_info_to_info(EFI_FILE_INFO*fi,fs_file_info*info){
	memset(info,0,sizeof(fs_file_info));
	memcpy(
		info->magic,
		FS_INFO_MAGIC,
		sizeof(info->magic)
	);
	info->ctime=Efi2Time(&fi->CreateTime);
	info->atime=Efi2Time(&fi->LastAccessTime);
	info->mtime=Efi2Time(&fi->ModificationTime);
	info->size=fi->FileSize;
	UnicodeStrToAsciiStrS(
		fi->FileName,
		info->name,
		sizeof(info->name)-1
	);
	info->mode=0644;
	info->type=FS_TYPE_FILE_REG;
	if(fi->Attribute&EFI_FILE_DIRECTORY){
		info->type=FS_TYPE_FILE_FOLDER;
		info->mode=0755;
	}
}

static void media_info_to_info(EFI_BLOCK_IO_MEDIA*m,fs_file_info*info){
	memset(info,0,sizeof(fs_file_info));
	memcpy(
		info->magic,
		FS_INFO_MAGIC,
		sizeof(info->magic)
	);
	info->size=m->BlockSize*(m->LastBlock+1);
	info->mode=0644;
	info->type=FS_TYPE_FILE_BLOCK;
}

static void fsdrv_close(const fsdrv*drv,fsh*f){
	errno=0;
	struct fsd*d;
	EFI_STATUS st=EFI_INVALID_PARAMETER;
	if(!f||!drv||drv!=f->driver)return;
	if((d=f->data)){
		if(d->dp)FreePool(d->dp);
		if(d->type==TYPE_FILE){
			if(d->file.path)FreePool(d->file.path);
			if(d->file.file)st=d->file.file->Close(d->file.file);
			if(d->file.root)d->file.root->Close(d->file.root);
		}
		memset(d,0,sizeof(struct fsd));
	}
	errno=efi_status_to_errno(st);
}

static int fsdrv_flush(const fsdrv*drv,fsh*f){
	errno=0;
	struct fsd*d;
	if(!f||!drv||f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EROFS);
	RET(efi_status_to_errno(d->file.file->Flush(d->file.file)));
}

static EFI_HANDLE*fs_get_hand(url*u,enum fsd_type*type){
	UINTN len;
	CHAR16*buf;
	EFI_STATUS st;
	EFI_HANDLE hand=NULL;
	EFI_DEVICE_PATH_PROTOCOL*dp;
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	if(!u||!u->scheme)EPRET(EINVAL);
	if(u->username){
		if(strcasecmp(u->username,"block")==0)
			*type=TYPE_BLOCK;
		else if(strcasecmp(u->username,"file")==0)
			*type=TYPE_FILE;
		else EPRET(EINVAL);
	}
	if(*type==0)*type=TYPE_FILE;
	if(!u->host||!*u->host){
		if(*type==TYPE_BLOCK)EPRET(EINVAL);
                st=gBS->OpenProtocol(
                        gImageHandle,
                        &gEfiLoadedImageProtocolGuid,
                        (VOID**)&li,gImageHandle,NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );
		if(EFI_ERROR(st)||!li)EPRET(trlog_error(
			efi_status_to_errno(st),
			"get loaded image failed: %s",
			efi_status_to_string(st)
		));
		return li->DeviceHandle;
	}else if(strcasecmp(u->scheme,"locate")==0){
		hand=locate_get_handle_by_tag(u->host);
		if(!hand)EPRET(ENOENT);
		return hand;
	}else{
		len=(AsciiStrLen(u->host)+1)*sizeof(CHAR16);
		if(!(buf=AllocateZeroPool(len)))EPRET(ENOMEM);
		AsciiStrToUnicodeStrS(u->host,buf,len);
		dp=ConvertTextToDevicePath(buf);
		FreePool(buf);
		if(!dp)EPRET(trlog_error(
			EINVAL,"invalid device path"
		));
		st=gBS->LocateDevicePath(
			&gEfiDevicePathProtocolGuid,
			&dp,&hand
		);
		if(EFI_ERROR(st)||!hand)EPRET(trlog_error(
			efi_status_to_errno(st),
			"handle device path failed: %s",
			efi_status_to_string(st)
		));
		return hand;
	}
}

static int fs_get_root(
	url*u,
	enum fsd_type*type,
	struct fsd*d
){
	EFI_STATUS st;
	EFI_HANDLE hand=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	EFI_DISK_IO_PROTOCOL*disk=NULL;
	EFI_BLOCK_IO_PROTOCOL*blk=NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs=NULL;
	if(!u||!type)RET(EINVAL);
	hand=fs_get_hand(u,type);
	switch(*type){
		case TYPE_FILE:{
			if(hand&&!fs){
				st=gBS->HandleProtocol(
					hand,
					&gEfiSimpleFileSystemProtocolGuid,
					(VOID**)&fs
				);
				if(EFI_ERROR(st)||!fs)ERET(trlog_error(
					efi_status_to_errno(st),
					"handle simple file system for %p failed: %s",
					hand,efi_status_to_string(st)
				));
			}
			if(fs&&!fp){
				st=fs->OpenVolume(fs,&fp);
				if(EFI_ERROR(st)||!fp)ERET(trlog_error(
					efi_status_to_errno(st),
					"open volume from %p %p failed: %s",
					hand,fs,efi_status_to_string(st)
				));
			}
			if(d)d->file.root=fp,d->file.fs=fs;
		}break;
		case TYPE_BLOCK:{
			if(hand&&!disk){
				st=gBS->HandleProtocol(
					hand,
					&gEfiDiskIoProtocolGuid,
					(VOID**)&disk
				);
				if(EFI_ERROR(st)||!disk)ERET(trlog_error(
					efi_status_to_errno(st),
					"handle disk io for %p failed: %s",
					hand,efi_status_to_string(st)
				));
			}
			if(hand&&!blk){
				st=gBS->HandleProtocol(
					hand,
					&gEfiBlockIoProtocolGuid,
					(VOID**)&blk
				);
				if(EFI_ERROR(st)||!blk)ERET(trlog_error(
					efi_status_to_errno(st),
					"handle block io for %p failed: %s",
					hand,efi_status_to_string(st)
				));
			}
			if(d)d->block.disk=disk,d->block.blk=blk;
		}break;
		default:ERET(EINVAL);
	}
	if(d)d->hand=hand,d->type=*type;
	return 0;
}

static int fsdrv_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	size_t l;
	struct fsd d;
	EFI_STATUS st;
	CHAR16*buf=NULL;
	UINTN len,i,a,infos=0;
	EFI_FILE_INFO*info=NULL;
	enum fsd_type type=TYPE_NONE;
	UINT64 attr=0,mode=EFI_FILE_MODE_READ;
	if(!drv||!uri)RET(EINVAL);
	memset(&d,0,sizeof(struct fsd));
	if(fs_has_flag(flags,FILE_FLAG_FOLDER))
		attr|=EFI_FILE_DIRECTORY;
	if(fs_has_flag(flags,FILE_FLAG_WRITE))
		mode|=EFI_FILE_MODE_WRITE;
	if(fs_has_flag(flags,FILE_FLAG_CREATE))
		mode|=EFI_FILE_MODE_CREATE;
	if(fs_get_root(uri,&type,&d)!=0)
		DONE(errno?:EHOSTUNREACH);
	if(!d.hand)DONE(errno?:EHOSTUNREACH);
	switch(type){
		case TYPE_FILE:
			if(!uri->path)RET(EINVAL);
			if(!d.file.root)DONE(errno?:EHOSTUNREACH);
			len=(AsciiStrLen(uri->path)+1)*sizeof(CHAR16);
			if(!(buf=AllocateZeroPool(len)))DONE(errno?:ENOMEM);
			AsciiStrToUnicodeStrS(uri->path,buf,len);
			for(i=0;buf[i];i++)if(buf[i]=='/')buf[i]='\\';
			for(i=0;buf[i];i++)while(buf[i]=='\\'&&buf[i+1]=='\\'){
				for(a=0;buf[i+a+1];a++)buf[i+a]=buf[i+a+1];
				buf[i+a]=0;
			}
			st=d.file.root->Open(d.file.root,&d.file.file,buf,mode,attr);
			if(EFI_ERROR(st)||!d.file.file)DONE(efi_status_to_errno(st));
			if(!fs_has_flag(flags,FILE_FLAG_ACCESS)){
				if(!nf||!nf->uri||!nf->data)DONE(EINVAL);
				if((l=strlen(nf->uri->path))<=0)goto done;
				if(fs_has_flag(flags,FILE_FLAG_FOLDER))
					if(nf->uri->path[l-1]!='/')
						strcat(nf->uri->path,"/");
				d.file=d.file,d.file.path=buf,buf=NULL;
				d.dp=FileDevicePath(d.hand,d.file.path);
				if(!fs_has_flag(flags,FILE_FLAG_FOLDER)){
					st=efi_file_get_file_info(d.file.file,&infos,&info);
					if(EFI_ERROR(st)||!info)DONE(efi_status_to_errno(st));
					if(fs_has_flag(flags,FILE_FLAG_APPEND))
						d.file.file->SetPosition(d.file.file,info->FileSize);
					if(fs_has_flag(flags,FILE_FLAG_TRUNCATE)){
						d.file.file->SetPosition(d.file.file,0);
						if(fs_has_flag(flags,FILE_FLAG_WRITE)){
							info->FileSize=0;
							st=d.file.file->SetInfo(d.file.file,&gEfiFileInfoGuid,infos,info);
							if(EFI_ERROR(st)){
								d.file.file->Delete(d.file.file);
								d.file.file=NULL,mode|=EFI_FILE_MODE_CREATE;
								st=d.file.root->Open(d.file.root,&d.file.file,buf,mode,attr);
								if(EFI_ERROR(st)||!d.file.file)DONE(efi_status_to_errno(st));
							}
						}
					}
					FreePool(info);
				}
			}else d.file.file->Close(d.file.file),d.file.file=NULL;
			if(d.file.root)d.file.root->Close(d.file.root),d.file.root=NULL;
			if(buf)FreePool(buf);
		break;
		case TYPE_BLOCK:
			if(uri->path)RET(EINVAL);
			d.dp=DuplicateDevicePath(DevicePathFromHandle(d.hand));
		break;
		default:RET(ENOTSUP);
	}
	if(nf&&nf->data)memcpy(nf->data,&d,sizeof(d));
	RET(0);
	done:
	if(buf)FreePool(buf);
	if(d.file.file)d.file.file->Close(d.file.file);
	if(d.file.root)d.file.root->Close(d.file.root);
	EXRET(EIO);
}

static int fsdrv_read(
	const fsdrv*drv,fsh*f,
	void*buffer,
	size_t btr,
	size_t*br
){
	UINTN size=btr;
	EFI_STATUS st=EFI_SUCCESS;
	struct fsd*d=NULL;
	if(!f||!drv||!buffer)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(!fs_has_flag(f->flags,FILE_FLAG_READ))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	switch(d->type){
		case TYPE_FILE:{
			if(!d->file.file)RET(EBADF);
			st=d->file.file->Read(d->file.file,&size,buffer);
		}break;
		case TYPE_BLOCK:{
			if(
				!d->block.disk||
				!d->block.blk||
				!d->block.blk->Media
			)RET(EBADF);
			st=d->block.disk->ReadDisk(
				d->block.disk,
				d->block.blk->Media->MediaId,
				d->block.offset,btr,buffer
			);
			d->block.offset+=btr;
		}break;
		default:RET(ENOSYS);
	}
	if(br)*br=size;
	RET(efi_status_to_errno(st));
}

static int fsdrv_readdir(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
        struct fsd*d;
        EFI_STATUS st;
        EFI_FILE_INFO*fi=NULL;
        UINTN infos=0,si;
	if(!f||!drv||!info)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOSYS);
	if(!fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(ENOTDIR);
	for(;;){
                si=infos,st=d->file.file->Read(d->file.file,&si,fi);
                if(st==EFI_BUFFER_TOO_SMALL){
                        if(si<=0)break;
                        if(fi)FreePool(fi);
                        if(!(fi=AllocateZeroPool(si)))RET(ENOMEM);
                        infos=si;
                        continue;
		}
                if(EFI_ERROR(st)){
			if(fi)FreePool(fi);
			RET(efi_status_to_errno(st));
		}
		if(!fi)break;
                if(StrCmp(fi->FileName,L".")==0)continue;
                if(StrCmp(fi->FileName,L"..")==0)continue;
                break;
        }
	if(si<=0){
		if(fi)FreePool(fi);
		RET(EOF);
	}
	file_info_to_info(fi,info);
	info->features=drv->features;
	info->parent=f;
	RET(0);
}

static int fsdrv_write(
	const fsdrv*drv,
	fsh*f,
	void*buffer,
	size_t btw,
	size_t*bw
){
        struct fsd*d;
	UINTN size=btw;
	EFI_STATUS st=EFI_SUCCESS;
	if(!f||!drv||!buffer)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(!fs_has_flag(f->flags,FILE_FLAG_WRITE))RET(EPERM);
	if(fs_has_flag(f->flags,FILE_FLAG_FOLDER))RET(EISDIR);
	switch(d->type){
		case TYPE_FILE:{
			if(!d->file.file)RET(EBADF);
			st=d->file.file->Write(d->file.file,&size,buffer);
		}break;
		case TYPE_BLOCK:{
			if(
				!d->block.disk||
				!d->block.blk||
				!d->block.blk->Media
			)RET(EBADF);
			st=d->block.disk->WriteDisk(
				d->block.disk,
				d->block.blk->Media->MediaId,
				d->block.offset,btw,buffer
			);
			d->block.offset+=btw;
		}break;
		default:RET(ENOSYS);
	}
	if(bw)*bw=size;
	RET(efi_status_to_errno(st));
}

static int fsdrv_seek(const fsdrv*drv,fsh*f,size_t pos,int whence){
	int r=0;
	size_t o=0;
	struct fsd*d;
	EFI_STATUS st=EFI_SUCCESS;
	if(!f||!drv)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	switch(whence){
		case SEEK_CUR:
			r=fs_tell(f,&o);
			if(r!=0)RET(r);
			RET(fs_seek_locked(f,o+pos,SEEK_SET));
		break;
		case SEEK_END:
			r=fs_get_size_locked(f,&o);
			if(r!=0)RET(r);
			RET(fsdrv_seek(drv,f,o+pos,SEEK_SET));
		break;
		case SEEK_SET:switch(d->type){
			case TYPE_FILE:
				if(!d->file.file)RET(EBADF);
				st=d->file.file->SetPosition(d->file.file,pos);
			break;
			case TYPE_BLOCK:d->block.offset=pos;break;
			default:RET(ENOSYS);
		}break;
	}
	RET(efi_status_to_errno(st));
}

static int fsdrv_tell(const fsdrv*drv,fsh*f,size_t*pos){
	UINT64 p=0;
        struct fsd*d;
	EFI_STATUS st=EFI_SUCCESS;
	if(!f||!drv||!pos)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	switch(d->type){
		case TYPE_FILE:
			if(!d->file.file)RET(EBADF);
			st=d->file.file->GetPosition(d->file.file,&p);
		break;
		case TYPE_BLOCK:p=d->block.offset;break;
		default:RET(ENOSYS);
	}
	*pos=p;
	RET(efi_status_to_errno(st));
}

static int fsdrv_get_info(
	const fsdrv*drv,
	fsh*f,
	fs_file_info*info
){
	struct fsd*d;
	EFI_STATUS st;
	EFI_FILE_INFO*fi=NULL;
	if(!f||!drv||!info)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	switch(d->type){
		case TYPE_FILE:
			if(!d->file.file)RET(EBADF);
			st=efi_file_get_file_info(d->file.file,NULL,&fi);
			if(EFI_ERROR(st))RET(efi_status_to_errno(st));
			file_info_to_info(fi,info);
			FreePool(fi);
			info->features=FS_FEATURE_READABLE|
				FS_FEATURE_WRITABLE|
				FS_FEATURE_SEEKABLE|
				FS_FEATURE_HAVE_STAT|
				FS_FEATURE_HAVE_SIZE|
				FS_FEATURE_HAVE_PATH|
				FS_FEATURE_HAVE_TIME|
				FS_FEATURE_HAVE_FOLDER;
		break;
		case TYPE_BLOCK:
			if(
				!d->block.disk||
				!d->block.blk||
				!d->block.blk->Media
			)RET(EBADF);
			media_info_to_info(d->block.blk->Media,info);
			info->features=FS_FEATURE_READABLE|
				FS_FEATURE_WRITABLE|
				FS_FEATURE_SEEKABLE|
				FS_FEATURE_HAVE_STAT|
				FS_FEATURE_HAVE_SIZE;
		break;
		default:RET(ENOSYS);
	}
	info->parent=f;
	RET(0);
}

static int fsdrv_resize(const fsdrv*drv,fsh*f,size_t pos){
	struct fsd*d;
	UINTN size=0;
	EFI_STATUS st;
	EFI_FILE_INFO*fi=NULL;
	if(!f||!drv)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOSYS);
	st=efi_file_get_file_info(d->file.file,&size,&fi);
	if(EFI_ERROR(st))RET(efi_status_to_errno(st));
	fi->FileSize=pos;
	st=d->file.file->SetInfo(
		d->file.file,
		&gEfiFileInfoGuid,
		size,fi
	);
	FreePool(fi);
	RET(efi_status_to_errno(st));
}

static int fsdrv_rename(const fsdrv*drv,fsh*f,url*to){
	struct fsd*d;
	EFI_STATUS st;
	UINTN size=0,sl=0,nl=0;
	EFI_FILE_INFO*fi=NULL,*ni=NULL;
	if(!f||!drv||!to||!to->path)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOSYS);
	st=efi_file_get_file_info(d->file.file,&size,&fi);
	if(EFI_ERROR(st))RET(efi_status_to_errno(st));
	sl=(AsciiStrLen(to->path)+1)*sizeof(CHAR16);
	nl=sizeof(EFI_FILE_INFO)+sl;
	if(nl>size){
		if(!(ni=ReallocatePool(size,nl,fi)))RET(ENOMEM);
		fi=ni,size=nl,fi->Size=nl;
	}
	AsciiStrToUnicodeStrS(to->path,fi->FileName,sl);
	for(UINTN i=0;fi->FileName[i];i++)
		if(fi->FileName[i]=='/')fi->FileName[i]='\\';
	st=d->file.file->SetInfo(d->file.file,&gEfiFileInfoGuid,size,fi);
	if(!EFI_ERROR(st))f->cache_info_time=0;
	FreePool(fi);
	RET(efi_status_to_errno(st));
}

static int fsdrv_uri_parse(const fsdrv*drv,fsh*f,const char*uri,url*u){
	char*p=NULL;
	if(!drv||!uri||!u)RET(EINVAL);
	if(f&&f->driver!=drv)return -1;
	url_clean(u);
	switch(uri[0]){
		case '@':url_set_username(u,"file",0);break;
		case '#':url_set_username(u,"block",0);break;
		default:return -1;
	}
	url_set_scheme(u,"locate",0);
	if((p=strchr(uri,':'))){
		url_set_host(u,uri+1,p-uri-1);
		url_set_path_decoded(u,p+1,0);
		trim_path(u->path);
	}else url_set_host(u,uri+1,0);
	RET(0);
}

static int fsdrv_ioctl(const fsdrv*drv,fsh*f,fs_ioctl_id id,va_list args){
        struct fsd*d;
	if(!f||!drv)RET(EINVAL);
	if(f->driver!=drv)RET(EINVAL);
	if(!(d=f->data))RET(EBADF);
	if(d->type!=TYPE_FILE)RET(ENOTSUP);
	if(!d->file.file)RET(EBADF);
	switch(id){
		case FS_IOCTL_UEFI_GET_HANDLE:{
			EFI_HANDLE*hand;
			if((hand=va_arg(args,EFI_HANDLE*))){
				*hand=d->hand;
				RET(0);
			}else RET(EINVAL);
		}break;
		case FS_IOCTL_UEFI_GET_FILE_PROTOCOL:{
			EFI_FILE_PROTOCOL**fp;
			if(d->type!=TYPE_FILE)RET(ENOTSUP);
			if((fp=va_arg(args,EFI_FILE_PROTOCOL**))){
				*fp=d->file.file;
				RET(0);
			}else RET(EINVAL);
		}break;
		case FS_IOCTL_UEFI_GET_DEVICE_PATH:{
			EFI_DEVICE_PATH_PROTOCOL**dp;
			if((dp=va_arg(args,EFI_DEVICE_PATH_PROTOCOL**))){
				*dp=d->dp;
				RET(0);
			}else RET(EINVAL);
		}break;
		default:RET(ENOTSUP);
	}
	RET(0);
}

static struct fsdrv_map{
	char protocol[64];
}map[]={
	{.protocol="uefi"},
	{.protocol="file"},
	{.protocol="locate"},
};

void fsdrv_register_uefi(bool deinit){
	fsdrv*drv;
	if(!deinit)for(size_t i=0;i<ARRLEN(map);i++){
		if(!(drv=memdup(&fsdrv_uefi,sizeof(fsdrv))))continue;
		strcpy(drv->protocol,map[i].protocol);
		fsdrv_register(drv);
	}
}

static fsdrv fsdrv_uefi={
	.magic=FS_DRIVER_MAGIC,
	.base=&fsdrv_template,
	.cache_info_time=5,
	.hand_data_size=sizeof(struct fsd),
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_WRITABLE|
		FS_FEATURE_SEEKABLE|
		FS_FEATURE_HAVE_STAT|
		FS_FEATURE_HAVE_SIZE|
		FS_FEATURE_HAVE_PATH|
		FS_FEATURE_HAVE_TIME|
		FS_FEATURE_HAVE_FOLDER,
	.flush=fsdrv_flush,
	.close=fsdrv_close,
	.open=fsdrv_open,
	.read=fsdrv_read,
	.readdir=fsdrv_readdir,
	.write=fsdrv_write,
	.seek=fsdrv_seek,
	.tell=fsdrv_tell,
	.get_info=fsdrv_get_info,
	.uri_parse=fsdrv_uri_parse,
	.resize=fsdrv_resize,
	.ioctl=fsdrv_ioctl,
	.rename=fsdrv_rename,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdlib.h>
#include<Guid/FileInfo.h>
#include<Guid/FileSystemVolumeLabelInfo.h>
#include<Library/BaseLib.h>
#include<Library/UefiLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include"uefi.h"
#include"lvgl.h"
#include"logger.h"
#include"defines.h"
#include"gui/fsext.h"
#define TAG "filesystem"

#define XWARN(str...){if(fs->debug)tlog_warn(str);}
#define XINFO(str...){if(fs->debug)tlog_info(str);}

bool fsext_is_multi=true;

struct fs_root{
	bool debug;
	EFI_HANDLE hand;
	EFI_FILE_PROTOCOL*proto;
};

static lv_res_t efi_status_to_lv_res(EFI_STATUS err){
	switch(err){
		case EFI_SUCCESS:return LV_FS_RES_OK;
		case EFI_SECURITY_VIOLATION:
		case EFI_WRITE_PROTECTED:
		case EFI_ACCESS_DENIED:
			return LV_FS_RES_DENIED;
		case EFI_NO_MEDIA:
		case EFI_NO_RESPONSE:
		case EFI_NOT_FOUND:
			return LV_FS_RES_NOT_EX;
		case EFI_NOT_READY:
		case EFI_NO_MAPPING:
		case EFI_NOT_STARTED:
		case EFI_MEDIA_CHANGED:
			return LV_FS_RES_BUSY;
		case EFI_BAD_BUFFER_SIZE:
		case EFI_BUFFER_TOO_SMALL:
		case EFI_INVALID_LANGUAGE:
		case EFI_INVALID_PARAMETER:
			return LV_FS_RES_INV_PARAM;
		case EFI_OUT_OF_RESOURCES:
			return LV_FS_RES_OUT_OF_MEM;
		case EFI_COMPROMISED_DATA:
		case EFI_VOLUME_CORRUPTED:
		case EFI_LOAD_ERROR:
		case EFI_ICMP_ERROR:
		case EFI_TFTP_ERROR:
		case EFI_HTTP_ERROR:
		case EFI_CRC_ERROR:
			return LV_FS_RES_FS_ERR;
		case EFI_TIMEOUT:
			return LV_FS_RES_TOUT;
		case EFI_VOLUME_FULL:
		case EFI_END_OF_FILE:
		case EFI_END_OF_MEDIA:
			return LV_FS_RES_FULL;
		case EFI_ALREADY_STARTED:
		case EFI_ABORTED:
			return LV_FS_RES_LOCKED;
		case EFI_UNSUPPORTED:
			return LV_FS_RES_NOT_IMP;
		case EFI_DEVICE_ERROR:
		case EFI_PROTOCOL_ERROR:
		case EFI_INCOMPATIBLE_VERSION:
			return LV_FS_RES_HW_ERR;
		default:
			return LV_FS_RES_UNKNOWN;
	}
}

static bool fileinfo_is_dir(EFI_FILE_INFO*info){
	return (info->Attribute&EFI_FILE_DIRECTORY)!=0;
}

static EFI_HANDLE get_fs_proto(UINTN*bs){
	EFI_HANDLE*hb=NULL;
	EFI_STATUS st=gBS->LocateHandle(ByProtocol,&gEfiSimpleFileSystemProtocolGuid,NULL,bs,hb);
	if(st==EFI_BUFFER_TOO_SMALL){
		if(!(hb=AllocateZeroPool(*bs))){
			tlog_warn("allocate buffer for simple file system failed");
			return NULL;
		}
		st=gBS->LocateHandle(ByProtocol,&gEfiSimpleFileSystemProtocolGuid,NULL,bs,hb);
	}
	if(EFI_ERROR(st)){
		tlog_warn("location simple file system failed: %s",efi_status_to_string(st));
		return NULL;
	}
	return hb;
}

static lv_res_t fs_get_volume_label(struct _lv_fs_drv_t*drv,char*label,size_t len){
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	EFI_FILE_SYSTEM_VOLUME_LABEL*fi=NULL;
	EFI_STATUS st=st=efi_file_get_info(
		fs->proto,
		&gEfiFileSystemVolumeLabelInfoIdGuid,
		NULL,(VOID**)&fi
	);
	strncpy(label,_("Unknown"),len-1);
	if(
		!EFI_ERROR(st)&&fi&&
		fi->VolumeLabel&&
		fi->VolumeLabel[0]
	){
		memset(label,0,len);
		UnicodeStrToAsciiStrS(fi->VolumeLabel,label,len);
	}
	return LV_FS_RES_OK;
}

static bool fs_ready_cb(struct _lv_fs_drv_t*drv){
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	return fs->proto!=NULL;
}

static lv_res_t fs_open_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	const char*path,
	lv_fs_mode_t mode
){
	if(!drv||!file_p||!path)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	UINT64 flags=EFI_FILE_MODE_READ;
	switch(mode){
		case LV_FS_MODE_RD:break;
		case LV_FS_MODE_WR:
			flags|=EFI_FILE_MODE_CREATE;
			flags|=EFI_FILE_MODE_WRITE;
		break;
		default:return LV_FS_RES_INV_PARAM;
	}
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,path);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(fs->proto,&fh,xpath,flags,0);
	if(EFI_ERROR(st))XWARN(
		"open %c:%s mode %d failed: %s",
		drv->letter,path,mode,efi_status_to_string(st)
	);
	if(fh)*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p)=fh;
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_close_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p
){
	if(!drv||!file_p)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	EFI_STATUS st=fh->Close(fh);
	if(EFI_ERROR(st))XWARN(
		"close %c:#%p failed: %s",
		drv->letter,fh,efi_status_to_string(st)
	);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_remove_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn
){
	if(!drv||!fn)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,fn);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(fs->proto,&fh,xpath,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,0);
	if(EFI_ERROR(st)){
		XWARN(
			"open %c:%s failed: %s",
			drv->letter,fn,efi_status_to_string(st)
		);
	}else{
		st=fh->Delete(fh);
		if(EFI_ERROR(st))XWARN(
			"delete %c:%s failed: %s",
			drv->letter,fn,efi_status_to_string(st)
		);
	}
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_get_type_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn,
	enum item_type*type
){
	if(!drv||!fn||!type)return false;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return false;
	bool isdir=false;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh;
	EFI_FILE_INFO*info=NULL;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,fn);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(fs->proto,&fh,xpath,EFI_FILE_MODE_READ,0);
	if(EFI_ERROR(st)){
		XWARN(
			"get type open %c:%s failed: %s",
			drv->letter,fn,efi_status_to_string(st)
		);
	}else{
		st=efi_file_get_file_info(fh,NULL,&info);
		if(info&&!EFI_ERROR(st))*type=fileinfo_is_dir(info)?TYPE_DIR:TYPE_FILE;
		else XWARN(
			"get type %c:#%p failed: %s",
			drv->letter,fh,efi_status_to_string(st)
		);
		fh->Close(fh);
	}
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	if(info)FreePool(info);
	return isdir;
}

static bool fs_is_dir_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn
){
	if(!drv||!fn)return false;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return false;
	bool isdir=false;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh;
	EFI_FILE_INFO*info=NULL;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,fn);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(fs->proto,&fh,xpath,EFI_FILE_MODE_READ,0);
	if(EFI_ERROR(st)){
		XWARN(
			"is dir open %c:%s failed: %s",
			drv->letter,fn,efi_status_to_string(st)
		);
	}else{
		st=efi_file_get_file_info(fh,NULL,&info);
		if(info&&!EFI_ERROR(st))isdir=fileinfo_is_dir(info);
		else XWARN(
			"is dir %c:#%p failed: %s",
			drv->letter,fh,efi_status_to_string(st)
		);
		fh->Close(fh);
	}
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	if(info)FreePool(info);
	return isdir;
}

EFI_FILE_PROTOCOL*lv_fs_file_to_fp(lv_fs_file_t*fp){
	return fp?*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)fp->file_d):NULL;
}

static lv_res_t fs_read_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	void*buf,
	uint32_t btr,
	uint32_t*br
){
	if(!drv||!file_p||!buf||!br)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	UINTN r=(UINTN)btr;
	EFI_STATUS st=fh->Read(fh,&r,buf);
	*br=(uint32_t)r;
	if(EFI_ERROR(st))XWARN(
		"read %c:#%p failed: %s",
		drv->letter,fh,efi_status_to_string(st)
	);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_write_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	const void*buf,
	uint32_t btw,
	uint32_t*bw
){
	if(!drv||!file_p||!buf||!bw)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	UINTN w=(UINTN)btw;
	EFI_STATUS st=fh->Write(fh,&w,(void*)buf);
	*bw=(uint32_t)w;
	if(EFI_ERROR(st))XWARN(
		"write %c:#%p: %s",
		drv->letter,fh,efi_status_to_string(st)
	)
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_seek_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t pos
){
	if(!drv||!file_p)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_STATUS st=fh->SetPosition(fh,(UINTN)pos);
	if(EFI_ERROR(st))XWARN(
		"set position %c:#%p failed: %s",
		drv->letter,fh,efi_status_to_string(st)
	);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_tell_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t*pos_p
){
	if(!drv||!file_p||!pos_p)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	UINT64 pos=(UINT64)*pos_p;
	EFI_STATUS st=fh->GetPosition(fh,&pos);
	*pos_p=(uint32_t)pos;
	if(EFI_ERROR(st))XWARN(
		"get position %c:#%p failed: %s",
		drv->letter,fh,efi_status_to_string(st)
	);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_trunc_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p
){
	return LV_FS_RES_NOT_IMP;
}

static lv_res_t fs_size_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t*size_p
){
	if(!drv||!file_p||!size_p)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*fh=*(EFI_FILE_PROTOCOL**)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_FILE_INFO*info=NULL;
	EFI_STATUS st=st=efi_file_get_file_info(fh,NULL,&info);
	if(info&&!EFI_ERROR(st))*size_p=(uint32_t)info->FileSize;
	else XWARN(
		"size %c:#%p failed: %s",
		drv->letter,fh,efi_status_to_string(st)
	);
	if(info)FreePool(info);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_rename_cb(
	struct _lv_fs_drv_t*drv,
	const char*oldname,
	const char*newname
){
	return LV_FS_RES_NOT_IMP;
}

static lv_res_t fs_free_space_cb(
	struct _lv_fs_drv_t*drv,
	uint32_t*total_p,
	uint32_t*free_p
){
	return LV_FS_RES_NOT_IMP;
}

static lv_res_t fs_dir_open_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p,
	const char*path
){
	if(!drv||!rddir_p||!path)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	if(!*path||strcmp(path,"/")==0){
		fh=fs->proto,st=EFI_SUCCESS;
		fh->SetPosition(fh,0);
	}else{
		strcpy(ep,path);
		do{if(*cp=='/')*cp='\\';}while(*cp++);
		AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
		st=fs->proto->Open(fs->proto,&fh,xpath,EFI_FILE_READ_ONLY,0);
		if(EFI_ERROR(st))XWARN(
			"open dir %c:%s failed: %s",
			drv->letter,path,efi_status_to_string(st)
		);
	}
	if(fh)((lv_fs_dir_t*)rddir_p)->dir_d=fh;
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_dir_read_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p,
	char*fn
){
	if(!drv||!rddir_p||!fn)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*dh=(EFI_FILE_PROTOCOL*)((lv_fs_dir_t*)rddir_p)->dir_d;
	UINTN infos=0,si;
	EFI_FILE_INFO*info=NULL;
	EFI_STATUS st;
	for(;;){
		si=infos,st=dh->Read(dh,&si,info);
		if(st==EFI_BUFFER_TOO_SMALL){
			if(si<=0)break;
			if(info)FreePool(info);
			if(!(info=AllocateZeroPool(si)))
				return LV_FS_RES_OUT_OF_MEM;
			infos=si;
			continue;
		}
		if(EFI_ERROR(st)||si==0)break;
		if(StrCmp(info->FileName,L".")==0)continue;
		if(StrCmp(info->FileName,L"..")==0)continue;
		break;
	}
	*fn=0;
	if(EFI_ERROR(st)){
		XWARN(
			"read dir %c:%s failed: %s",
			drv->letter,fn,efi_status_to_string(st)
		);
	}else if(si!=0){
		int i=255;
		char*name=fn;
		if(fileinfo_is_dir(info))*(name++)='/',i--;
		UnicodeStrToAsciiStrS(info->FileName,name,i);
	}
	if(info)FreePool(info);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_dir_close_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p
){
	if(!drv||!rddir_p)return LV_FS_RES_INV_PARAM;
	EFI_FILE_PROTOCOL*dh=(EFI_FILE_PROTOCOL*)((lv_fs_dir_t*)rddir_p)->dir_d;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	if(dh==fs->proto)return LV_FS_RES_OK;
	EFI_STATUS st=dh->Close(dh);
	if(EFI_ERROR(st))XWARN(
		"close dir %c:#%p failed: %s",
		drv->letter,dh,efi_status_to_string(st)
	);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_mkdir_cb(
	struct _lv_fs_drv_t*drv,
	const char*name
){
	if(!drv||!name||!*name)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh=NULL;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,name);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(
		fs->proto,&fh,xpath,
		EFI_FILE_MODE_READ|
		EFI_FILE_MODE_WRITE|
		EFI_FILE_MODE_CREATE,
		EFI_FILE_DIRECTORY
	);
	if(EFI_ERROR(st))XWARN(
		"create dir %c:%s failed: %s",
		drv->letter,name,efi_status_to_string(st)
	);
	if(fh)fh->Close(fh);
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	return efi_status_to_lv_res(st);
}

static lv_res_t fs_creat_cb(
	struct _lv_fs_drv_t*drv,
	const char*name
){
	if(!drv||!name||!*name)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs||!fs->proto)return LV_FS_RES_INV_PARAM;
	EFI_STATUS st=EFI_OUT_OF_RESOURCES;
	EFI_FILE_PROTOCOL*fh=NULL;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xpath=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xpath=AllocateZeroPool(xs)))goto done;
	strcpy(ep,name);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xpath,xs/sizeof(CHAR16));
	st=fs->proto->Open(
		fs->proto,&fh,xpath,
		EFI_FILE_MODE_READ|
		EFI_FILE_MODE_WRITE|
		EFI_FILE_MODE_CREATE,
		0
	);
	if(EFI_ERROR(st))XWARN(
		"create file %c:%s failed: %s",
		drv->letter,name,efi_status_to_string(st)
	);
	if(fh)fh->Close(fh);
	done:
	if(ep)FreePool(ep);
	if(xpath)FreePool(xpath);
	return efi_status_to_lv_res(st);
}

int init_lvgl_uefi_fs(char letter,EFI_HANDLE hand,EFI_FILE_PROTOCOL*proto,bool debug){
	lv_fs_drv_t*drv=NULL;
	struct fs_root*fs=NULL;
	struct fsext*fse=NULL;
	if(!proto)return -1;
	if(
		!(fs=AllocateZeroPool(sizeof(struct fs_root)))||
		!(fse=AllocateZeroPool(sizeof(struct fsext)))||
		!(drv=AllocateZeroPool(sizeof(lv_fs_drv_t)))
	){
		if(fs)FreePool(fs);
		if(fse)FreePool(fse);
		return -1;
	}
	fs->hand=hand;
	fs->proto=proto;
	fs->debug=debug;
	lv_fs_drv_init(drv);
	fse->get_volume_label=fs_get_volume_label;
	fse->get_type_cb=fs_get_type_cb;
	fse->is_dir_cb=fs_is_dir_cb;
	fse->creat=fs_creat_cb;
	fse->mkdir=fs_mkdir_cb;
	fse->user_data=fs;
	drv->user_data=fse;
	drv->letter=letter;
	drv->file_size=sizeof(EFI_FILE_PROTOCOL*);
	drv->rddir_size=sizeof(EFI_FILE_PROTOCOL*);
	drv->ready_cb=fs_ready_cb;
	drv->open_cb=fs_open_cb;
	drv->close_cb=fs_close_cb;
	drv->remove_cb=fs_remove_cb;
	drv->read_cb=fs_read_cb;
	drv->write_cb=fs_write_cb;
	drv->seek_cb=fs_seek_cb;
	drv->tell_cb=fs_tell_cb;
	drv->trunc_cb=fs_trunc_cb;
	drv->size_cb=fs_size_cb;
	drv->rename_cb=fs_rename_cb;
	drv->free_space_cb=fs_free_space_cb;
	drv->dir_open_cb=fs_dir_open_cb;
	drv->dir_read_cb=fs_dir_read_cb;
	drv->dir_close_cb=fs_dir_close_cb;
	lv_fs_drv_register(drv);
	return 0;
}

void lvgl_init_all_fs_uefi(bool debug){
	char letter;
	UINTN bs=0,i;
	EFI_STATUS st;
	EFI_HANDLE*hb=get_fs_proto(&bs);
	if(!hb)return;
	for(i=0,letter='A';i<bs/sizeof(hb)&&letter<='Z';i++,letter++){
		EFI_FILE_PROTOCOL*fh=NULL;
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*v=NULL;
		if(lv_fs_get_drv(letter))continue;
		st=gBS->HandleProtocol(hb[i],&gEfiSimpleFileSystemProtocolGuid,(VOID*)&v);
		if(EFI_ERROR(st)){
			tlog_warn("handle protocol failed: %s",efi_status_to_string(st));
			continue;
		}
		st=v->OpenVolume(v,&fh);
		if(EFI_ERROR(st)){
			tlog_warn("open volume failed: %s",efi_status_to_string(st));
			continue;
		}
		if(init_lvgl_uefi_fs(letter,hb[i],fh,debug)!=0){
			tlog_warn("init file system failed");
			continue;
		}
		if(debug)tlog_debug("add lvgl drive %c",letter);
	}
}

EFI_DEVICE_PATH_PROTOCOL*fs_drv_get_device_path(char letter){
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return NULL;
	if(drv->ready_cb&&!drv->ready_cb(drv))return NULL;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	return DevicePathFromHandle(fs->hand);
}

EFI_DEVICE_PATH_PROTOCOL*fs_get_device_path(const char*path){
	if(!path)return NULL;
	char letter=path[0];
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return NULL;
	if(drv->ready_cb&&!drv->ready_cb(drv))return NULL;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	EFI_DEVICE_PATH_PROTOCOL*ret=NULL;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	char*ep=NULL,*cp;
	CHAR16*xp=NULL;
	if(!(cp=ep=AllocateZeroPool(PATH_MAX)))goto done;
	if(!(xp=AllocateZeroPool(xs)))goto done;
	strcpy(ep,path+2);
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	AsciiStrToUnicodeStrS(ep,xp,xs/sizeof(CHAR16));
	ret=FileDevicePath(fs->hand,xp);
	done:
	if(ep)FreePool(ep);
	if(xp)FreePool(xp);
	return ret;
}

EFI_FILE_PROTOCOL*fs_get_root_by_letter(char letter){
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return NULL;
	if(drv->ready_cb&&!drv->ready_cb(drv))return NULL;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	return fs->proto;
}
EFI_HANDLE fs_get_root_handle_by_letter(char letter){
	lv_fs_drv_t*drv=lv_fs_get_drv(letter);
	if(!drv)return NULL;
	if(drv->ready_cb&&!drv->ready_cb(drv))return NULL;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	return fs->hand;
}
#endif
#endif

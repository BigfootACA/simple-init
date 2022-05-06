/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_UEFI
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileSystemInfo.h>
#include<Guid/FileInfo.h>
#endif
#include<string.h>
#include<sys/stat.h>
#include"confd_internal.h"
#include"logger.h"
#include"locate.h"
#include"uefi.h"
#define TAG "config"

extern struct conf_file_hand conf_hand_conf;
extern struct conf_file_hand conf_hand_json;
extern struct conf_file_hand conf_hand_xml;
struct conf_file_hand*conf_hands[]={
	&conf_hand_conf,
	#ifdef ENABLE_JSONC
	&conf_hand_json,
	#endif
	#ifdef ENABLE_MXML
	&conf_hand_xml,
	#endif
	NULL
};

static const char*file_get_ext(const char*path){
	size_t s=strlen(path);
	if(s==0)EPRET(EINVAL);
	for(size_t i=s-1;i>0;i--){
		if(path[i]=='.')return path+i+1;
		if(path[i]=='/')break;
	}
	return NULL;
}

static struct conf_file_hand*find_hand(const char*ext){
	if(!ext)EPRET(EINVAL);
	char*e;
	struct conf_file_hand*fh;
	for(size_t s=0;(fh=conf_hands[s]);s++){
		if(!fh->ext)continue;
		for(size_t x=0;(e=fh->ext[x]);x++)
			if(strcasecmp(e,ext)==0)return fh;
	}
	EPRET(ENOENT);
}

static struct conf_file_hand*find_hand_by_file(const char*file){
	return find_hand(file_get_ext(file));
}

#ifdef ENABLE_UEFI
static int do_close(struct conf_file_hand*hand,bool save){
	int r=0;
	if(save&&hand->off>0){
		EFI_STATUS st;
		UINTN w=hand->off;
		st=hand->fd->Write(hand->fd,&w,hand->buff);
		if(EFI_ERROR(st))r=trlog_warn(
			ENUM(efi_status_to_errno(st)),
			"write config failed: %s",
			efi_status_to_string(st)
		);
		else if(w!=(UINTN)hand->off)r=trlog_warn(
			-1,"write size mismatch %llu != %llu",
			(unsigned long long)w,
			(unsigned long long)hand->off
		);
	}
	if(hand->fd)hand->fd->Close(hand->fd);
	if(hand->buff)free(hand->buff);
	hand->fd=NULL,hand->buff=NULL,hand->len=0,hand->off=0;
	return r;
}

static int do_load(struct conf_file_hand*hand){
	char*cp=hand->path;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	CHAR16*xp=NULL;
	locate_ret*loc=NULL;
	EFI_STATUS st=EFI_SUCCESS;
	EFI_FILE_INFO*info=NULL;
	do_close(hand,false);
	if((cp[0]=='@'&&strchr(cp,':'))||!hand->dir){
		if(!(loc=AllocateZeroPool(sizeof(locate_ret))))
			EDONE(tlog_warn("alloc locate failed"));
		if(!boot_locate(loc,cp))
			EDONE(tlog_warn("locate %s failed",cp));
		if(loc->type!=LOCATE_FILE||!loc->file)
			EDONE(tlog_warn("locate %s type not supported failed",cp));
		hand->fd=loc->file;
	}else{
		if(!(xp=AllocateZeroPool(xs)))goto done;
		do{if(*cp=='/')*cp='\\';}while(*cp++);
		AsciiStrToUnicodeStrS(hand->path,xp,xs/sizeof(CHAR16));
		st=hand->dir->Open(hand->dir,&hand->fd,xp,EFI_FILE_MODE_READ,0);
		if(EFI_ERROR(st))EDONE(tlog_warn(
			"open config '%s' failed: %s",
			hand->path,efi_status_to_string(st)
		));
	}
	st=efi_file_get_file_info(hand->fd,NULL,&info);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get config '%s' file info failed: %s",
		hand->path,efi_status_to_string(st)
	));
	hand->len=info->FileSize;
	if(info->Attribute&EFI_FILE_DIRECTORY)
		EDONE(tlog_warn("config '%s' is a directory",hand->path));
	if(hand->len>0x400000)
		EDONE(tlog_warn("config '%s' too large",hand->path));
	if(hand->len<=0)
		EDONE(tlog_warn("config '%s' too small",hand->path));
	if(!(hand->buff=malloc(hand->len+1)))
		EDONE(tlog_warn("allocate file content failed"));
	ZeroMem(hand->buff,hand->len+1);
	st=hand->fd->Read(hand->fd,(UINTN*)&hand->len,hand->buff);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"read '%s' failed: %s",
		hand->path,efi_status_to_string(st)
	));
	if(loc)FreePool(loc);
	if(info)FreePool(info);
	if(xp)FreePool(xp);
	return 0;
	done:
	if(loc)FreePool(loc);
	if(xp)FreePool(xp);
	if(info)FreePool(info);
	do_close(hand,false);
	return EFI_ERROR(st)?ENUM(efi_status_to_errno(st)):-1;
}

static int do_save(struct conf_file_hand*hand){
	char*cp=hand->path;
	UINTN infos=0;
	UINTN xs=PATH_MAX*sizeof(CHAR16);
	CHAR16*xp=NULL;
	locate_ret*loc=NULL;
	EFI_STATUS st=EFI_SUCCESS;
	EFI_FILE_INFO*info=NULL;
	EFI_FILE_PROTOCOL*dir=hand->dir;
	do_close(hand,false);
	if((cp[0]=='@'&&strchr(cp,':'))||!dir){
		if(!(loc=AllocateZeroPool(sizeof(locate_ret))))
			EDONE(tlog_warn("alloc locate failed"));
		if(!boot_locate_create_file(loc,cp))
			EDONE(tlog_warn("locate %s failed",cp));
		if(loc->type!=LOCATE_FILE||!loc->file)
			EDONE(tlog_warn("locate %s type not supported failed",cp));
		hand->fd=loc->file,dir=loc->root;
	}else{
		if(!(xp=AllocateZeroPool(xs)))goto done;
		do{if(*cp=='/')*cp='\\';}while(*cp++);
		AsciiStrToUnicodeStrS(hand->path,xp,xs/sizeof(CHAR16));
		st=dir->Open(
			dir,&hand->fd,xp,
			EFI_FILE_MODE_READ|
			EFI_FILE_MODE_WRITE|
			EFI_FILE_MODE_CREATE,0
		);
		if(EFI_ERROR(st))EDONE(tlog_warn(
			"open config '%s' failed: %s",
			hand->path,efi_status_to_string(st)
		));
	}
	st=efi_file_get_file_info(hand->fd,&infos,&info);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get config '%s' file info failed: %s",
		hand->path,efi_status_to_string(st)
	));
	if(info->Attribute&EFI_FILE_DIRECTORY)
		EDONE(tlog_warn("config '%s' is a directory",hand->path));
	info->FileSize=0;
	st=hand->fd->SetInfo(hand->fd,&gEfiFileInfoGuid,infos,info);
	if(EFI_ERROR(st)){
		hand->fd->Delete(hand->fd);
		hand->fd=NULL;
		st=dir->Open(
			dir,&hand->fd,xp,
			EFI_FILE_MODE_READ|
			EFI_FILE_MODE_WRITE|
			EFI_FILE_MODE_CREATE,0
		);
		if(EFI_ERROR(st))EDONE(tlog_warn(
			"create config '%s' failed: %s",
			hand->path,efi_status_to_string(st)
		));
	}
	hand->fd->SetPosition(hand->fd,0);
	if(loc)FreePool(loc);
	if(info)FreePool(info);
	if(xp)FreePool(xp);
	return 0;
	done:
	if(loc)FreePool(loc);
	if(xp)FreePool(xp);
	if(info)FreePool(info);
	do_close(hand,false);
	return EFI_ERROR(st)?ENUM(efi_status_to_errno(st)):-1;
}

#else

static int do_close(struct conf_file_hand*hand,bool save){
	int r=0;
	if(save&&hand->off>0){
		ssize_t w=write(hand->fd,hand->buff,hand->off);
		if(w<0)r=terlog_warn(ENUM(errno),"write config failed");
		else if((size_t)w!=(size_t)hand->off)r=trlog_warn(
			-1,"write size mismatch %zu != %zu",
			(size_t)w,hand->off
		);
	}
	if(hand->fd>0)close(hand->fd);
	if(hand->buff)free(hand->buff);
	hand->fd=-1,hand->buff=NULL,hand->len=0,hand->off=0;
	return r;
}

static int do_load(struct conf_file_hand*hand){
	ssize_t r=-1;
	struct stat st;
	do_close(hand,false);
	if((hand->fd=openat(hand->dir,hand->path,O_RDONLY))<0)
		EDONE(telog_warn("open config '%s' failed",hand->path));
	if(fstat(hand->fd,&st)<0)
		EDONE(telog_warn("stat config '%s' failed",hand->path));
	hand->len=st.st_size;
	if(!S_ISREG(st.st_mode))
		EDONE(tlog_warn("config '%s' not a file",hand->path));
	if(hand->len>0x400000)
		EDONE(tlog_warn("config '%s' too large",hand->path));
	if(hand->len<=0)
		EDONE(tlog_warn("config '%s' too small",hand->path));
	if(!(hand->buff=malloc(hand->len+1)))
		EDONE(tlog_warn("allocate file content failed"));
	r=read(hand->fd,hand->buff,hand->len);
	if(r<0)EDONE(telog_warn("read '%s' failed",hand->path));
	if((size_t)r!=hand->len)EDONE(tlog_warn(
		"read '%s' size mismatch",
		hand->path
	));
	return 0;
	done:do_close(hand,false);
	return errno<0?-(errno):-1;
}

static int do_save(struct conf_file_hand*hand){
	do_close(hand,false);
	if((hand->fd=openat(hand->dir,hand->path,O_RDWR|O_CREAT,0644))<0)
		EDONE(telog_warn("open config '%s' failed",hand->path));
	lseek(hand->fd,0,SEEK_SET);
	ftruncate(hand->fd,0);
	return 0;
	done:do_close(hand,false);
	return errno<0?-(errno):-1;
}
#endif

static ssize_t conf_read(struct conf_file_hand*hand,char*buff,size_t len){
	if(!hand||!buff||len<=0)return -1;
	memset(buff,0,len);
	size_t avail=hand->len-hand->off;
	size_t process=MIN(avail,len);
	if(avail==0)return 0;
	memcpy(buff,hand->buff+hand->off,process);
	hand->off+=process;
	return process;
}

static ssize_t conf_write(struct conf_file_hand*hand,char*buff,size_t len){
	if(!hand||!buff)return -1;
	if(len<=0)len=strlen(buff);
	if(len>hand->len-hand->off-1||!hand->buff){
		do{hand->len+=4096;}while(len>hand->len-hand->off-1);
		if(hand->buff){
			void*buf=realloc(hand->buff,hand->len);
			if(!buf)return -1;
			hand->buff=buf;
		}else if(!(hand->buff=malloc(hand->len)))return -1;
	}
	memcpy(hand->buff+hand->off,buff,len);
	hand->off+=len;
	return len;
}

static int _conf_load_file(_ROOT_TYPE dir,const char*file,bool inc){
	int r=0;
	static char xpath[PATH_MAX];
	if(!file)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	if(!dir)ERET(EINVAL);
	#else
	if(dir<0&&dir!=AT_FDCWD)ERET(EINVAL);
	#endif
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->load)ERET(ENOSYS);
	if(!hand->initialized){
		MUTEX_INIT(hand->lock);
		hand->initialized=true;
	}
	MUTEX_LOCK(hand->lock);
	memset(xpath,0,sizeof(xpath));
	strncpy(xpath,file,sizeof(xpath)-1);
	hand->include=inc;
	hand->dir=dir;
	hand->write=NULL;
	hand->read=conf_read;
	hand->path=xpath;
	r=do_load(hand);
	if(r==0){
		r=hand->load(hand);
		do_close(hand,false);
		if(r==0)errno=0;
	}else{
		tlog_warn("load failed");
		r=-1;
	}
	hand->include=false;
	hand->dir=0;
	hand->write=NULL;
	hand->read=NULL;
	hand->path=NULL;
	MUTEX_UNLOCK(hand->lock);
	return r;
}

int conf_load_file(_ROOT_TYPE dir,const char*file){
	return _conf_load_file(dir,file,false);
}

int conf_include_file(_ROOT_TYPE dir,const char*file){
	return _conf_load_file(dir,file,true);
}

int conf_save_file(_ROOT_TYPE dir,const char*file){
	int r=0;
	static char xpath[PATH_MAX];
	if(!file)ERET(EINVAL);
	#ifdef ENABLE_UEFI
	if(!dir)ERET(EINVAL);
	#else
	if(dir<0&&dir!=AT_FDCWD)ERET(EINVAL);
	#endif
	struct conf_file_hand*hand=find_hand_by_file(file);
	if(!hand)ERET(EINVAL);
	if(!hand->save)ERET(ENOSYS);
	if(!hand->initialized){
		MUTEX_INIT(hand->lock);
		hand->initialized=true;
	}
	MUTEX_LOCK(hand->lock);
	memset(xpath,0,sizeof(xpath));
	strncpy(xpath,file,sizeof(xpath)-1);
	hand->dir=dir;
	hand->write=conf_write;
	hand->read=NULL;
	hand->path=xpath;
	r=do_save(hand);
	if(r==0){
		r=hand->save(hand);
		do_close(hand,true);
		if(r==0)errno=0;
	}else{
		tlog_warn("save failed");
		r=-1;
	}
	hand->include=false;
	hand->dir=0;
	hand->write=NULL;
	hand->read=NULL;
	hand->path=NULL;
	MUTEX_UNLOCK(hand->lock);
	return r;
}

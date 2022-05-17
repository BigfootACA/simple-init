/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<Library/UefiLib.h>
#include<Library/BaseLib.h>
#include<Library/PcdLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileSystemInfo.h>
#include"uefi.h"
#include"logger.h"
#include"compatible.h"
#include"confd_internal.h"
#define TAG "conf"

int confd=-1;
static char*def_path=NULL;
static EFI_FILE_PROTOCOL*def_fp=NULL;

static void hand2fp(EFI_HANDLE hand,UINTN i,EFI_FILE_PROTOCOL**out){
	EFI_STATUS st;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*proto=NULL;
	proto=NULL,st=gBS->HandleProtocol(
		hand,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&proto
	);
	if(EFI_ERROR(st)||!proto){
		tlog_warn(
			"handle Simple File System Protocol"
			" %p(%lld) failed: %s",
			hand,(unsigned long long)i,
			efi_status_to_string(st)
		);
		return;
	}
	st=proto->OpenVolume(proto,out);
	if(EFI_ERROR(st)||!*out)tlog_warn(
		"open volume %p(%lld) failed: %s",
		proto,(unsigned long long)i,
		efi_status_to_string(st)
	);
}

static void set_default(EFI_FILE_PROTOCOL*fp,char*path,...){
	if(def_path)free(def_path);
	if(def_fp)def_fp->Close(def_fp);
	if(!(def_path=malloc(PATH_MAX)))return;
	ZeroMem(def_path,PATH_MAX);
	va_list va;
	va_start(va,path);
	vsnprintf(def_path,PATH_MAX-1,path,va);
	va_end(va);
	def_fp=fp;
	tlog_debug(
		"use %p:%s as default config path",
		def_fp,def_path
	);
}

static void load_default(){
	EFI_STATUS st;
	UINTN cnt=0,size,i;
	EFI_HANDLE*hands=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	EFI_PARTITION_INFO_PROTOCOL**pis=NULL;
	if(def_fp&&def_path)return;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiPartitionInfoProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)||!hands||cnt<=0){
		tlog_warn(
			"failed to handle PartitionInfo: %s",
			efi_status_to_string(st)
		);
		return;
	}
	size=(cnt+1)*sizeof(EFI_FILE_PROTOCOL*);
	if(!(pis=AllocateZeroPool(size)))
		EDONE(tlog_warn("alloc file protocols failed"));
	for(i=0;i<cnt;i++)gBS->HandleProtocol(
		hands[i],
		&gEfiPartitionInfoProtocolGuid,
		(VOID**)&pis[i]
	);

	for(i=0;i<cnt;i++){
		if(!pis[i]||!hands[i])continue;
		if(pis[i]->Type!=PARTITION_TYPE_GPT)continue;
		if(StrCmp(pis[i]->Info.Gpt.PartitionName,L"logfs")==0)goto found;
		if(StrCmp(pis[i]->Info.Gpt.PartitionName,L"LOGFS")==0)goto found;
	}
	for(i=0;i<cnt;i++){
		if(!pis[i]||!hands[i])continue;
		if(pis[i]->Type!=PARTITION_TYPE_GPT)continue;
		if(StrCmp(pis[i]->Info.Gpt.PartitionName,L"esp")==0)goto found;
		if(StrCmp(pis[i]->Info.Gpt.PartitionName,L"ESP")==0)goto found;
	}
	for(i=0;i<cnt;i++)if(pis[i]&&hands[i]&&pis[i]->System)goto found;
	tlog_warn("no logfs or esp found");
	done:
	if(pis)FreePool(pis);
	return;
	found:
	fp=NULL;
	hand2fp(hands[i],i,&fp);
	if(fp)set_default(
		fp,"%s.cfg",
		PcdGetPtr(PcdConfDefaultPrefix)
	);
	goto done;
}

int confd_init(){
	EFI_STATUS st;
	UINTN cnt=0,size=0;
	EFI_HANDLE*hands=NULL;
	char**exts=NULL,*path=NULL;
	EFI_FILE_PROTOCOL**fps=NULL;
	EFI_FILE_SYSTEM_INFO*fsi=NULL;
	if(!(exts=conf_get_supported_exts()))
		EDONE(tlog_warn("get supported exts failed"));
	if(!(path=AllocatePool(PATH_MAX)))
		EDONE(tlog_warn("alloc path failed"));
	if(def_fp)def_fp->Close(def_fp);
	if(def_path)free(def_path);
	def_fp=NULL,def_path=NULL;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimpleFileSystemProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)||cnt<0||!hands)EDONE(tlog_warn(
		"locate Simple File System Protocol failed: %s",
		efi_status_to_string(st)
	));
	size=(cnt+1)*sizeof(EFI_FILE_PROTOCOL*);
	if(!(fps=AllocateZeroPool(size)))
		EDONE(tlog_warn("alloc file protocols failed"));
	for(UINTN i=0;i<cnt;i++)hand2fp(hands[i],i,&fps[i]);
	load_default();
	for(UINTN i=0;i<cnt;i++)for(size_t x=0;exts[x];x++){
		ZeroMem(path,PATH_MAX);
		AsciiSPrint(
			path,PATH_MAX,"%a.%a",
			PcdGetPtr(PcdConfDefaultStaticPrefix),
			exts[x]
		);
		if(conf_include_file(fps[i],path)==0){
			tlog_debug(
				"loaded %s from %p(%lld)",
				path,fps[i],(unsigned long long)i
			);
		}
	}
	for(UINTN i=0;i<cnt;i++)for(size_t x=0;exts[x];x++){
		ZeroMem(path,PATH_MAX);
		AsciiSPrint(
			path,PATH_MAX,"%a.%a",
			PcdGetPtr(PcdConfDefaultPrefix),
			exts[x]
		);
		if(conf_load_file(fps[i],path)==0){
			tlog_debug(
				"loaded %s from %p(%llu)",
				path,fps[i],(unsigned long long)i
			);
			set_default(fps[i],path);
			fps[i]=NULL;
		}
	}
	if(!def_fp||!def_path)for(UINTN i=0;i<cnt;i++){
		fsi=NULL,st=efi_file_get_info(
			fps[i],&gEfiFileSystemInfoGuid,
			NULL,(VOID**)&fsi
		);
		if(EFI_ERROR(st)||!fsi)continue;
		if(!fsi->ReadOnly&&fsi->FreeSpace>0x20000){
			tlog_debug(
				"fallback use first usable filesystem %p(%llu)",
				fps[i],(unsigned long long)i
			);
			set_default(
				fps[i],"%s.cfg",
				PcdGetPtr(PcdConfDefaultPrefix)
			);
		}
		FreePool(fsi);
		if(def_fp&&def_path)break;
	}
	if(!def_fp||!def_path)tlog_warn("no default config save path");
	done:
	if(fps){
		if(cnt>0)for(UINTN i=0;i<cnt;i++)
			if(fps[i])fps[i]->Close(fps[i]);
		FreePool(fps);
	}
	if(path)FreePool(path);
	if(exts)free(exts);
	return 0;
}

int open_confd_socket(
	bool quiet __attribute__((unused)),
	char*tag __attribute__((unused)),
	char*path __attribute__((unused))
){
	return -1;
}

int set_confd_socket(int fd __attribute__((unused))){
	return -1;
}

void close_confd_socket(){}

int confd_quit(){
	return -1;
}

int confd_dump(enum log_level level){
	conf_dump_store(level);
	return 0;
}

int confd_delete(const char*path){
	return conf_del(path,0,0);
}

int confd_set_integer(const char*path,int64_t data){
	return conf_set_integer(path,data,0,0);
}

int confd_set_string(const char*path,char*data){
	char*s=strdup(data);
	if(!s)ERET(ENOMEM);
	char*c=conf_get_string(path,NULL,0,0);
	if(c)free(c);
	return conf_set_string(path,s,0,0);
}

int confd_set_boolean(const char*path,bool data){
	return conf_set_boolean(path,data,0,0);
}

char**confd_ls(const char*path){
	const char**x=conf_ls(path,0,0);
	if(!x)return NULL;
	size_t as=0,ss=0,al,vl;
	for(as=0;x[as];as++)ss+=strlen(x[as])+1;
	al=sizeof(char*)*(as+1),vl=sizeof(char)*(ss+1);
	char**rx=malloc(al),*vx=malloc(vl);
	if(!rx||!vx){
		free(x);
		if(rx)free(rx);
		if(vx)free(vx);
		EPRET(ENOMEM);
	}
	memset(rx,0,al);
	memset(vx,0,vl);
	for(size_t i=0;i<as;i++){
		const char*v=x[i];
		strcpy(vx,v);
		rx[i]=vx,vx+=strlen(v)+1;
	}
	if(as==0)free(vx);
	return rx;
}

int confd_load_file(EFI_FILE_PROTOCOL*fd,const char*file){
	const char*fn=file?file:def_path;
	EFI_FILE_PROTOCOL*fp=fd?fd:def_fp;
	return conf_load_file(fp,fn);
}

int confd_include_file(EFI_FILE_PROTOCOL*fd,const char*file){
	if(!fd||!file)return -1;
	return conf_include_file(fd,file);
}

int confd_save_file(EFI_FILE_PROTOCOL*fd,const char*file){
	const char*fn=file?file:def_path;
	EFI_FILE_PROTOCOL*fp=fd?fd:def_fp;
	return conf_save_file(fp,fn);
}

int64_t confd_count(const char*path){
	return conf_count(path,0,0);
}

enum conf_type confd_get_type(const char*path){
	return conf_get_type(path,0,0);
}

char*confd_get_string(const char*path,char*def){
	char*x=conf_get_string(path,def,0,0);
	if(!x)x=def;
	return x?strdup(x):NULL;
}

char*confd_get_sstring(const char*path,char*def,char*buf,size_t len){
	char*x=conf_get_string(path,NULL,0,0);
	memset(buf,0,len);
	if(x||def)strncpy(buf,x?x:def,len-1);
	return buf;
}

int64_t confd_get_integer(const char*path,int64_t def){
	return conf_get_integer(path,def,0,0);
}

bool confd_get_boolean(const char*path,bool def){
	return conf_get_boolean(path,def,0,0);
}

int confd_rename(const char*path,const char*name){
	return conf_rename(path,name,0,0);
}

int confd_set_save(const char*path,bool save){
	return conf_set_save(path,save,0,0);
}

bool confd_get_save(const char*path){
	return conf_get_save(path,0,0);
}

int confd_get_own(const char*path,uid_t*own){
	return conf_get_own(path,own,0,0);
}

int confd_get_grp(const char*path,uid_t*grp){
	return conf_get_grp(path,grp,0,0);
}

int confd_get_mod(const char*path,mode_t*mod){
	return conf_get_mod(path,mod,0,0);
}

int confd_set_own(const char*path,uid_t own){
	return conf_set_own(path,own,0,0);
}

int confd_set_grp(const char*path,uid_t grp){
	return conf_set_grp(path,grp,0,0);
}

int confd_set_mod(const char*path,mode_t mod){
	return conf_set_mod(path,mod,0,0);
}

int confd_add_key(const char*path){
	return conf_add_key(path,0,0);
}

#define _EXT_BASE(ret,func,ret_func,...) \
ret func##_base(const char*base,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX];\
	memset(xpath,0,sizeof(xpath));\
	snprintf(xpath,PATH_MAX-1,"%s.%s",base,path);\
	return ret_func;\
}
#define _EXT_DICT(ret,func,ret_func,...) \
ret func##_dict(const char*base,const char*key,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX];\
	memset(xpath,0,sizeof(xpath));\
	snprintf(xpath,PATH_MAX-1,"%s.%s.%s",base,key,path);\
	return ret_func;\
}
#define _EXT_ARRAY(ret,func,ret_func,...) \
ret func##_array(const char*base,int index,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX];\
	memset(xpath,0,sizeof(xpath));\
	snprintf(xpath,PATH_MAX-1,"%s.%d.%s",base,index,path);\
	return ret_func;\
}

#define EXT_BASE(func,arg,type,ret) _EXT_BASE(ret,func,func(xpath,arg),,type arg)
#define EXT_DICT(func,arg,type,ret) _EXT_DICT(ret,func,func(xpath,arg),,type arg)
#define EXT_ARRAY(func,arg,type,ret) _EXT_ARRAY(ret,func,func(xpath,arg),,type arg)
#define XEXT_BASE(func,ret) _EXT_BASE(ret,func,func(xpath),)
#define XEXT_DICT(func,ret) _EXT_DICT(ret,func,func(xpath),)
#define XEXT_ARRAY(func,ret) _EXT_ARRAY(ret,func,func(xpath),)

#define EXT(func,arg,type,ret) \
	EXT_BASE(func,arg,type,ret) \
	EXT_DICT(func,arg,type,ret) \
	EXT_ARRAY(func,arg,type,ret)
#define XEXT(func,ret) \
	XEXT_BASE(func,ret) \
	XEXT_DICT(func,ret) \
	XEXT_ARRAY(func,ret)

_EXT_BASE(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
_EXT_DICT(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
_EXT_ARRAY(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
EXT(confd_set_integer, data,int64_t,int);
EXT(confd_set_string,  data,char*,  int);
EXT(confd_set_boolean, data,bool,   int);
EXT(confd_get_string,  data,char*,  char*);
EXT(confd_get_integer, data,int64_t,int64_t);
EXT(confd_get_boolean, data,bool,   bool);
EXT(confd_set_save,    save,bool,   int);
EXT(confd_get_own,     own,uid_t*,  int);
EXT(confd_get_grp,     grp,gid_t*,  int);
EXT(confd_get_mod,     mod,mode_t*, int);
EXT(confd_set_own,     own,uid_t,   int);
EXT(confd_set_grp,     grp,gid_t,   int);
EXT(confd_set_mod,     mod,mode_t,  int);
EXT(confd_rename,      name,const char*,int);
XEXT(confd_get_save,   bool);
XEXT(confd_add_key,    int);
XEXT(confd_delete,     int);
XEXT(confd_count,      int64_t);
XEXT(confd_ls,         char**);
XEXT(confd_get_type,   enum conf_type);

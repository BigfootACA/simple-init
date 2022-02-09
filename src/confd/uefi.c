/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<Library/BaseLib.h>
#include<Library/PcdLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include"logger.h"
#include"confd_internal.h"
#define TAG "conf"

int confd=-1;

static EFI_HANDLE get_usable_pi(){
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiPartitionInfoProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_error("locate Partition Info Protocol failed: 0x%llx",st);
		return NULL;
	}
	char*match[]={
		(char*)PcdGetPtr(PcdConfGptPartition),
		"logfs",
		"esp",
		NULL
	};
	char partname[256];
	for(UINTN i=0;i<cnt;i++){
		EFI_PARTITION_INFO_PROTOCOL*pi=NULL;
		st=gBS->HandleProtocol(hands[i],&gEfiPartitionInfoProtocolGuid,(VOID**)&pi);
		if(EFI_ERROR(st)||!pi)continue;
		if(pi->Type!=PARTITION_TYPE_GPT)continue;
		memset(partname,0,sizeof(partname));
		UnicodeStrToAsciiStrS(pi->Info.Gpt.PartitionName,partname,sizeof(partname));
		for(int c=0;match[c];c++)if(strcmp(match[c],partname)==0)return hands[i];
	}
	return NULL;
}

static EFI_HANDLE get_usable_fs(){
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimpleFileSystemProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_error("locate Simple File System Protocol failed: 0x%llx",st);
		return NULL;
	}
	for(UINTN i=0;i<cnt;i++){
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs=NULL;
		st=gBS->HandleProtocol(hands[i],&gEfiSimpleFileSystemProtocolGuid,(VOID**)&fs);
		if(EFI_ERROR(st)||!fs)continue;
		return hands[i];
	}
	return NULL;
}

static EFI_FILE_PROTOCOL*get_usable_fp(){
	EFI_STATUS st;
	EFI_HANDLE*hand=NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*proto=NULL;
	static EFI_FILE_PROTOCOL*fp=NULL;
	if(fp)return fp;
	if(!hand)hand=get_usable_pi();
	if(!hand)hand=get_usable_fs();
	if(!hand)return NULL;
	st=gBS->HandleProtocol(hand,&gEfiSimpleFileSystemProtocolGuid,(VOID**)&proto);
	if(EFI_ERROR(st)){
		tlog_error("handle Simple File System Protocol failed: 0x%llx",st);
		return NULL;
	}
	st=proto->OpenVolume(proto,&fp);
	if(EFI_ERROR(st)){
		tlog_error("OpenVolume failed: 0x%llx",st);
		fp=NULL;
		return NULL;
	}
	return fp;
}

int open_confd_socket(bool quiet __attribute__((unused)),char*tag __attribute__((unused)),char*path __attribute__((unused))){
	return -1;
}

int set_confd_socket(int fd __attribute__((unused))){
	return -1;
}

void close_confd_socket(){}

int confd_quit(){
	return -1;
}

int confd_dump(){
	conf_dump_store();
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
	const char*fn=file?file:(const char*)PcdGetPtr(PcdConfDefaultPath);
	EFI_FILE_PROTOCOL*fp=fd?fd:get_usable_fp();
	return conf_load_file(fp,fn);
}

int confd_save_file(EFI_FILE_PROTOCOL*fd,const char*file){
	const char*fn=file?file:(const char*)PcdGetPtr(PcdConfDefaultPath);
	EFI_FILE_PROTOCOL*fp=fd?fd:get_usable_fp();
	return conf_save_file(fp,fn);
}

enum conf_type confd_get_type(const char*path){
	return conf_get_type(path,0,0);
}

char*confd_get_string(const char*path,char*def){
	char*x=conf_get_string(path,def,0,0);
	if(!x)x=def;
	return x?strdup(x):NULL;
}

int64_t confd_get_integer(const char*path,int64_t def){
	return conf_get_integer(path,def,0,0);
}

bool confd_get_boolean(const char*path,bool def){
	return conf_get_boolean(path,def,0,0);
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

#define _EXT_BASE(ret,func,ret_func,...) \
ret func##_base(const char*base,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s",base,path);\
	return ret_func;\
}
#define _EXT_DICT(ret,func,ret_func,...) \
ret func##_dict(const char*base,const char*key,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s.%s",base,key,path);\
	return ret_func;\
}
#define _EXT_ARRAY(ret,func,ret_func,...) \
ret func##_array(const char*base,int index,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
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
XEXT(confd_get_save,   bool);
XEXT(confd_add_key,    int);
XEXT(confd_delete,     int);
XEXT(confd_ls,         char**);
XEXT(confd_get_type,   enum conf_type);

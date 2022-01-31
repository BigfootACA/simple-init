/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#ifdef ENABLE_UEFI
#include<Guid/FileInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Library/MemoryAllocationLib.h>
#else
#include<sys/mman.h>
#endif
#include"str.h"
#include"logger.h"
#include"confd_internal.h"
#define TAG "config"

static char*str_escape(char*str){
	if(!str)EPRET(EINVAL);
	size_t xs=strlen(str)*2+1;
	char*dup=malloc(xs);
	if(!dup)EPRET(ENOMEM);
	memset(dup,0,xs);
	char*ptr_dup=dup,*ptr_str=str;
	while(*ptr_str){
		switch(*ptr_str){
			case '"':*ptr_dup++='\\',*ptr_dup++='\"';break;
			case '\\':*ptr_dup++='\\',*ptr_dup++='\\';break;
			case '\t':*ptr_dup++='\\',*ptr_dup++='t';break;
			case '\n':*ptr_dup++='\\',*ptr_dup++='n';break;
			case '\r':*ptr_dup++='\\',*ptr_dup++='r';break;
			default:*ptr_dup++=*ptr_str;
		}
		ptr_str++;
	}
	return dup;
}

static char*str_unescape(char*str){
	if(!str)EPRET(EINVAL);
	size_t xs=strlen(str)+1;
	char*dup=malloc(xs);
	if(!dup)EPRET(ENOMEM);
	memset(dup,0,xs);
	char*ptr_dup=dup,*ptr_str=str;
	while(*ptr_str){
		switch(*ptr_str){
			case '\\':
				switch(*(++ptr_str)){
					case 't':*ptr_dup++='\t';break;
					case 'n':*ptr_dup++='\n';break;
					case 'r':*ptr_dup++='\r';break;
					default:*ptr_dup++=*ptr_str;break;
				}
			break;
			default:*ptr_dup++=*ptr_str;
		}
		ptr_str++;
	}
	return dup;
}

static char*get_buffer(size_t size){
	static size_t buf_size=0;
	static char*buf=NULL;
	if(size==0){
		free(buf);
		buf=NULL,buf_size=0;
		return NULL;
	}
	if(size>buf_size||!buf){
		if(buf)free(buf);
		buf_size=size,buf=malloc(size);
		if(buf)memset(buf,0,size);
	}
	return buf;
}

static void str_write(_ROOT_TYPE fd,char*str){
	size_t s=strlen(str);
	#ifdef ENABLE_UEFI
	UINTN w=(UINTN)s;
	fd->Write(fd,&w,str);
	#else
	write(fd,str,s);
	#endif
}

static int print_conf(_ROOT_TYPE fd,struct conf*key,const char*name){
	size_t size=0;
	char path[PATH_MAX]={0},*buf,*str;
	if(!key->save&&!*name)return 0;
	if(key->name[0]){
		if(!name[0])strcpy(path,key->name);
		else snprintf(path,PATH_MAX-1,"%s.%s",name,key->name);
	}
	if(key->type==TYPE_KEY){
		list*p=list_first(key->keys);
		if(!p)return 0;
		do{print_conf(fd,LIST_DATA(p,struct conf*),path);}while((p=p->next));
	}else switch(key->type){
		case TYPE_STRING:
			if(!(str=str_escape(VALUE_STRING(key))))break;
			size=strlen(str)+strlen(path)+10;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = \"%s\"\n",path,str);
				str_write(fd,buf);
			}
			free(str);
		break;
		case TYPE_INTEGER:
			size=strlen(path)+50;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = %lld\n",path,(long long int)VALUE_INTEGER(key));
				str_write(fd,buf);
			}
		break;
		case TYPE_BOOLEAN:
			size=strlen(path)+15;
			if((buf=get_buffer(size))){
				snprintf(buf,size-1,"%s = %s\n",path,BOOL2STR(VALUE_BOOLEAN(key)));
				str_write(fd,buf);
			}
		break;
		default:;
	}
	return 0;
}

static int conf_save(_ROOT_TYPE dir,const char*path){
	_ROOT_TYPE fd;
	struct conf*store=conf_get_store();
	if(!store)ERET(EINVAL);
	char xpath[PATH_MAX]={0};
	strncpy(xpath,path,PATH_MAX-1);
	#ifdef ENABLE_UEFI
	char*cp=xpath;
	CHAR16 xp[PATH_MAX]={0};
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	mbstowcs(xp,xpath,sizeof(xp)-1);
	EFI_STATUS st=dir->Open(dir,&fd,xp,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,0);
	if(!EFI_ERROR(st))fd->Delete(fd);
	fd=NULL,st=dir->Open(dir,&fd,xp,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);
	if(EFI_ERROR(st)){
		tlog_warn("open config '%s' failed: %llx",path,st);
		return -efi_status_to_errno(st);
	}
	#else
	if((fd=openat(dir,path,O_WRONLY|O_CREAT|O_TRUNC,0644))<0){
		telog_warn("open config '%s' failed",path);
		return -errno;
	}
	#endif
	str_write(fd,"#!/usr/bin/confctl -L\n");
	str_write(fd,"# -*- coding: utf-8 -*-\n");
	str_write(fd,"##\n## Simple Init Configuration Store\n##\n\n");
	print_conf(fd,store,"");
	get_buffer(0);
	#ifdef ENABLE_UEFI
	fd->Close(fd);
	#else
	close(fd);
	sync();
	#endif
	return 0;
}

static bool string_is_true(char*string){
	return
		strcasecmp(string,"on")==0||
		strcasecmp(string,"yes")==0||
		strcasecmp(string,"true")==0||
		strcasecmp(string,"enable")==0||
		strcasecmp(string,"enabled")==0;
}

static bool string_is_false(char*string){
	return
		strcasecmp(string,"no")==0||
		strcasecmp(string,"off")==0||
		strcasecmp(string,"false")==0||
		strcasecmp(string,"disable")==0||
		strcasecmp(string,"disabled")==0;
}

static void line_set_string(char*key,char*value,size_t len){
	value[len-1]=0,value++;
	char*old=conf_get_string(key,NULL,0,0);
	char*val=str_unescape(value);
	if(!val)return;
	if(old)free(old);
	conf_set_string(key,val,0,0);
}

static void conf_parse_line(int*err,const char*name,size_t n,char*data){
	if(!data)return;
	char*p=strchr(data,'=');
	if(!p){
		if(!check_valid(data,CONF_KEY_CHARS))goto inv_key;
		size_t ds=strlen(data);
		if(ds==0)goto inv_key;
		if(strncmp(data,"runtime.",MIN(7,ds))==0)goto runtime;
		conf_del(data,0,0);
		return;
	}
	*p=0;
	char*key=data,*value=p+1;
	trim(key);
	trim(value);
	size_t ks=strlen(key),vs=strlen(value);
	if(ks==0)goto inv_key;
	if(vs==0)goto inv_val;
	if(!check_valid(key,CONF_KEY_CHARS))goto inv_key;
	if(strncmp(key,"runtime.",MIN(7,ks))==0)goto runtime;
	if(value[0]=='\''){
		if(vs<2||value[vs-1]!='\'')goto inv_val;
		line_set_string(key,value,vs);
	}else if(value[0]=='"'){
		if(vs<2||value[vs-1]!='"')goto inv_val;
		line_set_string(key,value,vs);
	}else if(string_is_true(value))conf_set_boolean(key,true,0,0);
	else if(string_is_false(value))conf_set_boolean(key,false,0,0);
	else{
		char*end;
		errno=0;
		int64_t i=strtol(value,&end,0);
		if(errno!=0||end==value)goto inv_val;
		conf_set_integer(key,i,0,0);
	}
	return;
	runtime:
	tlog_debug("%s: skip runtime config in line %zu",name,n);
	return;
	inv_key:
	tlog_warn("%s: invalid key in line %zu",name,n);
	(*err)++;
	return;
	inv_val:
	tlog_warn("%s: invalid value in line %zu",name,n);
	(*err)++;
}

static void conf_parse_bare_line(int*err,const char*name,size_t n,char*data){
	if(!data)return;
	trim(data);

	// empty line
	if(strlen(data)<=0)return;

	// comment line
	if(*data=='#'||strncmp(data,"//",2)==0)return;

	conf_parse_line(err,name,n,data);
}

static int conf_parse(const char*path,char*data,size_t len){
	char*buff=malloc(len);
	if(!buff)ERET(ENOMEM);
	int err=0;
	char last=0,cur=0;
	size_t dx=0,bx=0,n=0;
	memset(buff,0,len);
	const char*name=path;
	for(size_t x=strlen(name)-1;x>0;x--)if(name[x]=='/'){
		name+=x+1;
		break;
	}
	while(dx<len&&bx<len){
		cur=data[dx];
		switch(cur){
			case '\n':case '\r':
				if(last!='\r'||cur!='\n')n++;
				if(bx==0)break;
				conf_parse_bare_line(&err,name,n,buff);
				if(err>16){
					tlog_warn("too many errors, stop parse config");
					free(buff);
					ERET(EINVAL);
				}
				bx=0;
				memset(buff,0,len);
			break;
			default:buff[bx++]=cur;
		}
		last=cur,dx++;
	}
	free(buff);
	return 0;
}

static int conf_load(_ROOT_TYPE dir,const char*path){
	int r=0;
	void*data;
	struct conf*store=conf_get_store();
	errno=0;
	if(!store)ERET(EINVAL);
	char xpath[PATH_MAX]={0};
	strncpy(xpath,path,PATH_MAX-1);
	#ifdef ENABLE_UEFI
	EFI_FILE_PROTOCOL*fd=NULL;
	EFI_FILE_INFO*info=NULL;
	UINTN infos=sizeof(EFI_FILE_INFO)+256;
	char *cp=xpath;
	CHAR16 xp[PATH_MAX]={0};
	do{if(*cp=='/')*cp='\\';}while(*cp++);
	mbstowcs(xp,xpath,sizeof(xp)-1);
	EFI_STATUS st=dir->Open(dir,&fd,xp,EFI_FILE_MODE_READ,0);
	if(EFI_ERROR(st)){
		tlog_debug("open config '%s' failed: %llx",path,st);
		r=-efi_status_to_errno(st);
		goto fail;
	}
	if(!(info=AllocateZeroPool(infos))){
		tlog_debug("allocate file info failed");
		r=-ENOMEM;
		goto fail;
	}
	st=fd->GetInfo(fd,&gEfiFileInfoGuid,&infos,info);
	if(EFI_ERROR(st)){
		tlog_debug("get config '%s' file info failed: %llx",path,st);
		r=-efi_status_to_errno(st);
		goto fail;
	}
	UINTN size=info->FileSize;
	if(size>0x400000){
		tlog_debug("config '%s' too large",path);
		r=-EFBIG;
		goto fail;
	}
	if(size>0){
		if(!(data=AllocateZeroPool(size))){
			tlog_debug("allocate file content failed");
			r=ENOMEM;
			goto fail;
		}
		st=fd->Read(fd,&size,data);
		if(EFI_ERROR(st)){
			tlog_debug("read '%s' failed: %llx",path,st);
			FreePool(data);
			r=-efi_status_to_errno(st);
			goto fail;
		}
		r=conf_parse(path,(char*)data,size);
		if(r==0)errno=0;
		FreePool(data);
	}
	fail:
	if(info)FreePool(info);
	if(fd)fd->Close(fd);
	return r;
	#else
	int fd=-1;
	struct stat st;
	if((fd=openat(dir,path,O_RDONLY))<0){
		telog_debug("open config '%s' failed",path);
		r=-errno;
		goto fail;
	}
	if(fstat(fd,&st)<0){
		telog_debug("stat config '%s' failed",path);
		r=-errno;
		goto fail;
	}
	if(st.st_size>0x400000){
		tlog_debug("config '%s' too large",path);
		r=-EFBIG;
		goto fail;
	}
	if(st.st_size!=0){
		if(!(data=mmap(0,st.st_size,PROT_READ,MAP_PRIVATE,fd,0))){
			telog_debug("mmap config '%s' failed",path);
			r=-errno;
			goto fail;
		}
		r=conf_parse(path,(char*)data,st.st_size);
		conf_store_changed=true;
		munmap(data,st.st_size);
	}
	fail:
	if(fd>0)close(fd);
	return r;
	#endif
}

struct conf_file_hand conf_hand_conf={
	.ext=(char*[]){"conf","cfg","txt",NULL},
	.load=conf_load,
	.save=conf_save,
};

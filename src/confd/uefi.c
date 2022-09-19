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
#include"uefi.h"
#include"logger.h"
#include"filesystem.h"
#include"compatible.h"
#include"confd_internal.h"
#define TAG "conf"

int confd=-1;
static char*def_path=NULL;
static fsh*def_fp=NULL;

static void set_default(fsh*fp,char*path,...){
	url*u;
	char xpath[1024];
	if(def_path)free(def_path);
	if(def_fp)fs_close(&def_fp);
	if(!(def_path=malloc(PATH_MAX)))return;
	memset(def_path,0,PATH_MAX);
	va_list va;
	va_start(va,path);
	vsnprintf(def_path,PATH_MAX-1,path,va);
	va_end(va);
	def_fp=fp;
	fs_get_url(fp,&u);
	tlog_debug(
		"use %s%s as default config path",
		url_generate(xpath,sizeof(xpath),u),def_path
	);
}

static inline void set_conf_default(fsh*fp){
	set_default(
		fp,"%s.cfg",
		PcdGetPtr(PcdConfDefaultPrefix)
	);
}

static bool try_default(fsvol_info*vol){
	int r;
	fsh*f=NULL;
	if(!vol)return false;
	if(fs_has_vol_feature(vol->features,FSVOL_READONLY))return false;
	if(!fs_has_vol_feature(vol->features,FSVOL_FILES))return false;
	if((r=fsvol_open_volume(vol,&f))!=0||!f)return false;
	set_conf_default(f);
	return true;
}

static void load_default(){
	if(def_fp&&def_path)return;
	if(try_default(fsvol_lookup_by_part_label("logfs")))return;
	if(try_default(fsvol_lookup_by_part_label("esp")))return;
	tlog_warn("no logfs or esp found");
}

int confd_init(){
	fsh*f=NULL;
	size_t i,x;
	bool c=true;
	fsvol_info**vs=NULL;
	char**exts,path[1024],fn[256];
	if(!(exts=conf_get_supported_exts()))
		EDONE(tlog_warn("get supported exts failed"));
	if(def_fp)fs_close(&def_fp);
	if(def_path)free(def_path);
	def_fp=NULL,def_path=NULL;
	load_default();
	if((vs=fsvol_get_volumes())){
		for(i=0;vs[i];i++){
			if(fsvol_open_volume(vs[i],&f)!=0)continue;
			fs_get_path(f,path,sizeof(path));
			for(x=0;exts[x];x++){
				memset(fn,0,sizeof(fn));
				snprintf(
					fn,sizeof(fn),"%s.%s",
					PcdGetPtr(PcdConfDefaultStaticPrefix),
					exts[x]
				);
				if(conf_include_file(f,fn)==0)
					tlog_debug("loaded %s%s",path,fn);
			}
			fs_close(&f);
		}
		for(i=0;vs[i];i++){
			if(fsvol_open_volume(vs[i],&f)!=0)continue;
			fs_get_path(f,path,sizeof(path));
			for(x=0;exts[x];x++){
				memset(fn,0,sizeof(fn));
				snprintf(
					fn,sizeof(fn),"%s.%s",
					PcdGetPtr(PcdConfDefaultPrefix),
					exts[x]
				);
				if(conf_load_file(f,fn)==0){
					tlog_debug("loaded %s%s",path,fn);
					set_default(f,"%s",path);
					c=false;
				}
			}
			if(!c)fs_close(&f);
			c=true;
		}
		for(i=0;vs[i];i++)if(try_default(vs[i]))break;
	}
	if(!def_fp||!def_path)tlog_warn("no default config save path");
	done:
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

int confd_load_file(const char*file){
	const char*fn=file?file:def_path;
	return conf_load_file(def_fp,fn);
}

int confd_include_file(const char*file){
	return conf_include_file(def_fp,file);
}

int confd_save_file(const char*file){
	const char*fn=file?file:def_path;
	return conf_save_file(def_fp,fn);
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

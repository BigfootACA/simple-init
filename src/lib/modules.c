/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<libkmod.h>
#include<sys/utsname.h>
#include"logger.h"
#include"defines.h"
#include"pathnames.h"
#define TAG "kmod"

static char modsdir[PATH_MAX]={0};

static struct kmod_ctx*_new_context(){
	return kmod_new(modsdir[0]?modsdir:NULL,NULL);
}

static bool try_modules_dir(char*dir){
	struct utsname u;
	static int r=-1;
	if(r<0&&(r=uname(&u))<0)return false;
	memset(modsdir,0,sizeof(modsdir));
	snprintf(modsdir,sizeof(modsdir),"%s/%s",dir,u.release);
	return access(modsdir,F_OK)==0;
}

static bool try_modules_dirs(char**dirs){
	for(int i=0;dirs[i];i++)
		if(try_modules_dir(dirs[i]))
			return true;
	return false;
}

static struct kmod_ctx*_new_context_mods(bool log){
	if(!modsdir[0]){
		bool ret=try_modules_dirs((char*[]){
			_PATH_LIB_MODULES,
			_PATH_USR_LIB"/modules",
			_PATH_LIB64"/modules",
			_PATH_USR_LIB64"/modules",
			_PATH_LIB32"/modules",
			_PATH_USR_LIB32"/modules",
			"/modules",
			NULL
		});
		if(!ret&&log)tlog_error("failed to find modules tree");
	}
	struct kmod_ctx*ctx=_new_context();
	if(!ctx)return NULL;
	kmod_load_resources(ctx);
	return ctx;
}

static bool _is_module_loaded(struct kmod_ctx*ctx,const char*name){
	if(!ctx||!name)return false;
	if(access(_PATH_PROC_MODULES,R_OK)!=0)return false;
	bool used=false;
	struct kmod_list*list,*itr;
	kmod_module_new_from_loaded(ctx,&list);
	kmod_list_foreach(itr,list){
		struct kmod_module*mod=kmod_module_get_module(itr);
		if(strcmp(name,kmod_module_get_name(mod))==0)used=true;
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);
	return used;
}

static void _mod_load_err(bool log,int err,const char*name){
	if(log)switch(err){
		case -EEXIST:break;
		case -ENOENT:tlog_warn("could not insert '%s': Unknown symbol or parameter",name);break;
		default:telog_warn("could not insert '%s'",name);break;
	}
}

static int _insmod(struct kmod_ctx*ctx,const char*alias,bool log){
	struct kmod_list*l,*list=NULL;
	int err,flags=0;
	err=kmod_module_new_from_lookup(ctx,alias,&list);
	if(!list||err<0)ERET(ENOENT);
	flags|=KMOD_PROBE_APPLY_BLACKLIST_ALIAS_ONLY;
	kmod_list_foreach(l,list){
		struct kmod_module*mod=kmod_module_get_module(l);
		const char*name=kmod_module_get_name(mod);
		if(!_is_module_loaded(ctx,name)){
			if((err=kmod_module_probe_insert_module(
				mod,
				flags,
				NULL,
				NULL,
				NULL,
				NULL
			))<0)_mod_load_err(log,err,name);
			err=MIN(0,err);
		}
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);
	return err;
}

int insmod(const char*alias,bool log){
	struct kmod_ctx*ctx=_new_context_mods(log);
	if(!ctx)return -1;
	int r=_insmod(ctx,alias,log);
	kmod_unref(ctx);
	return r;
}

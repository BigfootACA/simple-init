/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<string.h>
#include<stdlib.h>
#include"xlua.h"
#include"confd.h"
#include"logger.h"
#include"assets.h"
#ifdef ENABLE_UEFI
#include"uefi.h"
#include"locate.h"
#include<Library/MemoryAllocationLib.h>
#else
#include<unistd.h>
#endif

LUALIB_API void luaL_openlibs(lua_State*L){
	const luaL_Reg *lib;
	for(lib=lua_core_libs;lib->func;lib++){
		luaL_requiref(L,lib->name,lib->func,1);
		lua_pop(L,1);
	}
	for(lib=simple_init_lua_libs;lib->func;lib++){
		luaL_requiref(L,lib->name,lib->func,1);
		lua_pop(L,1);
	}
	for(lib=simple_init_lua_regs;lib->func;lib++){
		lua_register(L,lib->name,lib->func);
	}
}

lua_State*xlua_init(){
	lua_State*L=luaL_newstate();
	luaL_openlibs(L);
	return L;
}

lua_State*xlua_math(){
	lua_State*L=luaL_newstate();
	luaL_requiref(L,LUA_GNAME,luaopen_math,1);
	lua_pop(L,1);
	return L;
}

int xlua_eval_string(lua_State*L,const char*expr){
	if(!L||!expr)return -1;
	lua_settop(L,0);
	int r=xlua_return_string(L,expr);
	if(r==LUA_OK)r=lua_pcall(L,0,LUA_MULTRET,0);
	return r;
}

void xlua_show_error(lua_State*L,char*tag){
	const char*err;
	if(!(err=lua_tostring(L,-1)))return;
	logger_print(LEVEL_ERROR,tag,(char*)err);
	lua_pop(L,1);
}

int xlua_return_string(lua_State*L,const char*expr){
	if(!L||!expr)return -1;
	size_t len=strlen(expr)+16;
	char*code=malloc(len);
	if(!code)return -1;
	memset(code,0,len);
	snprintf(code,len-1,"return (%s)",expr);
	int r=luaL_loadstring(L,code);
	free(code);
	return r;
}

int xlua_create_metatable(
	lua_State*L,
	const char*name,
	const luaL_Reg*funcs,
	lua_CFunction tostring,
	lua_CFunction gc
){
	if(!luaL_newmetatable(L,name))return 0;
	luaL_setfuncs(L,funcs,0);
	lua_pushliteral(L,"__index");
	lua_pushvalue(L,-2);
	lua_settable(L,-3);
	if(tostring){
		lua_pushliteral(L,"__tostring");
		lua_pushstring(L,name);
		lua_pushcclosure(L,tostring,1);
		lua_settable(L,-3);
	}
	if(gc){
		lua_pushliteral(L,"__gc");
		lua_pushstring(L,name);
		lua_pushcclosure(L,gc,1);
		lua_settable(L,-3);
		lua_pushliteral(L,"__close");
		lua_pushstring(L,name);
		lua_pushcclosure(L,gc,1);
		lua_settable(L,-3);
	}
	lua_pushliteral(L,"__metatable");
	lua_pushliteral(L,"invalid access to metatable");
	lua_settable(L,-3);
	lua_pop(L,1);
	return 1;
}

int xlua_loadfile(lua_State*L,const char*name){
	#ifdef ENABLE_UEFI
	locate_ret ret;
	if(boot_locate(&ret,name)&&ret.type==LOCATE_FILE){
		int r=-255;
		VOID*data=NULL;
		UINTN size=0;
		EFI_STATUS status=efi_file_read_whole(ret.file,&data,&size);
		if(data){
			if(!EFI_ERROR(status))
				r=luaL_loadbufferx(L,data,size,name,NULL);
			FreePool(data);
		}
		return r;
	}
	#else
	if(access(name,R_OK)==0)return luaL_loadfile(L,name);
	#endif
	entry_file*f;
	char path[4096];
	memset(path,0,sizeof(path));
	strncpy(path,name,sizeof(path)-1);
	if((f=rootfs_get_assets_file(path)))goto found;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,_PATH_USR"/share/simple-init/lua/%s",name);
	if((f=rootfs_get_assets_file(path)))goto found;
	return -255;
	found:return luaL_loadbufferx(L,f->content,f->length,name,NULL);
}

int xlua_run(lua_State*L,char*tag,const char*name){
	int r=xlua_loadfile(L,name);
	if(r!=LUA_OK){
		log_error(tag,"load lua %s failed",name);
		if(r!=-255)xlua_show_error(L,tag);
		return r;
	}
	r=lua_pcall(L,0,LUA_MULTRET,0);
	if(r!=LUA_OK){
		log_error(tag,"run lua %s failed",name);
		xlua_show_error(L,tag);
	}
	return r;
}

static void xlua_run_conf(lua_State*L,char*tag,char*path){
	if(!L||!tag||!path||!*path)return;
	log_debug(tag,"try run lua script %s...",path);
	xlua_run(L,tag,path);
	free(path);
}

void xlua_run_confd(lua_State*L,char*tag,const char*key,...){
	char path[PATH_MAX],**ks;
	va_list va;
	va_start(va,key);
	memset(path,0,sizeof(path));
	vsnprintf(path,sizeof(path)-1,key,va);
	va_end(va);
	switch(confd_get_type(path)){
		case TYPE_KEY:
			if(!(ks=confd_ls(path)))return;
			for(size_t i=0;ks[i];i++)xlua_run_conf(
				L,tag,confd_get_string_base(path,ks[i],NULL)
			);
			if(ks[0])free(ks[0]);
			free(ks);
		break;
		case TYPE_STRING:xlua_run_conf(
			L,tag,confd_get_string(path,NULL)
		);
		break;
		default:;
	}
}
#endif

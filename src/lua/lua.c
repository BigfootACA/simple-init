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
#include"filesystem.h"

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
	int r=luaL_loadbuffer(L,code,strlen(code),expr);
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

int xlua_loadfile(lua_State*L,fsh*f,const char*name){
	int r=0;
	size_t len=0;
	void*buffer=NULL;
	r=fs_read_whole_file(f,name,&buffer,&len);
	if(r!=0)return r;
	if(!buffer)return EIO;
	r=luaL_loadbufferx(L,buffer,len,name,NULL);
	free(buffer);
	return r;
}

int xlua_run_by(lua_State*L,char*tag,fsh*f,const char*name){
	int r=xlua_loadfile(L,f,name);
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

int xlua_run(lua_State*L,char*tag,const char*name){
	return xlua_run_by(L,tag,NULL,name);
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

void xlua_dump_stack(lua_State*L){
	for(int i=lua_gettop(L);i>0;--i){
		log_debug(
			"lua","stack #%d: type: %s value: %s\n",
			i,lua_typename(L,lua_type(L,i)),
			luaL_tolstring(L,i,NULL)
		);
		lua_pop(L,1);
	}
}
#endif

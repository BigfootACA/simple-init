/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs.h"

static int lua_fs_open(lua_State*L){
	int r=0;
	fsh*nf=NULL;
	fs_file_flag flag=0;
	if(!lua_fs_get_flag(L,2,false,&flag))return 0;
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			r=fs_open(NULL,&nf,path,flag);
		}break;
		case LUA_TUSERDATA:{
			struct lua_url*d1;
			if((d1=luaL_testudata(L,1,LUA_URL))){
				r=fs_open_uri(&nf,d1->u,flag);
				break;
			}
		}
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_fsh_to_lua(L,nf);
	return 3;
}

static int lua_fs_exists(lua_State*L){
	int r=0;
	bool exists=false;
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			r=fs_exists(NULL,path,&exists);
		}break;
		case LUA_TUSERDATA:{
			struct lua_url*d1;
			if((d1=luaL_testudata(L,1,LUA_URL))){
				r=fs_exists_uri(d1->u,&exists);
				break;
			}
		}
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_pushboolean(L,exists);
	lua_pushinteger(L,r);
	return 0;
}

static int lua_fs_read_file(lua_State*L){
	int r=0;
	size_t size=0;
	void*buffer=NULL;
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			r=fs_read_whole_file(NULL,path,&buffer,&size);
		}break;
		case LUA_TUSERDATA:{
			struct lua_url*d1;
			if((d1=luaL_testudata(L,1,LUA_URL))){
				r=fs_read_whole_file_uri(d1->u,&buffer,&size);
				break;
			}
		}
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_data_to_lua(L,true,buffer,size);
	return 3;
}

static int lua_fs_write_file(lua_State*L){
	int r=0;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	lua_arg_get_data(L,3,false,&buffer,&size);
	int64_t len=luaL_optinteger(L,4,size);
	if(len<=0)return luaL_argerror(L,4,"invalid length");
	if(size>0&&(size_t)len>size)return luaL_argerror(L,4,"out of range");
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			r=fs_write_file(NULL,path,buffer,len);
		}break;
		case LUA_TUSERDATA:{
			struct lua_url*d1;
			if((d1=luaL_testudata(L,1,LUA_URL))){
				r=fs_write_file_uri(d1->u,buffer,len);
				break;
			}
		}
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 3;
}

static const luaL_Reg fs_lib[]={
	{"open",       lua_fs_open},
	{"exists",     lua_fs_exists},
	{"read_file",  lua_fs_read_file},
	{"write_file", lua_fs_write_file},
	{NULL, NULL}
};

static struct lua_fs_meta_table*tables[]={
	&lua_fsh,
	&lua_fs_info,
	NULL
};

LUAMOD_API int luaopen_fs(lua_State*L){
	for(size_t i=0;tables[i];i++)xlua_create_metatable(
		L,tables[i]->name,
		tables[i]->reg,
		tables[i]->tostring,
		tables[i]->gc
	);
	luaL_newlib(L,fs_lib);
	return 1;
}

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs.h"

void lua_fsh_to_lua(lua_State*L,fsh*f){
	if(!fsh_check(f)){
		lua_pushnil(L);
		return;
	}
	struct lua_fsh*e;
	e=lua_newuserdata(L,sizeof(struct lua_fsh));
	luaL_getmetatable(L,LUA_FSH);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fsh));
	e->f=f;
}

static int lua_fsh_readdir(lua_State*L){
	int r;
	fs_file_info info;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	memset(&info,0,sizeof(info));
	r=fs_readdir(f->f,&info);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_fs_file_info_to_lua(L,&info);
	return 3;
}

static int lua_fsh_read_all(lua_State*L){
	int r;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	r=fs_read_all(f->f,&buffer,&size);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_data_to_lua(L,true,buffer,size);
	return 3;
}

static int lua_fsh_read(lua_State*L){
	int r;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int64_t len=luaL_checkinteger(L,2);
	if(len<=0)return luaL_argerror(L,2,"invalid length");
	r=fs_read_alloc(f->f,&buffer,len,&size);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_data_to_lua(L,true,buffer,size);
	return 3;
}

static int lua_fsh_full_read(lua_State*L){
	int r;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int64_t len=luaL_checkinteger(L,2);
	if(len<=0)return luaL_argerror(L,2,"invalid length");
	r=fs_full_read_alloc(f->f,&buffer,len);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_data_to_lua(L,true,buffer,len);
	return 3;
}

static int lua_fsh_write(lua_State*L){
	int r;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	lua_arg_get_data(L,2,false,&buffer,&size);
	int64_t len=luaL_optinteger(L,3,size);
	if(len<=0)return luaL_argerror(L,3,"invalid length");
	if(size>0&&(size_t)len>size)return luaL_argerror(L,3,"out of range");
	r=fs_write(f->f,&buffer,len,&size);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_pushinteger(L,size);
	return 3;
}

static int lua_fsh_full_write(lua_State*L){
	int r;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	lua_arg_get_data(L,2,false,&buffer,&size);
	int64_t len=luaL_optinteger(L,3,size);
	if(len<=0)return luaL_argerror(L,3,"invalid length");
	if(size>0&&(size_t)len>size)return luaL_argerror(L,3,"out of range");
	size=0,r=fs_full_write(f->f,&buffer,len);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_print(lua_State*L){
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	const char*str=luaL_checkstring(L,2);
	int r=fs_print(f->f,str);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_printf(lua_State*L){
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	lua_pop(L,1);
	lua_getglobal(L,"string");
	lua_getfield(L,-1,"format");
	lua_insert(L,1);
	lua_pop(L,1);
	lua_pushvalue(L,lua_upvalueindex(1));
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-2,LUA_MULTRET);
	const char*str=luaL_checkstring(L,-1);
	int r=fs_print(f->f,str);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_println(lua_State*L){
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	const char*str=luaL_checkstring(L,2);
	int r=fs_println(f->f,str);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_seek(lua_State*L){
	int wh=SEEK_SET;
	GET_HANDLER(L,1,f);
	size_t pos=luaL_checkinteger(L,2);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	if(!lua_isnoneornil(L,3))switch(lua_type(L,3)){
		case LUA_TSTRING:{
			const char*n=luaL_checkstring(L,3);
			if(strcasecmp(n,"set")==0)wh=SEEK_SET;
			else if(strcasecmp(n,"cur")==0)wh=SEEK_CUR;
			else if(strcasecmp(n,"end")==0)wh=SEEK_END;
			else return luaL_argerror(L,3,"invalid whence");
		}break;
		case LUA_TNUMBER:wh=luaL_checkinteger(L,3);break;
		default:return luaL_argerror(L,3,"invalid whence");
	}
	int r=fs_seek(f->f,pos,wh);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_tell(lua_State*L){
	size_t pos=0;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int r=fs_tell(f->f,&pos);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_pushinteger(L,pos);
	return 3;
}

static int lua_fsh_size(lua_State*L){
	size_t out=0;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int r=fs_get_size(f->f,&out);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_pushinteger(L,out);
	return 3;
}

static int lua_fsh_set_size(lua_State*L){
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	size_t size=luaL_checkinteger(L,2);
	int r=fs_set_size(f->f,size);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_name(lua_State*L){
	char name[4096];
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	memset(name,0,sizeof(name));
	int r=fs_get_name(f->f,name,sizeof(name));
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_pushstring(L,name);
	return 3;
}

static int lua_fsh_path(lua_State*L){
	char*path=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int r=fs_get_path_alloc(f->f,&path);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	if(path){
		lua_pushstring(L,path);
		free(path);
	}else lua_pushnil(L);
	return 3;
}

static int lua_fsh_url(lua_State*L){
	url*u=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int r=fs_get_url(f->f,&u);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_url_to_lua(L,u);
	return 3;
}

static int lua_fsh_flush(lua_State*L){
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	int r=fs_flush(f->f);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 2;
}

static int lua_fsh_info(lua_State*L){
	int r;
	fs_file_info info;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	memset(&info,0,sizeof(info));
	r=fs_get_info(f->f,&info);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_fs_file_info_to_lua(L,&info);
	return 3;
}

static int lua_fsh_close(lua_State*L){
	GET_HANDLER(L,1,f);
	if(f->f)fs_close(&f->f);
	f->f=NULL;
	return 0;
}

static int lua_fsh_open(lua_State*L){
	int r=0;
	fsh*nf=NULL;
	fs_file_flag flag=0;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	const char*path=luaL_checkstring(L,2);
	if(!lua_fs_get_flag(L,3,false,&flag))return 0;
	r=fs_open(f->f,&nf,path,flag);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_fsh_to_lua(L,nf);
	return 3;
}

static int lua_fsh_exists(lua_State*L){
	int r=0;
	bool exists=false;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	const char*path=luaL_checkstring(L,2);
	r=fs_exists(f->f,path,&exists);
	lua_pushboolean(L,exists);
	lua_pushinteger(L,r);
	return 3;
}

static int lua_fsh_write_file(lua_State*L){
	int r=0;
	size_t size=0;
	void*buffer=NULL;
	GET_HANDLER(L,1,f);
	if(!f->f)return luaL_argerror(L,1,"invalid fsh");
	const char*path=luaL_checkstring(L,2);
	lua_arg_get_data(L,3,false,&buffer,&size);
	int64_t len=luaL_optinteger(L,4,size);
	if(len<=0)return luaL_argerror(L,4,"invalid length");
	if(size>0&&(size_t)len>size)return luaL_argerror(L,4,"out of range");
	r=fs_write_file(f->f,path,buffer,len);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	return 3;
}

struct lua_fs_meta_table lua_fsh={
	.name=LUA_FSH,
	.reg=(luaL_Reg[]){
		{"open",        lua_fsh_open},
		{"exists",      lua_fsh_exists},
		{"readdir",     lua_fsh_readdir},
		{"read",        lua_fsh_read},
		{"read_all",    lua_fsh_read_all},
		{"full_read",   lua_fsh_full_read},
		{"write",       lua_fsh_write},
		{"write_file",  lua_fsh_write_file},
		{"full_write",  lua_fsh_full_write},
		{"print",       lua_fsh_print},
		{"printf",      lua_fsh_printf},
		{"println",     lua_fsh_println},
		{"seek",        lua_fsh_seek},
		{"tell",        lua_fsh_tell},
		{"url",         lua_fsh_url},
		{"get_url",     lua_fsh_url},
		{"path",        lua_fsh_path},
		{"get_path",    lua_fsh_path},
		{"name",        lua_fsh_name},
		{"get_name",    lua_fsh_name},
		{"size",        lua_fsh_size},
		{"get_size",    lua_fsh_size},
		{"set_size",    lua_fsh_set_size},
		{"truncate",    lua_fsh_set_size},
		{"info",        lua_fsh_info},
		{"get_info",    lua_fsh_info},
		{"stat",        lua_fsh_info},
		{"flush",       lua_fsh_flush},
		{"close",       lua_fsh_close},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fsh_close,
};

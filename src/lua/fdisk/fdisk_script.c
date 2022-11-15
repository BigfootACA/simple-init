/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_script_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_context*parent,
	struct fdisk_script*data
){
	if(!parent||!data){
		lua_pushnil(L);
		return;
	}
	fdisk_ref_context(parent);
	if(!allocated)fdisk_ref_script(data);
	struct lua_fdisk_script*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_script));
	luaL_getmetatable(L,LUA_FDISK_SCRIPT);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_script));
	e->parent=parent,e->data=data;
}

static int lua_fdisk_script_get_header(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	const char*name=luaL_checkstring(L,2);
	const char*ret=fdisk_script_get_header(data->data,name);
	lua_pushstring(L,ret);
	return 1;
}

static int lua_fdisk_script_set_header(lua_State*L){
	LUA_ARG_MAX(3);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	const char*name=luaL_checkstring(L,2);
	const char*val=luaL_checkstring(L,3);
	fdisk_script_set_header(data->data,name,val);
	return 2;
}

static int lua_fdisk_script_get_table(lua_State*L){
	LUA_ARG_MAX(1);
	struct fdisk_table*table=NULL;
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	table=fdisk_script_get_table(data->data);
	if(!table)lua_pushnil(L);
	else lua_fdisk_table_to_lua(L,false,table);
	return 1;
}

static int lua_fdisk_script_set_table(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	struct lua_fdisk_table*table=luaL_checkudata(L,2,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	if(!table||!table->data)return luaL_argerror(L,2,"invalid table");
	fdisk_script_set_table(data->data,table->data);
	return 0;
}

static int lua_fdisk_script_get_nlines(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	lua_pushinteger(L,fdisk_script_get_nlines(data->data));
	return 1;
}

static int lua_fdisk_script_has_force_label(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	lua_pushboolean(L,fdisk_script_has_force_label(data->data));
	return 1;
}

static int lua_fdisk_script_enable_json(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	fdisk_script_enable_json(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_script_write_file(lua_State*L){
	LUA_ARG_MAX(2);
	struct luaL_Stream*stream=NULL;
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	stream=luaL_checkudata(L,2,LUA_FILEHANDLE);
	if(!stream||!stream->f)return luaL_argerror(L,2,"invalid stream");
	lua_pushinteger(L,fdisk_script_write_file(data->data,stream->f));
	return 1;
}

static int lua_fdisk_script_read_line(lua_State*L){
	LUA_ARG_MAX(3);
	char*buffer=NULL;
	struct luaL_Stream*stream=NULL;
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid script");
	stream=luaL_checkudata(L,2,LUA_FILEHANDLE);
	if(!stream||!stream->f)return luaL_argerror(L,2,"invalid stream");
	int size=luaL_optinteger(L,3,1024);
	if(!(buffer=malloc(size+1)))return luaL_error(L,"malloc failed");
	memset(buffer,0,size+1);
	int ret=fdisk_script_read_line(data->data,stream->f,buffer,size);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushstring(L,buffer);
	free(buffer);
	return 2;
}

static int lua_fdisk_script_gc(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_script*data=luaL_checkudata(L,1,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return 0;
	fdisk_unref_context(data->parent);
	fdisk_unref_script(data->data);
	data->data=NULL;
	data->parent=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_script={
	.name=LUA_FDISK_SCRIPT,
	.reg=(luaL_Reg[]){
		{"get_header",      lua_fdisk_script_get_header},
		{"set_header",      lua_fdisk_script_set_header},
		{"get_table",       lua_fdisk_script_get_table},
		{"set_table",       lua_fdisk_script_set_table},
		{"get_nlines",      lua_fdisk_script_get_nlines},
		{"has_force_label", lua_fdisk_script_has_force_label},
		{"enable_json",     lua_fdisk_script_enable_json},
		{"write_file",      lua_fdisk_script_write_file},
		{"read_line",       lua_fdisk_script_read_line},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_script_gc,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_parttype_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_parttype*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_parttype(data);
	struct lua_fdisk_parttype*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_parttype));
	luaL_getmetatable(L,LUA_FDISK_PARTTYPE);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_parttype));
	e->data=data;
}

static int lua_fdisk_parttype_set_name(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	fdisk_parttype_set_name(data->data,luaL_checkstring(L,2));
	return 0;
}

static int lua_fdisk_parttype_set_typestr(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	fdisk_parttype_set_typestr(data->data,luaL_checkstring(L,2));
	return 0;
}

static int lua_fdisk_parttype_set_code(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	fdisk_parttype_set_code(data->data,luaL_checkinteger(L,2));
	return 0;
}

static int lua_fdisk_parttype_copy(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	struct fdisk_parttype*parttype=fdisk_copy_parttype(data->data);
	if(!parttype)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,true,parttype);
	return 1;
}

static int lua_fdisk_parttype_get_string(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	const char*string=fdisk_parttype_get_string(data->data);
	if(!string)lua_pushnil(L);
	else lua_pushstring(L,string);
	return 1;
}

static int lua_fdisk_parttype_get_code(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	lua_pushinteger(L,fdisk_parttype_get_code(data->data));
	return 1;
}

static int lua_fdisk_parttype_get_name(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	const char*name=fdisk_parttype_get_name(data->data);
	if(!name)lua_pushnil(L);
	else lua_pushstring(L,name);
	return 1;
}

static int lua_fdisk_parttype_is_unknown(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_parttype*data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid part type");
	lua_pushboolean(L,fdisk_parttype_is_unknown(data->data));
	return 1;
}

static int lua_fdisk_parttype_gc(lua_State*L){
	struct lua_fdisk_parttype*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return 0;
	fdisk_unref_parttype(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_parttype={
	.name=LUA_FDISK_PARTTYPE,
	.reg=(luaL_Reg[]){
		{"set_name",    lua_fdisk_parttype_set_name},
		{"set_typestr", lua_fdisk_parttype_set_typestr},
		{"set_code",    lua_fdisk_parttype_set_code},
		{"copy",        lua_fdisk_parttype_copy},
		{"get_string",  lua_fdisk_parttype_get_string},
		{"get_code",    lua_fdisk_parttype_get_code},
		{"get_name",    lua_fdisk_parttype_get_name},
		{"is_unknown",  lua_fdisk_parttype_is_unknown},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_parttype_gc,
};

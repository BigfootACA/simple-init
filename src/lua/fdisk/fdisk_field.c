/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_field_to_lua(
	lua_State*L,
	struct fdisk_context*parent,
	struct fdisk_field*data
){
	if(!parent||!data){
		lua_pushnil(L);
		return;
	}
	fdisk_ref_context(parent);
	struct lua_fdisk_field*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_field));
	luaL_getmetatable(L,LUA_FDISK_FIELD);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_field));
	e->parent=parent,e->data=data;
}

static int lua_fdisk_field_get_id(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_field*data=luaL_checkudata(L,1,LUA_FDISK_FIELD);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid field");
	lua_pushinteger(L,fdisk_field_get_id(data->data));
	return 1;
}

static int lua_fdisk_field_get_name(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_field*data=luaL_checkudata(L,1,LUA_FDISK_FIELD);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid field");
	lua_pushstring(L,fdisk_field_get_name(data->data));
	return 1;
}

static int lua_fdisk_field_get_width(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_field*data=luaL_checkudata(L,1,LUA_FDISK_FIELD);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid field");
	lua_pushnumber(L,fdisk_field_get_width(data->data));
	return 1;
}

static int lua_fdisk_field_is_number(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_field*data=luaL_checkudata(L,1,LUA_FDISK_FIELD);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid field");
	lua_pushboolean(L,fdisk_field_is_number(data->data));
	return 1;
}


static int lua_fdisk_field_gc(lua_State*L){
	struct lua_fdisk_field*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_FIELD);
	if(!data||!data->parent)return 0;
	fdisk_unref_context(data->parent);
	data->data=NULL;
	data->parent=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_field={
	.name=LUA_FDISK_FIELD,
	.reg=(luaL_Reg[]){
		{"get_id",    lua_fdisk_field_get_id},
		{"get_name",  lua_fdisk_field_get_name},
		{"get_width", lua_fdisk_field_get_width},
		{"is_number", lua_fdisk_field_is_number},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_field_gc,
};

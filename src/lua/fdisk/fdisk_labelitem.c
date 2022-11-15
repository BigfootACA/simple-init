/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_labelitem_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_labelitem*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_labelitem(data);
	struct lua_fdisk_labelitem*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_labelitem));
	luaL_getmetatable(L,LUA_FDISK_LABELITEM);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_labelitem));
	e->data=data;
}

static int lua_fdisk_labelitem_reset(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	fdisk_reset_labelitem(data->data);
	return 0;
}

static int lua_fdisk_labelitem_get_name(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushstring(L,fdisk_labelitem_get_name(data->data));
	return 1;
}

static int lua_fdisk_labelitem_get_id(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushinteger(L,fdisk_labelitem_get_id(data->data));
	return 1;
}

static int lua_fdisk_labelitem_get_data_u64(lua_State*L){
	LUA_ARG_MAX(1);
	uint64_t val=0;
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushinteger(L,fdisk_labelitem_get_data_u64(data->data,&val));
	lua_pushinteger(L,val);
	return 2;
}

static int lua_fdisk_labelitem_get_data_string(lua_State*L){
	LUA_ARG_MAX(1);
	const char*val=NULL;
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	int ret=fdisk_labelitem_get_data_string(data->data,&val);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!val)lua_pushnil(L);
	else lua_pushstring(L,val);
	return 2;
}

static int lua_fdisk_labelitem_is_string(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushboolean(L,fdisk_labelitem_is_string(data->data));
	return 1;
}

static int lua_fdisk_labelitem_is_number(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_labelitem*data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushboolean(L,fdisk_labelitem_is_number(data->data));
	return 1;
}

static int lua_fdisk_labelitem_gc(lua_State*L){
	struct lua_fdisk_labelitem*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_LABELITEM);
	if(!data||!data->data)return 0;
	fdisk_unref_labelitem(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_labelitem={
	.name=LUA_FDISK_LABELITEM,
	.reg=(luaL_Reg[]){
		{"reset",           lua_fdisk_labelitem_reset},
		{"get_name",        lua_fdisk_labelitem_get_name},
		{"get_id",          lua_fdisk_labelitem_get_id},
		{"get_data_u64",    lua_fdisk_labelitem_get_data_u64},
		{"get_data_string", lua_fdisk_labelitem_get_data_string},
		{"is_string",       lua_fdisk_labelitem_is_string},
		{"is_number",       lua_fdisk_labelitem_is_number},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_labelitem_gc,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_partition_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_partition*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_partition(data);
	struct lua_fdisk_partition*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_partition));
	luaL_getmetatable(L,LUA_FDISK_PARTITION);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_partition));
	e->data=data;
}

static int lua_fdisk_partition_reset(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_reset_partition(data->data);
	return 0;
}

static int lua_fdisk_partition_is_freespace(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_freespace(data->data));
	return 1;
}

static int lua_fdisk_partition_set_start(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_sector_t off=luaL_checkinteger(L,2);
	int ret=fdisk_partition_set_start(data->data,off);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_unset_start(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_unset_start(data->data);
	return 0;
}

static int lua_fdisk_partition_get_start(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushinteger(L,fdisk_partition_get_start(data->data));
	return 1;
}

static int lua_fdisk_partition_has_start(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_has_start(data->data));
	return 1;
}

static int lua_fdisk_partition_start_follow_default(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_start_follow_default(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_partition_start_is_default(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_start_is_default(data->data));
	return 1;
}

static int lua_fdisk_partition_set_size(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_sector_t off=luaL_checkinteger(L,2);
	int ret=fdisk_partition_set_size(data->data,off);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_unset_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_unset_size(data->data);
	return 0;
}

static int lua_fdisk_partition_get_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushinteger(L,fdisk_partition_get_size(data->data));
	return 1;
}

static int lua_fdisk_partition_has_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_has_size(data->data));
	return 1;
}

static int lua_fdisk_partition_size_explicit(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_size_explicit(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_partition_get_end(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushinteger(L,fdisk_partition_get_end(data->data));
	return 1;
}

static int lua_fdisk_partition_has_end(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_has_end(data->data));
	return 1;
}

static int lua_fdisk_partition_end_follow_default(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_end_follow_default(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_partition_end_is_default(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_end_is_default(data->data));
	return 1;
}

static int lua_fdisk_partition_set_partno(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_sector_t off=luaL_checkinteger(L,2);
	int ret=fdisk_partition_set_partno(data->data,off);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_unset_partno(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_unset_partno(data->data);
	return 0;
}

static int lua_fdisk_partition_get_partno(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushinteger(L,fdisk_partition_get_partno(data->data));
	return 1;
}

static int lua_fdisk_partition_has_partno(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_has_partno(data->data));
	return 1;
}

static int lua_fdisk_partition_partno_follow_default(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_partno_follow_default(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_partition_set_type(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	struct lua_fdisk_parttype*type=luaL_checkudata(L,2,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	if(!type||!type->data)return luaL_argerror(L,2,"invalid partition type");
	int ret=fdisk_partition_set_type(data->data,type->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_get_type(lua_State*L){
	LUA_ARG_MAX(1);
	struct fdisk_parttype*type=NULL;
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	type=fdisk_partition_get_type(data->data);
	if(!type)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,false,type);
	return 1;
}

static int lua_fdisk_partition_set_name(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	int ret=fdisk_partition_set_name(data->data,luaL_checkstring(L,2));
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_get_name(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushstring(L,fdisk_partition_get_name(data->data));
	return 1;
}

static int lua_fdisk_partition_set_uuid(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	int ret=fdisk_partition_set_uuid(data->data,luaL_checkstring(L,2));
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_get_uuid(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushstring(L,fdisk_partition_get_uuid(data->data));
	return 1;
}

static int lua_fdisk_partition_set_attrs(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	int ret=fdisk_partition_set_attrs(data->data,luaL_checkstring(L,2));
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_get_attrs(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushstring(L,fdisk_partition_get_attrs(data->data));
	return 1;
}

static int lua_fdisk_partition_is_nested(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_nested(data->data));
	return 1;
}

static int lua_fdisk_partition_is_container(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_container(data->data));
	return 1;
}

static int lua_fdisk_partition_get_parent(lua_State*L){
	LUA_ARG_MAX(1);
	size_t parent=0;
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	fdisk_partition_get_parent(data->data,&parent);
	lua_pushinteger(L,parent);
	return 1;
}

static int lua_fdisk_partition_is_used(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_used(data->data));
	return 1;
}

static int lua_fdisk_partition_is_bootable(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_bootable(data->data));
	return 1;
}

static int lua_fdisk_partition_is_wholedisk(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	lua_pushboolean(L,fdisk_partition_is_wholedisk(data->data));
	return 1;
}

static int lua_fdisk_partition_to_string(lua_State*L){
	LUA_ARG_MAX(3);
	char*str=NULL;
	int id=luaL_checkinteger(L,3);
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	struct lua_fdisk_context*ctx=luaL_checkudata(L,2,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	if(!ctx||!ctx->data)return luaL_argerror(L,2,"invalid context");
	int ret=fdisk_partition_to_string(data->data,ctx->data,id,&str);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!str)lua_pushnil(L);
	else lua_pushstring(L,str);
	return 2;
}

static int lua_fdisk_partition_next_partno(lua_State*L){
	LUA_ARG_MAX(2);
	size_t partno=0;
	struct lua_fdisk_partition*data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	struct lua_fdisk_context*ctx=luaL_checkudata(L,2,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid partition");
	if(!ctx||!ctx->data)return luaL_argerror(L,2,"invalid context");
	int ret=fdisk_partition_next_partno(data->data,ctx->data,&partno);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,partno);
	return 2;
}

static int lua_fdisk_partition_gc(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_partition*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_PARTITION);
	if(!data||!data->data)return 0;
	fdisk_unref_partition(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_partition={
	.name=LUA_FDISK_PARTITION,
	.reg=(luaL_Reg[]){
		{"reset",                 lua_fdisk_partition_reset},
		{"is_freespace",          lua_fdisk_partition_is_freespace},
		{"set_start",             lua_fdisk_partition_set_start},
		{"unset_start",           lua_fdisk_partition_unset_start},
		{"get_start",             lua_fdisk_partition_get_start},
		{"has_start",             lua_fdisk_partition_has_start},
		{"start_follow_default",  lua_fdisk_partition_start_follow_default},
		{"start_is_default",      lua_fdisk_partition_start_is_default},
		{"set_size",              lua_fdisk_partition_set_size},
		{"unset_size",            lua_fdisk_partition_unset_size},
		{"get_size",              lua_fdisk_partition_get_size},
		{"has_size",              lua_fdisk_partition_has_size},
		{"size_explicit",         lua_fdisk_partition_size_explicit},
		{"get_end",               lua_fdisk_partition_get_end},
		{"has_end",               lua_fdisk_partition_has_end},
		{"end_follow_default",    lua_fdisk_partition_end_follow_default},
		{"end_is_default",        lua_fdisk_partition_end_is_default},
		{"set_partno",            lua_fdisk_partition_set_partno},
		{"unset_partno",          lua_fdisk_partition_unset_partno},
		{"get_partno",            lua_fdisk_partition_get_partno},
		{"has_partno",            lua_fdisk_partition_has_partno},
		{"partno_follow_default", lua_fdisk_partition_partno_follow_default},
		{"set_type",              lua_fdisk_partition_set_type},
		{"get_type",              lua_fdisk_partition_get_type},
		{"set_name",              lua_fdisk_partition_set_name},
		{"get_name",              lua_fdisk_partition_get_name},
		{"set_uuid",              lua_fdisk_partition_set_uuid},
		{"get_uuid",              lua_fdisk_partition_get_uuid},
		{"set_attrs",             lua_fdisk_partition_set_attrs},
		{"get_attrs",             lua_fdisk_partition_get_attrs},
		{"is_nested",             lua_fdisk_partition_is_nested},
		{"is_container",          lua_fdisk_partition_is_container},
		{"get_parent",            lua_fdisk_partition_get_parent},
		{"is_used",               lua_fdisk_partition_is_used},
		{"is_bootable",           lua_fdisk_partition_is_bootable},
		{"is_wholedisk",          lua_fdisk_partition_is_wholedisk},
		{"to_string",             lua_fdisk_partition_to_string},
		{"next_partno",           lua_fdisk_partition_next_partno},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_partition_gc,
};

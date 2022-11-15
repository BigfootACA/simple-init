/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_table_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_table*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_table(data);
	struct lua_fdisk_table*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_table));
	luaL_getmetatable(L,LUA_FDISK_TABLE);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_table));
	e->data=data;
}

static int lua_fdisk_table_reset(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	fdisk_reset_table(data->data);
	return 0;
}

static int lua_fdisk_table_get_nents(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	lua_pushinteger(L,fdisk_table_get_nents(data->data));
	return 1;
}

static int lua_fdisk_table_is_empty(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	lua_pushboolean(L,fdisk_table_is_empty(data->data));
	return 1;
}

static int lua_fdisk_table_add_partition(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	struct lua_fdisk_partition*part=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	if(!part||!part->data)return luaL_argerror(L,2,"invalid partition");
	int ret=fdisk_table_add_partition(data->data,part->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_table_remove_partition(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	struct lua_fdisk_partition*part=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	if(!part||!part->data)return luaL_argerror(L,2,"invalid partition");
	int ret=fdisk_table_remove_partition(data->data,part->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_table_wrong_order(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	lua_pushboolean(L,fdisk_table_wrong_order(data->data));
	return 1;
}

static int lua_fdisk_table_next_partition(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_partition*part=NULL;
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	struct lua_fdisk_iter*iter=luaL_checkudata(L,2,LUA_FDISK_ITER);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	if(!iter||!iter->data)return luaL_argerror(L,2,"invalid iterator");
	int ret=fdisk_table_next_partition(data->data,iter->data,&part);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!part)lua_pushnil(L);
	else lua_fdisk_partition_to_lua(L,false,part);
	return 2;
}

static int lua_fdisk_table_get_partition(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_partition*part=NULL;
	size_t n=luaL_checkinteger(L,2);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	part=fdisk_table_get_partition(data->data,n);
	if(!part)lua_pushnil(L);
	else lua_fdisk_partition_to_lua(L,false,part);
	return 1;
}

static int lua_fdisk_table_get_partition_by_partno(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_partition*part=NULL;
	size_t partno=luaL_checkinteger(L,2);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	part=fdisk_table_get_partition_by_partno(data->data,partno);
	if(!part)lua_pushnil(L);
	else lua_fdisk_partition_to_lua(L,false,part);
	return 1;
}

static int lua_fdisk_table_get_partition_by_name(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_partition*part=NULL,*result=NULL;
	const char*name=luaL_checkstring(L,2),*cur=NULL;
	struct fdisk_iter*iter=fdisk_new_iter(FDISK_ITER_FORWARD);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	if(!iter)return luaL_error(L,"allocate iterator failed");
	lua_createtable(L,0,0);
	while(fdisk_table_next_partition(data->data,iter,&part)==0){
		if(!part||!(cur=fdisk_partition_get_name(part)))continue;
		if(!name[0]||!cur[0]||strcasecmp(name,cur)!=0)continue;
		result=part;
		break;
	}
	if(!result)lua_pushnil(L);
	else lua_fdisk_partition_to_lua(L,false,result);
	fdisk_free_iter(iter);
	return 1;
}

static int lua_fdisk_table_get_partitions(lua_State*L){
	LUA_ARG_MAX(1);
	int i=0;
	struct fdisk_partition*part=NULL;
	struct fdisk_iter*iter=fdisk_new_iter(FDISK_ITER_FORWARD);
	struct lua_fdisk_table*data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid table");
	if(!iter)return luaL_error(L,"allocate iterator failed");
	lua_createtable(L,0,0);
	while(fdisk_table_next_partition(data->data,iter,&part)==0){
		if(!part)continue;
		lua_fdisk_partition_to_lua(L,false,part);
		lua_rawseti(L,-2,i+1);
		i++,part=NULL;
	}
	fdisk_free_iter(iter);
	return 1;
}

static int lua_fdisk_table_gc(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_table*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_TABLE);
	if(!data||!data->data)return 0;
	fdisk_unref_table(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_table={
	.name=LUA_FDISK_TABLE,
	.reg=(luaL_Reg[]){
		{"reset",                   lua_fdisk_table_reset},
		{"get_nents",               lua_fdisk_table_get_nents},
		{"is_empty",                lua_fdisk_table_is_empty},
		{"add_partition",           lua_fdisk_table_add_partition},
		{"remove_partition",        lua_fdisk_table_remove_partition},
		{"wrong_order",             lua_fdisk_table_wrong_order},
		{"next_partition",          lua_fdisk_table_next_partition},
		{"get_partition",           lua_fdisk_table_get_partition},
		{"get_partition_by_partno", lua_fdisk_table_get_partition_by_partno},
		{"get_partition_by_name",   lua_fdisk_table_get_partition_by_name},
		{"get_partitions",          lua_fdisk_table_get_partitions},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_table_gc,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_iter_to_lua(
	lua_State*L,
	struct fdisk_iter*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_fdisk_iter*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_iter));
	luaL_getmetatable(L,LUA_FDISK_ITER);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_iter));
	e->data=data;
}

static int lua_fdisk_iter_reset(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_iter*data=luaL_checkudata(L,1,LUA_FDISK_ITER);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	int direction=luaL_optinteger(L,2,fdisk_iter_get_direction(data->data));
	fdisk_reset_iter(data->data,direction);
	return 0;
}

static int lua_fdisk_iter_get_direction(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_iter*data=luaL_checkudata(L,1,LUA_FDISK_ITER);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label item");
	lua_pushinteger(L,fdisk_iter_get_direction(data->data));
	return 0;
}

static int lua_fdisk_iter_gc(lua_State*L){
	struct lua_fdisk_iter*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_ITER);
	if(!data||!data->data)return 0;
	fdisk_free_iter(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_iter={
	.name=LUA_FDISK_ITER,
	.reg=(luaL_Reg[]){
		{"reset",           lua_fdisk_iter_reset},
		{"get_direction",   lua_fdisk_iter_get_direction},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_iter_gc,
};

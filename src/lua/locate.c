/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#ifdef ENABLE_UEFI
#include"uefi/lua_uefi.h"
#include"locate.h"

static void locate_to_lua(lua_State*L,locate_ret*ret){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"tag");
	lua_pushstring(L,ret->tag);
	lua_settable(L,-3);
	lua_pushliteral(L,"path");
	lua_pushstring(L,ret->path);
	lua_settable(L,-3);
	lua_pushliteral(L,"handle");
	uefi_handle_to_lua(L,ret->hand);
	lua_settable(L,-3);
	lua_pushliteral(L,"device_path");
	uefi_device_path_to_lua(L,ret->device);
	lua_settable(L,-3);
	lua_pushliteral(L,"part_info");
	uefi_partition_info_protocol_to_lua(L,ret->part);
	lua_settable(L,-3);
	lua_pushliteral(L,"type");
	switch(ret->type){
		case LOCATE_FILE:
			lua_pushstring(L,"file");
			lua_settable(L,-3);
			lua_pushliteral(L,"file_system");
			uefi_simple_file_system_protocol_to_lua(L,ret->fs);
			lua_settable(L,-3);
			lua_pushliteral(L,"root");
			uefi_file_protocol_to_lua(L,ret->root);
			lua_settable(L,-3);
			lua_pushliteral(L,"file");
			uefi_file_protocol_to_lua(L,ret->file);
			lua_settable(L,-3);
		break;
		case LOCATE_BLOCK:
			lua_pushstring(L,"block");
			lua_settable(L,-3);
			lua_pushliteral(L,"block");
			uefi_block_io_protocol_to_lua(L,ret->block);
			lua_settable(L,-3);
		break;
		default:lua_pushnil(L);
	}
}

static int lua_locate_locate(lua_State*L){
	locate_ret*loc=AllocateZeroPool(sizeof(locate_ret));
	if(!loc)return luaL_error(L,"alloc locate failed");
	const char*path=luaL_checkstring(L,1);
	bool ret=boot_locate(loc,path);
	lua_pushboolean(L,ret);
	if(ret)locate_to_lua(L,loc);
	else lua_pushnil(L);
	FreePool(loc);
	return 2;
}

static int lua_locate_locate_quiet(lua_State*L){
	locate_ret*loc=AllocateZeroPool(sizeof(locate_ret));
	if(!loc)return luaL_error(L,"alloc locate failed");
	const char*path=luaL_checkstring(L,1);
	bool ret=boot_locate_quiet(loc,path);
	lua_pushboolean(L,ret);
	if(ret)locate_to_lua(L,loc);
	else lua_pushnil(L);
	FreePool(loc);
	return 2;
}

static int lua_locate_locate_create(lua_State*L){
	locate_ret*loc=AllocateZeroPool(sizeof(locate_ret));
	if(!loc)return luaL_error(L,"alloc locate failed");
	const char*path=luaL_checkstring(L,1);
	bool ret=boot_locate_create_file(loc,path);
	lua_pushboolean(L,ret);
	if(ret)locate_to_lua(L,loc);
	else lua_pushnil(L);
	FreePool(loc);
	return 2;
}

static int lua_locate_find_name(lua_State*L){
	char buf[256];
	ZeroMem(buf,sizeof(buf));
	char*r=locate_find_name(buf,sizeof(buf));
	if(!r)lua_pushnil(L);
	else lua_pushstring(L,r);
	return 1;
}

static int lua_locate_add_device_path(lua_State*L){
	GET_DEVICE_PATH(L,1,dp);
	bool save=true;
	if(!lua_isnoneornil(L,2)){
		luaL_checktype(L,2,LUA_TBOOLEAN);
		save=lua_toboolean(L,2);
	}
	char buf[256];
	ZeroMem(buf,sizeof(buf));
	locate_find_name(buf,sizeof(buf));
	const char*r=luaL_optstring(L,3,buf);
	bool ret=locate_add_by_device_path((char*)r,save,dp->dp);
	lua_pushboolean(L,ret);
	if(!ret)lua_pushnil(L);
	else lua_pushstring(L,r);
	return 2;
}

static luaL_Reg locate_lib[]={
	{"locate",          lua_locate_locate},
	{"locate_quiet",    lua_locate_locate_quiet},
	{"locate_create",   lua_locate_locate_create},
	{"find_name",       lua_locate_find_name},
	{"add_device_path", lua_locate_add_device_path},
	{NULL, NULL}
};

LUAMOD_API int luaopen_locate(lua_State*L){
	luaL_newlib(L,locate_lib);
	return 1;
}
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs.h"

void lua_fs_file_info_to_lua(lua_State*L,fs_file_info*info){
	if(!fs_file_info_check(info)){
		lua_pushnil(L);
		return;
	}
	struct lua_fs_info*e;
	e=lua_newuserdata(L,sizeof(struct lua_fs_info));
	luaL_getmetatable(L,LUA_FS_INFO);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fs_info));
	memcpy(&e->info,info,sizeof(fs_file_info));
}

static int lua_fs_info_open(lua_State*L){
	int r=0;
	fsh*nf=NULL;
	fs_file_flag flag=0;
	GET_INFO(L,1,info);
	if(!lua_fs_get_flag(L,2,false,&flag))return 0;
	r=fs_open_with(&nf,&info->info,flag);
	lua_pushboolean(L,r==0);
	lua_pushinteger(L,r);
	lua_fsh_to_lua(L,nf);
	return 3;
}

static int lua_fs_info_has_type(lua_State*L){
	fs_type type=0;
	GET_INFO(L,1,info);
	if(!lua_fs_get_type(L,2,false,&type))return 0;
	lua_pushboolean(L,fs_has_type(info->info.type,type));
	return 1;
}

static int lua_fs_info_has_feature(lua_State*L){
	fs_feature feature=0;
	GET_INFO(L,1,info);
	if(!lua_fs_get_feature(L,2,false,&feature))return 0;
	lua_pushboolean(L,fs_has_feature(info->info.features,feature));
	return 1;
}

static int lua_fs_info_types(lua_State*L){
	GET_INFO(L,1,info);
	lua_pushinteger(L,info->info.type);
	return 1;
}

static int lua_fs_info_features(lua_State*L){
	GET_INFO(L,1,info);
	lua_pushinteger(L,info->info.features);
	return 1;
}

static int lua_fs_info_parent(lua_State*L){
	GET_INFO(L,1,info);
	lua_fsh_to_lua(L,info->info.parent);
	return 1;
}

#define DECL_STR_FIELD(name)\
	static int lua_fs_info_##name(lua_State*L){\
		GET_INFO(L,1,info);\
		if(!lua_isnoneornil(L,2)){\
			const char*s=luaL_checkstring(L,2);\
			memset(info->info.name,0,sizeof(info->info.name));\
			strncpy(info->info.name,s,sizeof(info->info.name)-1);\
		}\
		lua_pushstring(L,info->info.name);\
		return 1;\
	}
#define DECL_INT_FIELD(name)\
	static int lua_fs_info_##name(lua_State*L){\
		GET_INFO(L,1,info);\
		if(!lua_isnoneornil(L,2))\
			info->info.name=luaL_checkinteger(L,2);\
		lua_pushinteger(L,info->info.name);\
		return 1;\
	}
DECL_STR_FIELD(name)
DECL_STR_FIELD(target)
DECL_INT_FIELD(size)
DECL_INT_FIELD(ctime)
DECL_INT_FIELD(mtime)
DECL_INT_FIELD(atime)
DECL_INT_FIELD(type)
DECL_INT_FIELD(mode)
DECL_INT_FIELD(owner)
DECL_INT_FIELD(group)
DECL_INT_FIELD(device)

struct lua_fs_meta_table lua_fs_info={
	.name=LUA_FS_INFO,
	.reg=(luaL_Reg[]){
		{"open",        lua_fs_info_open},
		{"parent",      lua_fs_info_parent},
		{"name",        lua_fs_info_name},
		{"size",        lua_fs_info_size},
		{"ctime",       lua_fs_info_ctime},
		{"mtime",       lua_fs_info_mtime},
		{"atime",       lua_fs_info_atime},
		{"type",        lua_fs_info_type},
		{"mode",        lua_fs_info_mode},
		{"owner",       lua_fs_info_owner},
		{"group",       lua_fs_info_group},
		{"device",      lua_fs_info_device},
		{"target",      lua_fs_info_target},
		{"types",       lua_fs_info_types},
		{"features",    lua_fs_info_features},
		{"has_type",    lua_fs_info_has_type},
		{"has_feature", lua_fs_info_has_feature},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=NULL,
};

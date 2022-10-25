/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LUA_FS_H
#define _LUA_FS_H
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include"xlua.h"
#include"filesystem.h"

#define OPT_HANDLER(L,n,var) OPT_UDATA(L,n,var,lua_fsh,LUA_FSH)
#define OPT_VOL(L,n,var) OPT_UDATA(L,n,var,lua_fs_vol,LUA_FS_VOL)
#define OPT_INFO(L,n,var) OPT_UDATA(L,n,var,lua_fs_info,LUA_FS_INFO)
#define GET_HANDLER(L,n,var) OPT_HANDLER(L,n,var);CHECK_NULL(L,n,var)
#define GET_INFO(L,n,var) OPT_INFO(L,n,var);CHECK_NULL(L,n,var)
#define GET_VOL(L,n,var) OPT_VOL(L,n,var);CHECK_NULL(L,n,var)

struct lua_fs_meta_table{
	char*name;
	const luaL_Reg*reg;
	const lua_CFunction tostring;
	const lua_CFunction gc;
};
extern struct lua_fs_meta_table lua_fsh;
extern struct lua_fs_meta_table lua_fs_info;
extern bool lua_fs_get_type(lua_State*L,int idx,bool nil,fs_type*type);
extern bool lua_fs_get_flag(lua_State*L,int idx,bool nil,fs_file_flag*flag);
extern bool lua_fs_get_feature(lua_State*L,int idx,bool nil,fs_feature*type);
#endif

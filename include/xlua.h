/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _XLUA_H
#define _XLUA_H
#include"lua.h"
#include"list.h"
#include"lualib.h"
#include"lauxlib.h"
#include"filesystem.h"

#define LUA_URL "URL"
#define LUA_FSH "File System Handler"
#define LUA_FS_INFO "File System File Info"
#define LUA_FS_VOL "File System Volume"
#define LUA_DATA "RAW Data"
#define CHECK_NULL(L,n,var) luaL_argcheck(L,var!=NULL,n,"not null")
#define OPT_UDATA(L,n,var,type,name)\
	struct type*(var)=NULL;\
	if(!lua_isnoneornil(L,n)){\
		var=luaL_checkudata(L,n,name);\
	}
struct lua_url{url*u;};
struct lua_fsh{fsh*f;};
struct lua_fs_info{fs_file_info info;};
struct lua_fs_vol{fsvol_info*info;};
struct lua_data{
	bool allocated;
	#ifdef ENABLE_UEFI
	bool uefi;
	#endif
	void*data;
	size_t size;
	struct lua_data*parent;
	list*refs;
};
extern const luaL_Reg lua_core_libs[];
extern const luaL_Reg simple_init_lua_libs[];
extern const luaL_Reg simple_init_lua_regs[];
LUAMOD_API int luaopen_fs(lua_State*L);
LUAMOD_API int luaopen_url(lua_State*L);
LUAMOD_API int luaopen_data(lua_State*L);
LUAMOD_API int luaopen_conf(lua_State*L);
LUAMOD_API int luaopen_init(lua_State*L);
LUAMOD_API int luaopen_uefi(lua_State*L);
LUAMOD_API int luaopen_lvgl(lua_State*L);
LUAMOD_API int luaopen_locate(lua_State*L);
LUAMOD_API int luaopen_logger(lua_State*L);
LUAMOD_API int luaopen_sysbar(lua_State*L);
LUAMOD_API int luaopen_nanosvg(lua_State*L);
LUAMOD_API int luaopen_abootimg(lua_State*L);
LUAMOD_API int luaopen_recovery(lua_State*L);
LUAMOD_API int luaopen_activity(lua_State*L);
LUAMOD_API int luaopen_xml_render(lua_State*L);
LUAMOD_API int lua_feature(lua_State*L);
extern lua_State*xlua_init();
extern lua_State*xlua_math();
extern int xlua_eval_string(lua_State*L,const char*expr);
extern int xlua_return_string(lua_State*L,const char*expr);
extern void xlua_show_error(lua_State*L,char*tag);
extern int xlua_loadfile(lua_State*L,fsh*f,const char*name);
extern int xlua_run(lua_State*L,char*tag,const char*name);
extern int xlua_run_by(lua_State*L,char*tag,fsh*f,const char*name);
extern void xlua_run_confd(lua_State*L,char*tag,const char*key,...);
extern int xlua_create_metatable(
	lua_State*L,
	const char*name,
	const luaL_Reg*funcs,
	lua_CFunction tostring,
	lua_CFunction gc
);
extern void lua_url_to_lua(lua_State*L,url*u);
extern void lua_url_dup_to_lua(lua_State*L,url*u);
extern void lua_fsh_to_lua(lua_State*L,fsh*f);
extern void lua_fs_file_info_to_lua(lua_State*L,fs_file_info*info);
extern void lua_arg_get_data(lua_State*L,int idx,bool nil,void**data,size_t*size);
extern void lua_data_to_lua(lua_State*L,bool allocated,void*data,size_t size);
extern void lua_data_dup_to_lua(lua_State*L,void*data,size_t size);
extern void xlua_dump_stack(lua_State*L);
#endif

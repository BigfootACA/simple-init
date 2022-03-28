/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<string.h>
#include"xlua.h"
#include"logger.h"
#define TAG "lua"

extern enum log_level logger_level;
static char log_tag[256]="lua";

/**
 * set or get logger tag
 *
 * @param string new tag (optional)
 * @return string current tag
 */
static int log_lib_tag(lua_State*L){
	const char*new_tag=luaL_optstring(L,1,NULL);
	if(new_tag&&new_tag[0]){
		memset(log_tag,0,sizeof(log_tag));
		strncpy(log_tag,new_tag,sizeof(log_tag)-1);
	}
	lua_pushstring(L,log_tag);
	return 1;
}

/**
 * set logger level
 *
 * @param string log level name
 * @return integer always 1
 */
static int log_lib_level(lua_State*L){
	const char*new_level=luaL_checkstring(L,1);
	logger_set_level(logger_parse_level(new_level));
	lua_pushinteger(L,1);
	return 1;
}

/**
 * set print to console enabled
 *
 * @note only take effect under UEFI
 * @param bool print to console enabled
 * @return integer always 1
 */
static int log_lib_console(lua_State*L){
	#ifdef ENABLE_UEFI
	int console=luaL_optinteger(L,1,-1);
	if(console>=0)logger_set_console(console!=0);
	#endif
	lua_pushinteger(L,1);
	return 1;
}

/**
 * print log to loggerd
 *
 * @param string content or format
 * @param ... string format (optional)
 * @return integer log string length or status
 */
static int log_call_log(lua_State*L,enum log_level level){
	lua_getglobal(L,"string");
	lua_getfield(L,-1,"format");
	lua_insert(L,1);
	lua_pop(L,1);
	lua_pushvalue(L,lua_upvalueindex(1));
	lua_insert(L,1);
	lua_call(L,lua_gettop(L)-2,LUA_MULTRET);
	const char*cont=lua_tostring(L,-1);
	int ret=logger_print(level,log_tag,(char*)cont);
	lua_pushinteger(L,ret);
	return 1;
}

#define DECL_LOG_LEVEL(level,name) static inline int log_lib_##name(lua_State*L){return log_call_log(L,level);}

DECL_LOG_LEVEL(LEVEL_VERBOSE,verbose)
DECL_LOG_LEVEL(LEVEL_DEBUG,debug)
DECL_LOG_LEVEL(LEVEL_INFO,info)
DECL_LOG_LEVEL(LEVEL_NOTICE,notice)
DECL_LOG_LEVEL(LEVEL_WARNING,warning)
DECL_LOG_LEVEL(LEVEL_ERROR,error)
DECL_LOG_LEVEL(LEVEL_CRIT,critical)
DECL_LOG_LEVEL(LEVEL_ALERT,alert)
DECL_LOG_LEVEL(LEVEL_EMERG,emergency)

static const luaL_Reg log_lib[]={
	{"tag",         log_lib_tag},
	{"get_tag",     log_lib_tag},
	{"set_tag",     log_lib_tag},
	{"set_level",   log_lib_level},
	{"set_console", log_lib_console},
	{"verb",        log_lib_verbose},
	{"verbose",     log_lib_verbose},
	{"dbg",         log_lib_debug},
	{"debug",       log_lib_debug},
	{"info",        log_lib_info},
	{"notice",      log_lib_notice},
	{"warn",        log_lib_warning},
	{"warning",     log_lib_warning},
	{"err",         log_lib_error},
	{"error",       log_lib_error},
	{"crit",        log_lib_critical},
	{"critical",    log_lib_critical},
	{"alert",       log_lib_alert},
	{"emerg",       log_lib_emergency},
	{"emergency",   log_lib_emergency},
	{NULL, NULL}
};

LUAMOD_API int luaopen_logger(lua_State*L){
	luaL_newlib(L,log_lib);
	return 1;
}
#endif

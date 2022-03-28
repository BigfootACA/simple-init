/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include"xlua.h"
#include"confd.h"

#define GET_KEY(n) \
        errno=0;\
	const char*key=luaL_checkstring(L,n);\
	if(!key||!*key)return luaL_error(L,"invalid conf key")

/**
 * is config item exists
 *
 * @param string config path
 * @return boolean config item exists
 */
static int conf_exists(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,(int)confd_get_type(key)!=-1);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * is config item a key
 *
 * @param string config path
 * @return boolean config item is a key
 */
static int conf_is_key(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_get_type(key)==TYPE_KEY);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * is config item a string
 *
 * @param string config path
 * @return boolean config item is a string
 */
static int conf_is_string(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_get_type(key)==TYPE_STRING);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * is config item a integer
 *
 * @param string config path
 * @return boolean config item is a integer
 */
static int conf_is_integer(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_get_type(key)==TYPE_INTEGER);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * is config item a boolean
 *
 * @param string config path
 * @return boolean config item is a boolean
 */
static int conf_is_boolean(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_get_type(key)==TYPE_BOOLEAN);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item type
 *
 * @param string config path
 * @return string config item type
 */
static int conf_get_type(lua_State*L){
	GET_KEY(1);
	const char*type=NULL;
	switch(confd_get_type(key)){
		case TYPE_KEY:type="key";break;
		case TYPE_STRING:type="string";break;
		case TYPE_INTEGER:type="integer";break;
		case TYPE_BOOLEAN:type="boolean";break;
	}
	if(!type)lua_pushnil(L);
	else lua_pushstring(L,type);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item as string
 *
 * @param string config path
 * @param string default value (optional)
 * @return string config item value
 */
static int conf_get_string(lua_State*L){
	GET_KEY(1);
	const char*def=luaL_optstring(L,2,NULL);
	errno=0;
	char*ret=confd_get_string(key,(char*)def);
	if(ret){
		lua_pushstring(L,ret);
		free(ret);
	}else lua_pushnil(L);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item as integer
 *
 * @param string config path
 * @param integer default value (optional)
 * @return integer config item value
 */
static int conf_get_integer(lua_State*L){
	GET_KEY(1);
	lua_Integer def=0;
	if(lua_isnoneornil(L,2)){
		luaL_checktype(L,2,LUA_TNUMBER);
		def=lua_tointeger(L,2);
	}
	lua_pushinteger(L,(lua_Integer)confd_get_integer(key,(int64_t)def));
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item as boolean
 *
 * @param string config path
 * @param boolean default value (optional)
 * @return boolean config item value
 */
static int conf_get_boolean(lua_State*L){
	GET_KEY(1);
	int def=0;
	if(lua_isnoneornil(L,2)){
		luaL_checktype(L,2,LUA_TBOOLEAN);
		def=lua_toboolean(L,2);
	}
	lua_pushboolean(L,confd_get_boolean(key,def!=0));
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item as string
 *
 * @param string config path
 * @param string value to set
 * @return integer status
 */
static int conf_set_string(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TSTRING);
	lua_pushboolean(L,confd_set_string(key,(char*)lua_tostring(L,2))==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item as integer
 *
 * @param string config path
 * @param integer value to set
 * @return integer status
 */
static int conf_set_integer(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_pushboolean(L,confd_set_integer(key,(int64_t)lua_tointeger(L,2))==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item as boolean
 *
 * @param string config path
 * @param boolean value to set
 * @return integer status
 */
static int conf_set_boolean(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	lua_pushboolean(L,confd_set_boolean(key,lua_toboolean(L,2)!=0)==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * list config items under a config key
 *
 * @param string config path
 * @return table result list
 */
static int conf_list(lua_State*L){
	GET_KEY(1);
	char**bs=confd_ls(key);
	lua_createtable(L,0,0);
	if(bs){
		for(size_t i=0;bs[i];i++){
			lua_pushstring(L,bs[i]);
			lua_rawseti(L,-2,i+1);
		}
		if(bs[0])free(bs[0]);
		if(bs)free(bs);
	}else lua_pushnil(L);
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * delete a config item and all children
 *
 * @param string config path
 * @return integer status
 */
static int conf_delete(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_delete(key)==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * count config items under a config key
 *
 * @param string config path
 * @return integer status
 */
static int conf_count(lua_State*L){
	GET_KEY(1);
	lua_pushinteger(L,confd_count(key));
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * create a config key
 *
 * @param string config path
 * @return integer status
 */
static int conf_add_key(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_add_key(key)==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item
 *
 * @param string config path
 * @param string/integer/boolean default value (optional)
 * @return string/integer/boolean config item value
 */
static int conf_get(lua_State*L){
	GET_KEY(1);
	switch(confd_get_type(key)){
		case TYPE_STRING:{
			char*ret=confd_get_string(key,(char*)lua_tostring(L,2));
			if(ret){
				lua_pushstring(L,ret);
				free(ret);
			}else lua_pushnil(L);
		}break;
		case TYPE_INTEGER:lua_pushinteger(L,confd_get_integer(key,(int64_t)lua_tointeger(L,2)));break;
		case TYPE_BOOLEAN:lua_pushboolean(L,confd_get_boolean(key,lua_toboolean(L,2)!=0));break;
		default:lua_pushnil(L);
	}
	if(errno==ENOENT)errno=0;
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item
 *
 * @param string config path
 * @param string/integer/boolean value to set
 * @return integer status
 */
 static int conf_set(lua_State*L){
	GET_KEY(1);
	int ret=-1;
	switch(lua_type(L,2)){
		case LUA_TSTRING:ret=confd_set_string(key,(char*)lua_tostring(L,2));break;
		case LUA_TNUMBER:ret=confd_set_integer(key,(int64_t)lua_tointeger(L,2));break;
		case LUA_TBOOLEAN:ret=confd_set_boolean(key,lua_toboolean(L,2)!=0);break;
		default:return luaL_error(L,"invalid data type");
	}
	lua_pushboolean(L,ret==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * call confd dump config store
 *
 * @return integer status
 */
static int conf_dump(lua_State*L){
	lua_pushboolean(L,confd_dump(LEVEL_DEBUG)==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item permission owner
 *
 * @param string config path
 * @param integer new owner uid
 * @return integer status
 */
static int conf_set_own(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_pushboolean(L,confd_set_own(key,(uid_t)lua_tointeger(L,2))==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item permission group
 *
 * @param string config path
 * @param integer new group gid
 * @return integer status
 */
static int conf_set_grp(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_pushboolean(L,confd_set_grp(key,(gid_t)lua_tointeger(L,2))==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item permission mode
 *
 * @param string config path
 * @param integer new mode number
 * @return integer status
 */
static int conf_set_mod(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TNUMBER);
	lua_pushboolean(L,confd_set_mod(key,(mode_t)lua_tointeger(L,2))==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * set config item should save
 *
 * @param string config path
 * @param boolean should save
 * @return integer status
 */
static int conf_set_save(lua_State*L){
	GET_KEY(1);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	lua_pushboolean(L,confd_set_save(key,lua_toboolean(L,2)!=0)==0);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item permission owner
 *
 * @param string config path
 * @return integer owner uid
 */
static int conf_get_own(lua_State*L){
	GET_KEY(1);
	uid_t uid=0;
	confd_get_own(key,&uid);
	lua_pushinteger(L,(lua_Integer)uid);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item permission group
 *
 * @param string config path
 * @return integer group gid
 */
static int conf_get_grp(lua_State*L){
	GET_KEY(1);
	gid_t gid=0;
	confd_get_grp(key,&gid);
	lua_pushinteger(L,(lua_Integer)gid);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item permission mode
 *
 * @param string config path
 * @return integer mode number
 */
static int conf_get_mod(lua_State*L){
	GET_KEY(1);
	mode_t mod=0;
	confd_get_own(key,&mod);
	lua_pushinteger(L,(lua_Integer)mod);
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

/**
 * get config item should save
 *
 * @param string config path
 * @return boolean should save
 */
static int conf_get_save(lua_State*L){
	GET_KEY(1);
	lua_pushboolean(L,confd_get_save(key));
	return errno!=0?luaL_error(L,"operation failed (%s)",strerror(errno)):1;
}

static const luaL_Reg conf_lib[]={
	{"count",        conf_count},
	{"exists",       conf_exists},
	{"is_key",       conf_is_key},
	{"is_str",       conf_is_string},
	{"is_string",    conf_is_string},
	{"is_int",       conf_is_integer},
	{"is_integer",   conf_is_integer},
	{"is_bool",      conf_is_boolean},
	{"is_boolean",   conf_is_boolean},
	{"type",         conf_get_type},
	{"get",          conf_get},
	{"get_own",      conf_get_own},
	{"get_owner",    conf_get_own},
	{"get_grp",      conf_get_grp},
	{"get_group",    conf_get_grp},
	{"get_mod",      conf_get_mod},
	{"get_mode",     conf_get_mod},
	{"get_type",     conf_get_type},
	{"get_save",     conf_get_save},
	{"get_str",      conf_get_string},
	{"get_string",   conf_get_string},
	{"get_int",      conf_get_integer},
	{"get_integer",  conf_get_integer},
	{"get_bool",     conf_get_boolean},
	{"get_boolean",  conf_get_boolean},
	{"set",          conf_set},
	{"chown",        conf_set_own},
	{"chowner",      conf_set_own},
	{"set_own",      conf_set_own},
	{"set_owner",    conf_set_own},
	{"chgrp",        conf_set_grp},
	{"chgroup",      conf_set_grp},
	{"set_grp",      conf_set_grp},
	{"set_group",    conf_set_grp},
	{"chmod",        conf_set_mod},
	{"chmode",       conf_set_mod},
	{"set_mod",      conf_set_mod},
	{"set_mode",     conf_set_mod},
	{"set_save",     conf_set_save},
	{"set_str",      conf_set_string},
	{"add_str",      conf_set_string},
	{"set_string",   conf_set_string},
	{"add_string",   conf_set_string},
	{"set_int",      conf_set_integer},
	{"add_int",      conf_set_integer},
	{"set_integer",  conf_set_integer},
	{"add_integer",  conf_set_integer},
	{"set_bool",     conf_set_boolean},
	{"add_bool",     conf_set_boolean},
	{"set_boolean",  conf_set_boolean},
	{"add_boolean",  conf_set_boolean},
	{"add_key",      conf_add_key},
	{"new_key",      conf_add_key},
	{"add",          conf_set},
	{"del",          conf_delete},
	{"delete",       conf_delete},
	{"rm",           conf_delete},
	{"remove",       conf_delete},
	{"purge",        conf_delete},
	{"dump",         conf_dump},
	{"list",         conf_list},
	{"ls",           conf_list},
	{NULL, NULL}
};

LUAMOD_API int luaopen_conf(lua_State*L){
	luaL_newlib(L,conf_lib);
	return 1;
}
#endif

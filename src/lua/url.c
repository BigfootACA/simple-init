/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<string.h>
#include"xlua.h"
#include"url.h"

#define OPT_URL(L,n,var) OPT_UDATA(L,n,var,lua_url,LUA_URL)
#define GET_URL(L,n,var) OPT_URL(L,n,var);CHECK_NULL(L,n,var)

void lua_url_to_lua(lua_State*L,url*u){
	if(!u){
		lua_pushnil(L);
		return;
	}
	struct lua_url*e;
	e=lua_newuserdata(L,sizeof(struct lua_url));
	luaL_getmetatable(L,LUA_URL);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_url));
	e->u=u;
}

void lua_url_dup_to_lua(lua_State*L,url*u){
	lua_url_to_lua(L,url_dup(u));
}

static int lua_url_tostring(lua_State*L){
	GET_URL(L,1,u);
	char*o;
	if(u->u&&(o=url_generate_alloc(u->u))){
		lua_pushstring(L,o);
		free(o);
	}else lua_pushfstring(L,LUA_URL" (%p)",u);
	return 1;
}

static int lua_url_string(lua_State*L){
	GET_URL(L,1,u);
	char*o;
	if(u->u&&(o=url_generate_alloc(u->u))){
		lua_pushstring(L,o);
		free(o);
	}else lua_pushnil(L);
	return 1;
}

static int lua_url_clean(lua_State*L){
	GET_URL(L,1,u);
	url_clean(u->u);
	u->u=NULL;
	return 0;
}

static int lua_url_free(lua_State*L){
	GET_URL(L,1,u);
	url_free(u->u);
	u->u=NULL;
	return 0;
}

static int lua_url_dup(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	lua_url_dup_to_lua(L,u->u);
	return 1;
}

static int lua_url_copy(lua_State*L){
	GET_URL(L,1,u1);
	GET_URL(L,2,u2);
	if(!u1->u)return luaL_argerror(L,1,"invalid url");
	if(!u2->u)return luaL_argerror(L,1,"invalid url");
	lua_pushboolean(L,url_copy(u1->u,u2->u)!=NULL);
	return 1;
}

static int lua_url_equals(lua_State*L){
	GET_URL(L,1,u1);
	GET_URL(L,2,u2);
	if(!u1->u)return luaL_argerror(L,1,"invalid url");
	if(!u2->u)return luaL_argerror(L,1,"invalid url");
	lua_pushboolean(L,url_equals(u1->u,u2->u));
	return 1;
}

static int lua_url_parse(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	const char*c=luaL_checkstring(L,1);
	lua_pushboolean(L,url_parse(u->u,c,0));
	return 1;
}

static int lua_url_relative(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	url*n=url_new();
	const char*c=luaL_checkstring(L,1);
	url*r=url_parse_relative_path(u->u,n,c,0);
	if(!r)url_free(r);
	lua_url_to_lua(L,r);
	return 1;
}

static int lua_url_go_back(lua_State*L){
	GET_URL(L,1,u);
	bool clean=true;
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	if(!lua_isnoneornil(L,2)){
		luaL_checktype(L,2,LUA_TBOOLEAN);
		clean=lua_toboolean(L,2);
	}
	lua_pushboolean(L,url_go_back(u->u,clean));
	return 1;
}

static int lua_url_is_on_top(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	lua_pushboolean(L,url_is_on_top(u->u));
	return 1;
}

#define DECL_STR_FIELD(name)\
	static int lua_url_get_##name(lua_State*L){\
		GET_URL(L,1,u);\
		if(!u->u)return luaL_argerror(L,1,"invalid url");\
		if(!u->u->name)lua_pushnil(L);\
		else lua_pushstring(L,u->u->name);\
		return 1;\
	}\
	static int lua_url_set_##name(lua_State*L){\
		GET_URL(L,1,u);\
		if(!u->u)return luaL_argerror(L,1,"invalid url");\
		url_set_##name(u->u,luaL_optstring(L,2,NULL),0);\
		return 0;\
	}

DECL_STR_FIELD(scheme)
DECL_STR_FIELD(username)
DECL_STR_FIELD(password)
DECL_STR_FIELD(host)
DECL_STR_FIELD(path)
DECL_STR_FIELD(query)
DECL_STR_FIELD(fragment)

static int lua_url_get_port(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	lua_pushinteger(L,u->u->port);
	return 1;
}

static int lua_url_set_port(lua_State*L){
	GET_URL(L,1,u);
	if(!u->u)return luaL_argerror(L,1,"invalid url");
	u->u->port=luaL_checkinteger(L,2);
	return 1;
}

static struct luaL_Reg lua_url_lib[]={
	{"is_on_top",    lua_url_is_on_top},
	{"go_back",      lua_url_go_back},
	{"relative",     lua_url_relative},
	{"parse",        lua_url_parse},
	{"equals",       lua_url_equals},
	{"copy",         lua_url_copy},
	{"dup",          lua_url_dup},
	{"duplicate",    lua_url_dup},
	{"clear",        lua_url_clean},
	{"clean",        lua_url_clean},
	{"free",         lua_url_free},
	{"set_scheme",   lua_url_set_scheme},
	{"set_username", lua_url_set_username},
	{"set_password", lua_url_set_password},
	{"set_host",     lua_url_set_host},
	{"set_port",     lua_url_set_port},
	{"set_path",     lua_url_set_path},
	{"set_query",    lua_url_set_query},
	{"set_fragment", lua_url_set_fragment},
	{"get_scheme",   lua_url_get_scheme},
	{"get_username", lua_url_get_username},
	{"get_password", lua_url_get_password},
	{"get_host",     lua_url_get_host},
	{"get_port",     lua_url_get_port},
	{"get_path",     lua_url_get_path},
	{"get_query",    lua_url_get_query},
	{"get_fragment", lua_url_get_fragment},
	{"get_string",   lua_url_string},
	{"tostring",     lua_url_string},
	{NULL, NULL},
};

static int lua_url_new(lua_State*L){
	url*u=url_new();
	if(!u)return luaL_error(L,"create new url failed");
	const char*c=luaL_optstring(L,1,NULL);
	if(c&&!url_parse(u,c,0)){
		url_free(u);
		lua_pushnil(L);
		return 1;
	}
	lua_url_to_lua(L,u);
	return 1;
}

static int lua_url_encode(lua_State*L){
	char*r;
	const char*c=luaL_checkstring(L,1);
	if((r=url_encode_alloc(c,0))){
		lua_pushstring(L,r);
		free(r);
	}else lua_pushnil(L);
	return 1;
}

static int lua_url_decode(lua_State*L){
	char*r;
	const char*c=luaL_checkstring(L,1);
	if((r=url_decode_alloc(c,0))){
		lua_pushstring(L,r);
		free(r);
	}else lua_pushnil(L);
	return 1;
}

static struct luaL_Reg url_lib[]={
	{"new",    lua_url_new},
	{"parse",  lua_url_new},
	{"encode", lua_url_encode},
	{"decode", lua_url_decode},
	{NULL, NULL},
};

LUAMOD_API int luaopen_url(lua_State*L){
	xlua_create_metatable(
		L,LUA_URL,
		lua_url_lib,
		lua_url_tostring,
		lua_url_free
	);
	luaL_newlib(L,url_lib);
	return 1;
}

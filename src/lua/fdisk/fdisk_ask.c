/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_ask_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_ask*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_ask(data);
	struct lua_fdisk_ask*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_ask));
	luaL_getmetatable(L,LUA_FDISK_ASK);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_ask));
	e->data=data;
}

static int lua_fdisk_ask_get_query(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	const char*ret=fdisk_ask_get_query(data->data);
	if(!ret)lua_pushnil(L);
	else lua_pushstring(L,ret);
	return 1;
}

static int lua_fdisk_ask_get_type(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_get_type(data->data));
	return 1;
}

static int lua_fdisk_ask_number_get_range(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	const char*ret=fdisk_ask_number_get_range(data->data);
	if(!ret)lua_pushnil(L);
	else lua_pushstring(L,ret);
	return 1;
}

static int lua_fdisk_ask_number_get_default(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_default(data->data));
	return 1;
}

static int lua_fdisk_ask_number_get_low(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_low(data->data));
	return 1;
}

static int lua_fdisk_ask_number_get_high(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_high(data->data));
	return 1;
}

static int lua_fdisk_ask_number_get_result(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_result(data->data));
	return 1;
}

static int lua_fdisk_ask_number_set_result(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	fdisk_ask_number_set_result(data->data,(uint64_t)luaL_checkinteger(L,2));
	return 0;
}

static int lua_fdisk_ask_number_get_base(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_base(data->data));
	return 1;
}

static int lua_fdisk_ask_number_get_unit(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_get_unit(data->data));
	return 1;
}

static int lua_fdisk_ask_number_set_relative(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	fdisk_ask_number_set_relative(data->data,lua_toboolean(L,2)?1:0);
	return 0;
}

static int lua_fdisk_ask_number_is_wrap_relative(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushboolean(L,fdisk_ask_number_is_wrap_negative(data->data));
	return 1;
}

static int lua_fdisk_ask_number_inchars(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_number_inchars(data->data));
	return 1;
}

static int lua_fdisk_ask_string_get_result(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushstring(L,fdisk_ask_string_get_result(data->data));
	return 1;
}

static int lua_fdisk_ask_string_set_result(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	const char*result=luaL_checkstring(L,2);
	if(!result)return luaL_argerror(L,2,"invalid result");
	fdisk_ask_string_set_result(data->data,strdup(result));
	return 0;
}

static int lua_fdisk_ask_yesno_get_result(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_yesno_get_result(data->data));
	return 1;
}

static int lua_fdisk_ask_yesno_set_result(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	fdisk_ask_yesno_set_result(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_ask_menu_get_default(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_menu_get_default(data->data));
	return 1;
}

static int lua_fdisk_ask_menu_set_result(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	fdisk_ask_menu_set_result(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_ask_menu_get_result(lua_State*L){
	LUA_ARG_MAX(1);
	int result=-1;
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	fdisk_ask_menu_get_result(data->data,&result);
	lua_pushinteger(L,result);
	return 1;
}

static int lua_fdisk_ask_menu_get_item(lua_State*L){
	LUA_ARG_MAX(2);
	int key=-1;
	const char*name=NULL,*desc=NULL;
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	size_t idx=luaL_checkinteger(L,2);
	int ret=fdisk_ask_menu_get_item(data->data,idx,&key,&name,&desc);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,key);
	if(!name)lua_pushnil(L);
	else lua_pushstring(L,name);
	if(!desc)lua_pushnil(L);
	else lua_pushstring(L,desc);
	return 4;
}

static int lua_fdisk_ask_menu_get_nitems(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_menu_get_nitems(data->data));
	return 1;
}

static int lua_fdisk_ask_print_get_errno(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	lua_pushinteger(L,fdisk_ask_print_get_errno(data->data));
	return 1;
}

static int lua_fdisk_ask_print_get_mesg(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_ask*data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid ask");
	const char*ret=fdisk_ask_print_get_mesg(data->data);
	if(!ret)lua_pushnil(L);
	else lua_pushstring(L,ret);
	return 1;
}

static int lua_fdisk_ask_gc(lua_State*L){
	struct lua_fdisk_ask*data;
	data=luaL_checkudata(L,1,LUA_FDISK_ASK);
	if(!data||!data->data)return 0;
	fdisk_unref_ask(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_ask={
	.name=LUA_FDISK_ASK,
	.reg=(luaL_Reg[]){
		{"get_query",               lua_fdisk_ask_get_query},
		{"get_type",                lua_fdisk_ask_get_type},
		{"number_get_range",        lua_fdisk_ask_number_get_range},
		{"number_get_default",      lua_fdisk_ask_number_get_default},
		{"number_get_low",          lua_fdisk_ask_number_get_low},
		{"number_get_high",         lua_fdisk_ask_number_get_high},
		{"number_get_result",       lua_fdisk_ask_number_get_result},
		{"number_set_result",       lua_fdisk_ask_number_set_result},
		{"number_get_base",         lua_fdisk_ask_number_get_base},
		{"number_get_unit",         lua_fdisk_ask_number_get_unit},
		{"number_set_relative",     lua_fdisk_ask_number_set_relative},
		{"number_is_wrap_relative", lua_fdisk_ask_number_is_wrap_relative},
		{"number_inchars",          lua_fdisk_ask_number_inchars},
		{"string_get_result",       lua_fdisk_ask_string_get_result},
		{"string_set_result",       lua_fdisk_ask_string_set_result},
		{"yesno_get_result",        lua_fdisk_ask_yesno_get_result},
		{"yesno_set_result",        lua_fdisk_ask_yesno_set_result},
		{"menu_get_default",        lua_fdisk_ask_menu_get_default},
		{"menu_set_result",         lua_fdisk_ask_menu_set_result},
		{"menu_get_result",         lua_fdisk_ask_menu_get_result},
		{"menu_get_item",           lua_fdisk_ask_menu_get_item},
		{"menu_get_nitems",         lua_fdisk_ask_menu_get_nitems},
		{"print_get_errno",         lua_fdisk_ask_print_get_errno},
		{"print_get_mesg",          lua_fdisk_ask_print_get_mesg},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_ask_gc,
};

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_label_to_lua(
	lua_State*L,
	struct fdisk_context*parent,
	struct fdisk_label*data
){
	if(!parent||!data){
		lua_pushnil(L);
		return;
	}
	fdisk_ref_context(parent);
	struct lua_fdisk_label*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_label));
	luaL_getmetatable(L,LUA_FDISK_LABEL);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_label));
	e->parent=parent,e->data=data;
}

static int lua_fdisk_label_get_nparttypes(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushinteger(L,fdisk_label_get_nparttypes(data->data));
	return 1;
}

static int lua_fdisk_label_get_parttype(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushinteger(L,fdisk_label_get_nparttypes(data->data));
	return 1;
}

static int lua_fdisk_label_get_parttype_shortcut(lua_State*L){
	LUA_ARG_MAX(2);
	const char*typestr=NULL,*shortcut=NULL,*alias=NULL;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	size_t n=luaL_checkinteger(L,2);
	int r=fdisk_label_get_parttype_shortcut(
		data->data,n,&typestr,&shortcut,&alias
	);
	lua_pushinteger(L,r);
	lua_pushstring(L,typestr);
	lua_pushstring(L,shortcut);
	lua_pushstring(L,alias);
	return 4;
}

static int lua_fdisk_label_has_code_parttypes(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_label_has_code_parttypes(data->data));
	return 1;
}

static int lua_fdisk_label_has_parttypes_shortcuts(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_label_has_parttypes_shortcuts(data->data));
	return 1;
}

static int lua_fdisk_label_get_parttype_from_code(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_parttype*parttype;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	unsigned int code=luaL_checkinteger(L,2);
	parttype=fdisk_label_get_parttype_from_code(data->data,code);
	if(!parttype)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,false,parttype);
	return 1;
}

static int lua_fdisk_label_get_parttype_from_string(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_parttype*parttype;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	const char*string=luaL_checkstring(L,2);
	parttype=fdisk_label_get_parttype_from_string(data->data,string);
	if(!parttype)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,false,parttype);
	return 1;
}

static int lua_fdisk_label_parse_parttype(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_parttype*parttype;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	const char*str=luaL_checkstring(L,2);
	parttype=fdisk_label_parse_parttype(data->data,str);
	if(!parttype)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,true,parttype);
	return 1;
}

static int lua_fdisk_label_advparse_parttype(lua_State*L){
	LUA_ARG_MAX(3);
	struct fdisk_parttype*parttype;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	const char*str=luaL_checkstring(L,2);
	int flags=luaL_checkinteger(L,3);
	parttype=fdisk_label_advparse_parttype(data->data,str,flags);
	if(!parttype)lua_pushnil(L);
	else lua_fdisk_parttype_to_lua(L,true,parttype);
	return 1;
}

static int lua_fdisk_label_get_type(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushinteger(L,fdisk_label_get_type(data->data));
	return 1;
}

static int lua_fdisk_label_get_name(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushstring(L,fdisk_label_get_name(data->data));
	return 1;
}

static int lua_fdisk_label_require_geometry(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_label_require_geometry(data->data));
	return 1;
}

static int lua_fdisk_label_get_geomrange_cylinders(lua_State*L){
	LUA_ARG_MAX(1);
	fdisk_sector_t min=0,max=0;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	int ret=fdisk_label_get_geomrange_cylinders(data->data,&min,&max);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,min);
	lua_pushinteger(L,max);
	return 3;
}

static int lua_fdisk_label_get_geomrange_heads(lua_State*L){
	LUA_ARG_MAX(1);
	unsigned int min=0,max=0;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	int ret=fdisk_label_get_geomrange_heads(data->data,&min,&max);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,min);
	lua_pushinteger(L,max);
	return 3;
}

static int lua_fdisk_label_get_geomrange_sectors(lua_State*L){
	LUA_ARG_MAX(1);
	fdisk_sector_t min=0,max=0;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	int ret=fdisk_label_get_geomrange_sectors(data->data,&min,&max);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,min);
	lua_pushinteger(L,max);
	return 3;
}

static int lua_fdisk_label_get_fields_ids(lua_State*L){
	LUA_ARG_MAX(1);
	int*ids=NULL;
	size_t cnt=0;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data||!data->parent)return luaL_argerror(L,1,"invalid label");
	int ret=fdisk_label_get_fields_ids(data->data,data->parent,&ids,&cnt);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(ids){
		lua_createtable(L,0,0);
		for(size_t i=0;i<cnt;i++){
			lua_pushinteger(L,ids[i]);
			lua_rawseti(L,-2,i+1);
		}
	}else lua_pushnil(L);
	return 2;
}

static int lua_fdisk_label_get_fields_ids_all(lua_State*L){
	LUA_ARG_MAX(1);
	int*ids=NULL;
	size_t cnt=0;
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data||!data->parent)return luaL_argerror(L,1,"invalid label");
	int ret=fdisk_label_get_fields_ids_all(data->data,data->parent,&ids,&cnt);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(ids){
		lua_createtable(L,0,0);
		for(size_t i=0;i<cnt;i++){
			lua_pushinteger(L,ids[i]);
			lua_rawseti(L,-2,i+1);
		}
	}else lua_pushnil(L);
	return 2;
}

static int lua_fdisk_label_get_field(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_field*field;
	int id=luaL_checkinteger(L,2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data||!data->parent)return luaL_argerror(L,1,"invalid label");
	field=(struct fdisk_field*)fdisk_label_get_field(data->data,id);
	if(!field)lua_pushnil(L);
	else lua_fdisk_field_to_lua(L,data->parent,field);
	return 1;
}

static int lua_fdisk_label_get_field_by_name(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_field*field;
	const char*name=luaL_checkstring(L,2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data||!data->parent)return luaL_argerror(L,1,"invalid label");
	field=(struct fdisk_field*)fdisk_label_get_field_by_name(data->data,name);
	if(!field)lua_pushnil(L);
	else lua_fdisk_field_to_lua(L,data->parent,field);
	return 1;
}

static int lua_fdisk_label_set_changed(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	fdisk_label_set_changed(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_label_is_changed(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_label_is_changed(data->data));
	return 1;
}

static int lua_fdisk_label_set_disabled(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	fdisk_label_set_disabled(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_label_is_disabled(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_label_is_disabled(data->data));
	return 1;
}

static int lua_fdisk_dos_enable_compatible(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	fdisk_dos_enable_compatible(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_dos_is_compatible(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	lua_pushboolean(L,fdisk_dos_is_compatible(data->data));
	return 1;
}

static int lua_fdisk_gpt_disable_relocation(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	fdisk_gpt_disable_relocation(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_gpt_enable_minimize(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_label*data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid label");
	fdisk_gpt_enable_minimize(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_label_gc(lua_State*L){
	struct lua_fdisk_label*data=NULL;
	data=luaL_checkudata(L,1,LUA_FDISK_LABEL);
	if(!data||!data->parent)return 0;
	fdisk_unref_context(data->parent);
	data->data=NULL;
	data->parent=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_label={
	.name=LUA_FDISK_LABEL,
	.reg=(luaL_Reg[]){
		{"get_nparttypes",           lua_fdisk_label_get_nparttypes},
		{"get_parttype",             lua_fdisk_label_get_parttype},
		{"get_parttype_shortcut",    lua_fdisk_label_get_parttype_shortcut},
		{"has_code_parttypes",       lua_fdisk_label_has_code_parttypes},
		{"has_parttypes_shortcuts",  lua_fdisk_label_has_parttypes_shortcuts},
		{"get_parttype_from_code",   lua_fdisk_label_get_parttype_from_code},
		{"get_parttype_from_string", lua_fdisk_label_get_parttype_from_string},
		{"parse_parttype",           lua_fdisk_label_parse_parttype},
		{"advparse_parttype",        lua_fdisk_label_advparse_parttype},
		{"get_type",                 lua_fdisk_label_get_type},
		{"get_name",                 lua_fdisk_label_get_name},
		{"require_geometry",         lua_fdisk_label_require_geometry},
		{"get_geomrange_cylinders",  lua_fdisk_label_get_geomrange_cylinders},
		{"get_geomrange_heads",      lua_fdisk_label_get_geomrange_heads},
		{"get_geomrange_sectors",    lua_fdisk_label_get_geomrange_sectors},
		{"get_fields_ids",           lua_fdisk_label_get_fields_ids},
		{"get_fields_ids_all",       lua_fdisk_label_get_fields_ids_all},
		{"get_field",                lua_fdisk_label_get_field},
		{"get_field_by_name",        lua_fdisk_label_get_field_by_name},
		{"set_changed",              lua_fdisk_label_set_changed},
		{"is_changed",               lua_fdisk_label_is_changed},
		{"set_disabled",             lua_fdisk_label_set_disabled},
		{"is_disabled",              lua_fdisk_label_is_disabled},
		{"dos_enable_compatible",    lua_fdisk_dos_enable_compatible},
		{"dos_is_compatible",        lua_fdisk_dos_is_compatible},
		{"gpt_disable_relocation",   lua_fdisk_gpt_disable_relocation},
		{"gpt_enable_minimize",      lua_fdisk_gpt_enable_minimize},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_label_gc,
};

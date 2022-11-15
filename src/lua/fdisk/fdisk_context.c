/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

void lua_fdisk_context_to_lua(
	lua_State*L,
	bool allocated,
	struct fdisk_context*data
){
	if(!data){
		lua_pushnil(L);
		return;
	}
	if(!allocated)fdisk_ref_context(data);
	struct lua_fdisk_context*e;
	e=lua_newuserdata(L,sizeof(struct lua_fdisk_context));
	luaL_getmetatable(L,LUA_FDISK_CONTEXT);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_fdisk_context));
	e->data=data;
}

static int lua_fdisk_new_nested(lua_State*L){
	LUA_ARG_MAX(2);
	const char*name=luaL_optstring(L,2,NULL);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_fdisk_context_to_lua(L,true,fdisk_new_nested_context(data->data,name));
	return 1;
}

static int lua_fdisk_get_parent(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_fdisk_context_to_lua(L,false,fdisk_get_parent(data->data));
	return 1;
}

static int lua_fdisk_get_npartitions(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_npartitions(data->data));
	return 1;
}

static int lua_fdisk_get_label(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_label*label=NULL;
	const char*name=luaL_optstring(L,2,NULL);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	label=fdisk_get_label(data->data,name);
	if(!label)lua_pushnil(L);
	else lua_fdisk_label_to_lua(L,data->data,label);
	return 1;
}

static int lua_fdisk_next_label(lua_State*L){
	LUA_ARG_MAX(1);
	struct fdisk_label*label=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_next_label(data->data,&label);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!label)lua_pushnil(L);
	else lua_fdisk_label_to_lua(L,data->data,label);
	return 2;
}

static int lua_fdisk_get_nlabels(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_nlabels(data->data));
	return 1;
}

static int lua_fdisk_has_label(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_has_label(data->data));
	return 1;
}

static int lua_fdisk_is_labeltype(lua_State*L){
	LUA_ARG_MAX(2);
	int type=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_labeltype(data->data,type));
	return 1;
}

static int lua_fdisk_assign_device(lua_State*L){
	LUA_ARG_MAX(3);
	bool readonly=false;
	const char*fname=luaL_checkstring(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,3))readonly=lua_toboolean(L,3);
	int ret=fdisk_assign_device(data->data,fname,readonly);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_assign_device_by_fd(lua_State*L){
	LUA_ARG_MAX(4);
	bool readonly=false;
	int fd=luaL_checkinteger(L,2);
	const char*fname=luaL_checkstring(L,3);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,4))readonly=lua_toboolean(L,4);
	int ret=fdisk_assign_device_by_fd(data->data,fd,fname,readonly);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_deassign_device(lua_State*L){
	LUA_ARG_MAX(4);
	bool nosync=false;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,4))nosync=lua_toboolean(L,4);
	int ret=fdisk_deassign_device(data->data,nosync);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_reassign_device(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_reassign_device(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_is_readonly(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_readonly(data->data));
	return 1;
}

static int lua_fdisk_is_regfile(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_regfile(data->data));
	return 1;
}

static int lua_fdisk_device_is_used(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_device_is_used(data->data));
	return 1;
}

static int lua_fdisk_disable_dialogs(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_disable_dialogs(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_has_dialogs(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_has_dialogs(data->data));
	return 1;
}

static int lua_fdisk_enable_details(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_enable_details(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_is_details(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_details(data->data));
	return 1;
}

static int lua_fdisk_enable_listonly(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_enable_listonly(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_is_listonly(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_listonly(data->data));
	return 1;
}

static int lua_fdisk_enable_wipe(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_enable_wipe(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_has_wipe(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_has_wipe(data->data));
	return 1;
}

static int lua_fdisk_get_collision(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushstring(L,fdisk_get_collision(data->data));
	return 1;
}

static int lua_fdisk_is_ptcollision(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_ptcollision(data->data));
	return 1;
}

static int lua_fdisk_set_unit(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_set_unit(data->data,luaL_checkstring(L,2));
	return 0;
}

static int lua_fdisk_get_unit(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushstring(L,fdisk_get_unit(data->data,luaL_optinteger(L,2,0)));
	return 1;
}

static int lua_fdisk_use_cylinders(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_use_cylinders(data->data));
	return 1;
}

static int lua_fdisk_get_units_per_sector(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_units_per_sector(data->data));
	return 1;
}

static int lua_fdisk_get_optimal_iosize(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_optimal_iosize(data->data));
	return 1;
}

static int lua_fdisk_get_minimal_iosize(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_minimal_iosize(data->data));
	return 1;
}

static int lua_fdisk_get_physector_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_physector_size(data->data));
	return 1;
}

static int lua_fdisk_get_sector_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_sector_size(data->data));
	return 1;
}

static int lua_fdisk_get_alignment_offset(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_alignment_offset(data->data));
	return 1;
}

static int lua_fdisk_get_grain_size(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_grain_size(data->data));
	return 1;
}

static int lua_fdisk_get_first_lba(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_first_lba(data->data));
	return 1;
}

static int lua_fdisk_set_first_lba(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_set_first_lba(data->data,luaL_checkinteger(L,2));
	return 0;
}

static int lua_fdisk_get_last_lba(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_last_lba(data->data));
	return 1;
}

static int lua_fdisk_set_last_lba(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_set_last_lba(data->data,luaL_checkinteger(L,2));
	return 0;
}

static int lua_fdisk_get_nsectors(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_nsectors(data->data));
	return 1;
}

static int lua_fdisk_get_devname(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushstring(L,fdisk_get_devname(data->data));
	return 1;
}

static int lua_fdisk_get_devfd(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_devfd(data->data));
	return 1;
}

static int lua_fdisk_get_devno(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	dev_t dev=fdisk_get_devno(data->data);
	lua_pushinteger(L,major(dev));
	lua_pushinteger(L,minor(dev));
	return 2;
}

static int lua_fdisk_get_devmodel(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushstring(L,fdisk_get_devmodel(data->data));
	return 1;
}

static int lua_fdisk_get_geom_heads(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_geom_heads(data->data));
	return 1;
}

static int lua_fdisk_get_geom_sectors(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_geom_sectors(data->data));
	return 1;
}

static int lua_fdisk_get_geom_cylinders(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_geom_cylinders(data->data));
	return 1;
}

static int lua_fdisk_set_size_unit(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_set_size_unit(data->data,luaL_checkinteger(L,2));
	return 0;
}

static int lua_fdisk_get_size_unit(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_get_size_unit(data->data));
	return 1;
}

static int lua_fdisk_enable_bootbits_protection(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_enable_bootbits_protection(data->data,lua_toboolean(L,2));
	return 0;
}

static int lua_fdisk_has_protected_bootbits(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_has_protected_bootbits(data->data));
	return 1;
}

static int lua_fdisk_write_disklabel(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_write_disklabel(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_verify_disklabel(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_verify_disklabel(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_create_disklabel(lua_State*L){
	LUA_ARG_MAX(2);
	const char*name=luaL_optstring(L,2,NULL);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_create_disklabel(data->data,name);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_list_disklabel(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_list_disklabel(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_locate_disklabel(lua_State*L){
	LUA_ARG_MAX(2);
	size_t size=0;
	uint64_t off=0;
	const char*name=NULL;
	int n=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_locate_disklabel(data->data,n,&name,&off,&size);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!name)lua_pushnil(L);
	else lua_pushstring(L,name);
	lua_pushinteger(L,off);
	lua_pushinteger(L,size);
	return 4;
}

static int lua_fdisk_get_disklabel_item(lua_State*L){
	LUA_ARG_MAX(3);
	int id=luaL_checkinteger(L,2);
	struct fdisk_labelitem*label=NULL;
	struct lua_fdisk_labelitem*item=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,3))item=luaL_checkudata(L,3,LUA_FDISK_LABELITEM);
	if(item&&!item->data)return luaL_argerror(L,3,"invalid label item");
	if(item)label=item->data;
	if(!label)label=fdisk_new_labelitem();
	int ret=fdisk_get_disklabel_item(data->data,id,label);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!label)lua_pushnil(L);
	else if(item&&label==item->data)lua_pushvalue(L,3);
	else lua_fdisk_labelitem_to_lua(L,true,label);
	if(!item)fdisk_unref_labelitem(label);
	return 2;
}

static int lua_fdisk_get_disklabel_id(lua_State*L){
	LUA_ARG_MAX(1);
	char*id=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_get_disklabel_id(data->data,&id);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(id){
		lua_pushstring(L,id);
		free(id);
	}else lua_pushnil(L);
	return 2;
}

static int lua_fdisk_set_disklabel_id(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_set_disklabel_id(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_set_disklabel_id_from_string(lua_State*L){
	LUA_ARG_MAX(2);
	const char*str=luaL_checkstring(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_set_disklabel_id_from_string(data->data,str);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_get_partition(lua_State*L){
	LUA_ARG_MAX(3);
	struct fdisk_partition*part=NULL;
	size_t partno=luaL_checkinteger(L,2);
	struct lua_fdisk_partition*pd=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,3))pd=luaL_checkudata(L,3,LUA_FDISK_PARTITION);
	if(pd&&!pd->data)return luaL_argerror(L,3,"invalid partition");
	if(pd)part=pd->data;
	int ret=fdisk_get_partition(data->data,partno,&part);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!part)lua_pushnil(L);
	else if(pd&&part==pd->data)lua_pushvalue(L,3);
	else lua_fdisk_partition_to_lua(L,true,part);
	return 2;
}

static int lua_fdisk_set_partition(lua_State*L){
	LUA_ARG_MAX(3);
	size_t partno=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_partition*part=luaL_checkudata(L,3,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!part||!part->data)return luaL_argerror(L,3,"invalid partition");
	int ret=fdisk_set_partition(data->data,partno,part->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_add_partition(lua_State*L){
	LUA_ARG_MAX(2);
	size_t partno=0;
	struct fdisk_partition*part=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_partition*pd=NULL;
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,2))pd=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(pd&&!pd->data)return luaL_argerror(L,2,"invalid partition");
	if(pd)part=pd->data;
	int ret=fdisk_add_partition(data->data,part,&partno);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,partno);
	return 2;
}

static int lua_fdisk_delete_partition(lua_State*L){
	LUA_ARG_MAX(2);
	size_t partno=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_delete_partition(data->data,partno);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_delete_all_partitions(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_delete_all_partitions(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_wipe_partition(lua_State*L){
	LUA_ARG_MAX(3);
	bool enable=lua_toboolean(L,3);
	size_t partno=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_wipe_partition(data->data,partno,enable);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_set_partition_type(lua_State*L){
	LUA_ARG_MAX(3);
	size_t partnum=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_parttype*type=luaL_checkudata(L,2,LUA_FDISK_PARTTYPE);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!type||!type->data)return luaL_argerror(L,2,"invalid partition type");
	int ret=fdisk_set_partition_type(data->data,partnum,type->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_is_partition_used(lua_State*L){
	LUA_ARG_MAX(2);
	size_t n=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_is_partition_used(data->data,n));
	return 1;
}

static int lua_fdisk_toggle_partition_flag(lua_State*L){
	LUA_ARG_MAX(3);
	size_t partnum=luaL_checkinteger(L,2);
	unsigned long flag=luaL_checkinteger(L,3);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_toggle_partition_flag(data->data,partnum,flag);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_reorder_partitions(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_reorder_partitions(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_partition_has_wipe(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_partition*part=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!part||!part->data)return luaL_argerror(L,2,"invalid partition");
	lua_pushboolean(L,fdisk_partition_has_wipe(data->data,part->data));
	return 1;
}

static int lua_fdisk_get_partitions(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_table*table=NULL;
	struct lua_fdisk_table*td=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,2))td=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(td&&!td->data)return luaL_argerror(L,2,"invalid table");
	if(td)table=td->data;
	int ret=fdisk_get_partitions(data->data,&table);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!table)lua_pushnil(L);
	else if(td&&table==td->data)lua_pushvalue(L,2);
	else lua_fdisk_table_to_lua(L,true,table);
	return 2;
}

static int lua_fdisk_get_freespaces(lua_State*L){
	LUA_ARG_MAX(2);
	struct fdisk_table*table=NULL;
	struct lua_fdisk_table*td=NULL;
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!lua_isnoneornil(L,2))td=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(td&&!td->data)return luaL_argerror(L,2,"invalid table");
	if(td)table=td->data;
	int ret=fdisk_get_freespaces(data->data,&table);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	if(!table)lua_pushnil(L);
	else if(td&&table==td->data)lua_pushvalue(L,2);
	else lua_fdisk_table_to_lua(L,true,table);
	return 2;
}

static int lua_fdisk_apply_table(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_table*table=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!table||!table->data)return luaL_argerror(L,2,"invalid table");
	int ret=fdisk_apply_table(data->data,table->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_align_lba(lua_State*L){
	LUA_ARG_MAX(2);
	int direction=luaL_checkinteger(L,3);
	fdisk_sector_t lba=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_align_lba(data->data,lba,direction));
	return 1;
}

static int lua_fdisk_align_lba_in_range(lua_State*L){
	LUA_ARG_MAX(4);
	fdisk_sector_t lba=luaL_checkinteger(L,2);
	fdisk_sector_t start=luaL_checkinteger(L,3);
	fdisk_sector_t stop=luaL_checkinteger(L,4);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_align_lba_in_range(data->data,lba,start,stop));
	return 1;
}

static int lua_fdisk_lba_is_phy_aligned(lua_State*L){
	LUA_ARG_MAX(2);
	fdisk_sector_t lba=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_lba_is_phy_aligned(data->data,lba));
	return 1;
}

static int lua_fdisk_override_geometry(lua_State*L){
	LUA_ARG_MAX(4);
	unsigned int cylinders=luaL_checkinteger(L,2);
	unsigned int heads=luaL_checkinteger(L,3);
	unsigned int sectors=luaL_checkinteger(L,4);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_override_geometry(data->data,cylinders,heads,sectors);
	return 0;
}

static int lua_fdisk_save_user_geometry(lua_State*L){
	LUA_ARG_MAX(4);
	unsigned int cylinders=luaL_checkinteger(L,2);
	unsigned int heads=luaL_checkinteger(L,3);
	unsigned int sectors=luaL_checkinteger(L,4);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_save_user_geometry(data->data,cylinders,heads,sectors);
	return 0;
}

static int lua_fdisk_save_user_sector_size(lua_State*L){
	LUA_ARG_MAX(3);
	unsigned int phy=luaL_checkinteger(L,2);
	unsigned int log=luaL_checkinteger(L,3);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_save_user_sector_size(data->data,phy,log);
	return 0;
}

static int lua_fdisk_save_user_grain(lua_State*L){
	LUA_ARG_MAX(2);
	unsigned long grain=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	fdisk_save_user_grain(data->data,grain);
	return 0;
}

static int lua_fdisk_has_user_device_properties(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_has_user_device_properties(data->data));
	return 1;
}

static int lua_fdisk_reset_alignment(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_reset_alignment(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_reset_device_properties(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_reset_device_properties(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_reread_partition_table(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_reread_partition_table(data->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_reread_changes(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_table*table=luaL_checkudata(L,2,LUA_FDISK_PARTITION);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!table||!table->data)return luaL_argerror(L,2,"invalid table");
	int ret=fdisk_reread_changes(data->data,table->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_new_script(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	struct fdisk_script*script=fdisk_new_script(data->data);
	if(!script)lua_pushnil(L);
	else lua_fdisk_script_to_lua(L,true,data->data,script);
	return 1;
}

static int lua_fdisk_new_script_from_file(lua_State*L){
	LUA_ARG_MAX(2);
	const char*filename=luaL_checkstring(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	struct fdisk_script*script=fdisk_new_script_from_file(data->data,filename);
	if(!script)lua_pushnil(L);
	else lua_fdisk_script_to_lua(L,true,data->data,script);
	return 1;
}

static int lua_fdisk_set_script(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_script*script=luaL_checkudata(L,2,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!script||!script->data)return luaL_argerror(L,2,"invalid script");
	int ret=fdisk_set_script(data->data,script->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_get_script(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	struct fdisk_script*script=fdisk_get_script(data->data);
	if(!script)lua_pushnil(L);
	else lua_fdisk_script_to_lua(L,false,data->data,script);
	return 1;
}

static int lua_fdisk_apply_script_headers(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_script*script=luaL_checkudata(L,2,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!script||!script->data)return luaL_argerror(L,2,"invalid script");
	int ret=fdisk_apply_script_headers(data->data,script->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_apply_script(lua_State*L){
	LUA_ARG_MAX(2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	struct lua_fdisk_script*script=luaL_checkudata(L,2,LUA_FDISK_SCRIPT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	if(!script||!script->data)return luaL_argerror(L,2,"invalid script");
	int ret=fdisk_apply_script(data->data,script->data);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_dos_fix_chs(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushinteger(L,fdisk_dos_fix_chs(data->data));
	return 1;
}

static int lua_fdisk_dos_move_begin(lua_State*L){
	LUA_ARG_MAX(2);
	size_t i=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_dos_move_begin(data->data,i);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_gpt_is_hybrid(lua_State*L){
	LUA_ARG_MAX(1);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	lua_pushboolean(L,fdisk_gpt_is_hybrid(data->data));
	return 1;
}

static int lua_fdisk_gpt_set_npartitions(lua_State*L){
	LUA_ARG_MAX(2);
	uint32_t nents=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_gpt_set_npartitions(data->data,nents);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_gpt_get_partition_attrs(lua_State*L){
	LUA_ARG_MAX(2);
	uint64_t attrs=0;
	size_t partnum=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_gpt_get_partition_attrs(data->data,partnum,&attrs);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	lua_pushinteger(L,attrs);
	return 2;
}

static int lua_fdisk_gpt_set_partition_attrs(lua_State*L){
	LUA_ARG_MAX(3);
	uint64_t attrs=luaL_checkinteger(L,3);
	size_t partnum=luaL_checkinteger(L,2);
	struct lua_fdisk_context*data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return luaL_argerror(L,1,"invalid context");
	int ret=fdisk_gpt_set_partition_attrs(data->data,partnum,attrs);
	if(lua_fdisk_check_error(L,ret))return 0;
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_fdisk_context_gc(lua_State*L){
	struct lua_fdisk_context*data;
	data=luaL_checkudata(L,1,LUA_FDISK_CONTEXT);
	if(!data||!data->data)return 0;
	if(data->ask_cb.func_ref>0)
		luaL_unref(L,LUA_REGISTRYINDEX,data->ask_cb.func_ref);
	if(data->ask_cb.data_ref>0)
		luaL_unref(L,LUA_REGISTRYINDEX,data->ask_cb.data_ref);
	fdisk_set_ask(data->data,NULL,NULL);
	fdisk_unref_context(data->data);
	data->data=NULL;
	return 0;
}

struct lua_fdisk_meta_table lua_fdisk_context={
	.name=LUA_FDISK_CONTEXT,
	.reg=(luaL_Reg[]){
		{"new_nested",                   lua_fdisk_new_nested},
		{"get_parent",                   lua_fdisk_get_parent},
		{"get_npartitions",              lua_fdisk_get_npartitions},
		{"get_label",                    lua_fdisk_get_label},
		{"next_label",                   lua_fdisk_next_label},
		{"get_nlabels",                  lua_fdisk_get_nlabels},
		{"has_label",                    lua_fdisk_has_label},
		{"is_labeltype",                 lua_fdisk_is_labeltype},
		{"assign_device",                lua_fdisk_assign_device},
		{"assign_device_by_fd",          lua_fdisk_assign_device_by_fd},
		{"deassign_device",              lua_fdisk_deassign_device},
		{"reassign_device",              lua_fdisk_reassign_device},
		{"is_readonly",                  lua_fdisk_is_readonly},
		{"is_regfile",                   lua_fdisk_is_regfile},
		{"device_is_used",               lua_fdisk_device_is_used},
		{"disable_dialogs",              lua_fdisk_disable_dialogs},
		{"has_dialogs",                  lua_fdisk_has_dialogs},
		{"enable_details",               lua_fdisk_enable_details},
		{"is_details",                   lua_fdisk_is_details},
		{"enable_listonly",              lua_fdisk_enable_listonly},
		{"is_listonly",                  lua_fdisk_is_listonly},
		{"enable_wipe",                  lua_fdisk_enable_wipe},
		{"has_wipe",                     lua_fdisk_has_wipe},
		{"get_collision",                lua_fdisk_get_collision},
		{"is_ptcollision",               lua_fdisk_is_ptcollision},
		{"set_unit",                     lua_fdisk_set_unit},
		{"get_unit",                     lua_fdisk_get_unit},
		{"use_cylinders",                lua_fdisk_use_cylinders},
		{"get_units_per_sector",         lua_fdisk_get_units_per_sector},
		{"get_optimal_iosize",           lua_fdisk_get_optimal_iosize},
		{"get_minimal_iosize",           lua_fdisk_get_minimal_iosize},
		{"get_physector_size",           lua_fdisk_get_physector_size},
		{"get_sector_size",              lua_fdisk_get_sector_size},
		{"get_alignment_offset",         lua_fdisk_get_alignment_offset},
		{"get_grain_size",               lua_fdisk_get_grain_size},
		{"get_first_lba",                lua_fdisk_get_first_lba},
		{"set_first_lba",                lua_fdisk_set_first_lba},
		{"get_last_lba",                 lua_fdisk_get_last_lba},
		{"set_last_lba",                 lua_fdisk_set_last_lba},
		{"get_nsectors",                 lua_fdisk_get_nsectors},
		{"get_devname",                  lua_fdisk_get_devname},
		{"get_devfd",                    lua_fdisk_get_devfd},
		{"get_devno",                    lua_fdisk_get_devno},
		{"get_devmodel",                 lua_fdisk_get_devmodel},
		{"get_geom_heads",               lua_fdisk_get_geom_heads},
		{"get_geom_sectors",             lua_fdisk_get_geom_sectors},
		{"get_geom_cylinders",           lua_fdisk_get_geom_cylinders},
		{"set_size_unit",                lua_fdisk_set_size_unit},
		{"get_size_unit",                lua_fdisk_get_size_unit},
		{"enable_bootbits_protection",   lua_fdisk_enable_bootbits_protection},
		{"has_protected_bootbits",       lua_fdisk_has_protected_bootbits},
		{"write_disklabel",              lua_fdisk_write_disklabel},
		{"verify_disklabel",             lua_fdisk_verify_disklabel},
		{"create_disklabel",             lua_fdisk_create_disklabel},
		{"list_disklabel",               lua_fdisk_list_disklabel},
		{"locate_disklabel",             lua_fdisk_locate_disklabel},
		{"get_disklabel_item",           lua_fdisk_get_disklabel_item},
		{"get_disklabel_id",             lua_fdisk_get_disklabel_id},
		{"set_disklabel_id",             lua_fdisk_set_disklabel_id},
		{"set_disklabel_id_from_string", lua_fdisk_set_disklabel_id_from_string},
		{"get_partition",                lua_fdisk_get_partition},
		{"set_partition",                lua_fdisk_set_partition},
		{"add_partition",                lua_fdisk_add_partition},
		{"delete_partition",             lua_fdisk_delete_partition},
		{"delete_all_partitions",        lua_fdisk_delete_all_partitions},
		{"wipe_partition",               lua_fdisk_wipe_partition},
		{"set_partition_type",           lua_fdisk_set_partition_type},
		{"is_partition_used",            lua_fdisk_is_partition_used},
		{"toggle_partition_flag",        lua_fdisk_toggle_partition_flag},
		{"reorder_partitions",           lua_fdisk_reorder_partitions},
		{"partition_has_wipe",           lua_fdisk_partition_has_wipe},
		{"get_partitions",               lua_fdisk_get_partitions},
		{"get_freespaces",               lua_fdisk_get_freespaces},
		{"apply_table",                  lua_fdisk_apply_table},
		{"align_lba",                    lua_fdisk_align_lba},
		{"align_lba_in_range",           lua_fdisk_align_lba_in_range},
		{"lba_is_phy_aligned",           lua_fdisk_lba_is_phy_aligned},
		{"override_geometry",            lua_fdisk_override_geometry},
		{"save_user_geometry",           lua_fdisk_save_user_geometry},
		{"save_user_sector_size",        lua_fdisk_save_user_sector_size},
		{"save_user_grain",              lua_fdisk_save_user_grain},
		{"has_user_device_properties",   lua_fdisk_has_user_device_properties},
		{"reset_alignment",              lua_fdisk_reset_alignment},
		{"reset_device_properties",      lua_fdisk_reset_device_properties},
		{"reread_partition_table",       lua_fdisk_reread_partition_table},
		{"reread_changes",               lua_fdisk_reread_changes},
		{"new_script",                   lua_fdisk_new_script},
		{"new_script_from_file",         lua_fdisk_new_script_from_file},
		{"set_script",                   lua_fdisk_set_script},
		{"get_script",                   lua_fdisk_get_script},
		{"apply_script_headers",         lua_fdisk_apply_script_headers},
		{"apply_script",                 lua_fdisk_apply_script},
		{"dos_fix_chs",                  lua_fdisk_dos_fix_chs},
		{"dos_move_begin",               lua_fdisk_dos_move_begin},
		{"gpt_is_hybrid",                lua_fdisk_gpt_is_hybrid},
		{"gpt_set_npartitions",          lua_fdisk_gpt_set_npartitions},
		{"gpt_get_partition_attrs",      lua_fdisk_gpt_get_partition_attrs},
		{"gpt_set_partition_attrs",      lua_fdisk_gpt_set_partition_attrs},
		{NULL, NULL}
	},
	.tostring=NULL,
	.gc=lua_fdisk_context_gc,
};

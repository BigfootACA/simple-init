/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LUA
#ifdef ENABLE_MXML
#include"gui/lua.h"
#include"../engine/render_internal.h"

static int lua_render_obj_get_obj(lua_State*L){
	struct lua_render_obj_data*e=luaL_checkudata(L,1,LUA_RENDER_OBJ);
	lvgl_obj_to_lua(L,e->obj->obj);
	return 1;
}

static int lua_render_obj_get_id(lua_State*L){
	struct lua_render_obj_data*e=luaL_checkudata(L,1,LUA_RENDER_OBJ);
	lua_pushstring(L,e->obj->id);
	return 1;
}

static int lua_render_obj_get_render(lua_State*L){
	struct lua_render_obj_data*e=luaL_checkudata(L,1,LUA_RENDER_OBJ);
	render_to_lua(L,e->obj->render);
	return 1;
}

static int lua_render_obj_get_parent(lua_State*L){
	struct lua_render_obj_data*e=luaL_checkudata(L,1,LUA_RENDER_OBJ);
	render_obj_to_lua(L,e->obj->parent);
	return 1;
}

static struct luaL_Reg render_obj_func[]={
	{"get_render",lua_render_obj_get_render},
	{"get_parent",lua_render_obj_get_parent},
	{"get_obj",lua_render_obj_get_obj},
	{"get_id",lua_render_obj_get_id},
	{NULL, NULL},
};

static int lua_render_obj_tostring(lua_State*L){
	struct lua_render_obj_data*e=luaL_checkudata(L,1,LUA_RENDER_OBJ);
	lua_pushfstring(L,LUA_RENDER_OBJ" %s (%p)",e->obj->id,e->obj);
	return 1;
}

void render_obj_to_lua(lua_State*L,xml_render_obj*obj){
	struct lua_render_obj_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_render_obj_data));
	luaL_getmetatable(L,LUA_RENDER_OBJ);
	lua_setmetatable(L,-2);
	e->obj=obj;
}

int render_obj_lua_init(lua_State*L){
	xlua_create_metatable(L,LUA_RENDER_OBJ,render_obj_func,lua_render_obj_tostring,NULL);
	return 1;
}
#endif
#endif
#endif

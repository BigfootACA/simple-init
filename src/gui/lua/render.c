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

static int lua_render_get_root_obj(lua_State*L){
	struct lua_render_data*e=luaL_checkudata(L,1,LUA_RENDER);
	lvgl_obj_to_lua(L,e->render->root_obj);
	return 1;
}

static int lua_render_find_object(lua_State*L){
	struct lua_render_data*e=luaL_checkudata(L,1,LUA_RENDER);
	lvgl_obj_to_lua(L,e->render->root_obj);
	return 1;
}

static struct luaL_Reg render_func[]={
	{"get_root_obj",lua_render_get_root_obj},
	{"find_object",lua_render_find_object},
	{NULL, NULL},
};

static int lua_render_tostring(lua_State*L){
	struct lua_render_data*e=luaL_checkudata(L,1,LUA_RENDER);
	lua_pushfstring(L,LUA_RENDER" (%p)",e->render);
	return 1;
}

void render_to_lua(lua_State*L,xml_render*render){
	struct lua_render_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_render_data));
	luaL_getmetatable(L,LUA_RENDER);
	lua_setmetatable(L,-2);
	e->render=render;
}

extern int render_lua_init_event(lua_State*L);
LUAMOD_API int luaopen_xml_render(lua_State*L){
	xlua_create_metatable(L,LUA_RENDER,render_func,lua_render_tostring,NULL);
	render_lua_init_event(L);
	return 1;
}
#endif
#endif
#endif

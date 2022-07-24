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
#include"gui/tools.h"
#include"gui/string.h"
#include"../engine/render_internal.h"

static int lua_render_event_get_type(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	const char*event=lv_event_code_to_name(e->event->event->code);
	if(event)lua_pushstring(L,event);
	else lua_pushnil(L);
	return 1;
}

static int lua_render_event_get_event_id(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	lua_pushstring(L,e->event->info->event_id);
	return 1;
}

static int lua_render_event_get_render(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	render_to_lua(L,e->event->info->obj->render);
	return 1;
}

static int lua_render_event_get_object(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	render_obj_to_lua(L,e->event->info->obj);
	return 1;
}

static int lua_render_event_get_object_id(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	lua_pushstring(L,e->event->info->obj->id);
	return 1;
}

static int lua_render_event_get_info(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	render_event_info_to_lua(L,e->event->info);
	return 1;
}

static int lua_render_event_get_lvgl_obj(lua_State*L){
	struct lua_render_event_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT);
	lvgl_obj_to_lua(L,e->event->info->obj->obj);
	return 1;
}

static int lua_render_event_info_get_event_id(lua_State*L){
	struct lua_render_event_info_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT_INFO);
	lua_pushstring(L,e->info->event_id);
	return 1;
}

static int lua_render_event_info_get_render(lua_State*L){
	struct lua_render_event_info_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT_INFO);
	render_to_lua(L,e->info->obj->render);
	return 1;
}

static int lua_render_event_info_get_object(lua_State*L){
	struct lua_render_event_info_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT_INFO);
	render_obj_to_lua(L,e->info->obj);
	return 1;
}

static int lua_render_event_info_get_object_id(lua_State*L){
	struct lua_render_event_info_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT_INFO);
	lua_pushstring(L,e->info->obj_name);
	return 1;
}

static int lua_render_event_info_get_lvgl_obj(lua_State*L){
	struct lua_render_event_info_data*e=luaL_checkudata(L,1,LUA_RENDER_EVENT_INFO);
	lvgl_obj_to_lua(L,e->info->obj->obj);
	return 1;
}

static struct luaL_Reg lua_render_event_func[]={
	{"get_type",lua_render_event_get_type},
	{"get_render",lua_render_event_get_render},
	{"get_object",lua_render_event_get_object},
	{"get_event_id",lua_render_event_get_event_id},
	{"get_object_id",lua_render_event_get_object_id},
	{"get_lvgl_obj",lua_render_event_get_lvgl_obj},
	{"get_info",lua_render_event_get_info},
	{NULL, NULL},
};

static struct luaL_Reg lua_render_event_info_func[]={
	{"get_render",lua_render_event_info_get_render},
	{"get_object",lua_render_event_info_get_object},
	{"get_event_id",lua_render_event_info_get_event_id},
	{"get_object_id",lua_render_event_info_get_object_id},
	{"get_lvgl_obj",lua_render_event_info_get_lvgl_obj},
	{NULL, NULL},
};

void render_event_to_lua(lua_State*L,xml_render_event*event){
	struct lua_render_event_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_render_event_data));
	luaL_getmetatable(L,LUA_RENDER_EVENT);
	lua_setmetatable(L,-2);
	e->event=event;
}

void render_event_info_to_lua(lua_State*L,xml_event_info*info){
	struct lua_render_event_info_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_render_event_info_data));
	luaL_getmetatable(L,LUA_RENDER_EVENT_INFO);
	lua_setmetatable(L,-2);
	e->info=info;
}

int render_lua_init_event(lua_State*L){
	xlua_create_metatable(L,LUA_RENDER_EVENT,lua_render_event_func,NULL,NULL);
	xlua_create_metatable(L,LUA_RENDER_EVENT_INFO,lua_render_event_info_func,NULL,NULL);
	return 1;
}
#endif
#endif
#endif

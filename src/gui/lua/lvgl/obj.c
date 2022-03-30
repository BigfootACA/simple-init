/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"gui/lua.h"
#define GET_OBJ(var,n)struct lua_lvgl_obj_data*var=luaL_checkudata(L,n,LUA_LVGL_OBJ)
#define COORD_FUNC(target)\
	int lua_lvgl_obj_##target(lua_State*L){\
		GET_OBJ(o,1);\
		if(!lua_isnoneornil(L,2))lv_obj_set_##target(\
			o->obj,(lv_coord_t)luaL_checkinteger(L,2)\
		);\
		lua_pushinteger(L,lv_obj_get_##target(o->obj));\
		return 1;\
	}
#define VOID_FUNC(target)\
	int lua_lvgl_obj_##target(lua_State*L){\
		GET_OBJ(o,1);\
		lv_obj_##target(o->obj);\
		lua_pushboolean(L,true);\
		return 1;\
	}
#define SET_BOOL_FUNC(target)\
	int lua_lvgl_obj_set_##target(lua_State*L){\
		GET_OBJ(o,1);\
		luaL_checktype(L,2,LUA_TBOOLEAN);\
		lv_obj_set_##target(o->obj,lua_toboolean(L,2));\
		lua_pushboolean(L,true);\
		return 1;\
	}
#define GET_BOOL_FUNC(target)\
	int lua_lvgl_obj_##target(lua_State*L){\
		GET_OBJ(o,1);\
		lua_pushboolean(L,lv_obj_##target(o->obj));\
		return 1;\
	}

void lvgl_obj_to_lua(lua_State*L,lv_obj_t*obj){
	const char*name;
	lv_obj_type_t types;
	struct lua_lvgl_obj_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_lvgl_obj_data));
	luaL_getmetatable(L,LUA_LVGL_OBJ);
	lua_setmetatable(L,-2);
	e->obj=obj;
	lv_obj_get_type(obj,&types);
	name=types.type[0];
	if(strncmp(name,"lv_",3)==0)name+=3;
	e->type=lvgl_string_to_type(name);
}

int lua_lvgl_get_scr_act(lua_State*L){
	lvgl_obj_to_lua(L,lv_scr_act());
	return 1;
}

int lua_lvgl_obj_create(lua_State*L){
	lv_obj_t*obj;
	GET_OBJ(o,1);
	enum lvgl_obj_type type=lvgl_string_to_type(luaL_checkstring(L,2));
	if(type<=LV_OBJ_NONE||type>=LV_OBJ_LAST)
		return luaL_error(L,"invalid lvgl object type");
	if(!(obj=lvgl_object_create(o->obj,type)))
		return luaL_error(L,"create lvgl object failed");
	lvgl_obj_to_lua(L,obj);
	return 1;
}

int lua_lvgl_obj_set_parent(lua_State*L){
	GET_OBJ(o1,1);
	GET_OBJ(o2,2);
	lv_obj_set_parent(o1->obj,o2->obj);
	lua_pushboolean(L,true);
	return 1;
}

int lua_lvgl_obj_get_parent(lua_State*L){
	GET_OBJ(o,1);
	lvgl_obj_to_lua(L,lv_obj_get_parent(o->obj));
	return 1;
}

int lua_lvgl_obj_get_screen(lua_State*L){
	GET_OBJ(o,1);
	lvgl_obj_to_lua(L,lv_obj_get_screen(o->obj));
	return 1;
}

int lua_lvgl_obj_get_focused_obj(lua_State*L){
	GET_OBJ(o,1);
	lvgl_obj_to_lua(L,lv_obj_get_focused_obj(o->obj));
	return 1;
}

int lua_lvgl_obj_size(lua_State*L){
	GET_OBJ(o,1);
	if(!lua_isnoneornil(L,2))lv_obj_set_size(
		o->obj,
		(lv_coord_t)luaL_checkinteger(L,2),
		(lv_coord_t)luaL_checkinteger(L,3)
	);
	lua_pushinteger(L,lv_obj_get_width(o->obj));
	lua_pushinteger(L,lv_obj_get_height(o->obj));
	return 1;
}

int lua_lvgl_obj_pos(lua_State*L){
	GET_OBJ(o,1);
	if(!lua_isnoneornil(L,2))lv_obj_set_pos(
		o->obj,
		(lv_coord_t)luaL_checkinteger(L,2),
		(lv_coord_t)luaL_checkinteger(L,3)
	);
	lua_pushinteger(L,lv_obj_get_x(o->obj));
	lua_pushinteger(L,lv_obj_get_y(o->obj));
	return 1;
}

COORD_FUNC(x)
COORD_FUNC(y)
COORD_FUNC(width)
COORD_FUNC(width_fit)
COORD_FUNC(width_margin)
COORD_FUNC(height)
COORD_FUNC(height_fit)
COORD_FUNC(height_margin)
SET_BOOL_FUNC(auto_realign)
SET_BOOL_FUNC(hidden)
SET_BOOL_FUNC(adv_hittest)
SET_BOOL_FUNC(click)
SET_BOOL_FUNC(top)
SET_BOOL_FUNC(drag)
SET_BOOL_FUNC(drag_throw)
SET_BOOL_FUNC(drag_parent)
SET_BOOL_FUNC(focus_parent)
SET_BOOL_FUNC(gesture_parent)
SET_BOOL_FUNC(parent_event)
GET_BOOL_FUNC(is_visible)
GET_BOOL_FUNC(get_auto_realign)
GET_BOOL_FUNC(get_hidden)
GET_BOOL_FUNC(get_adv_hittest)
GET_BOOL_FUNC(get_click)
GET_BOOL_FUNC(get_top)
GET_BOOL_FUNC(get_drag)
GET_BOOL_FUNC(get_drag_throw)
GET_BOOL_FUNC(get_drag_parent)
GET_BOOL_FUNC(get_focus_parent)
GET_BOOL_FUNC(get_parent_event)
GET_BOOL_FUNC(get_gesture_parent)
GET_BOOL_FUNC(is_focused)
VOID_FUNC(clean)
VOID_FUNC(del)
VOID_FUNC(del_async)
VOID_FUNC(invalidate)
VOID_FUNC(move_foreground)
VOID_FUNC(move_background)
VOID_FUNC(realign)

#endif

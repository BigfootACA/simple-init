/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LUA
#include"gui/lua.h"

static struct class_names{
	enum lvgl_obj_type type;
	const char*name;
	const lv_obj_class_t*cls;
}lv_cls[]={
	#define LV_CLS(_type,_name){\
		.type=(_type),\
		.name=(#_name),\
		.cls=&lv_##_name##_class\
	},
	#include"obj_class.h"
	{LV_OBJ_NONE,NULL,NULL}
};

const lv_obj_class_t*lvgl_type_to_class(enum lvgl_obj_type type){
	for(size_t i=0;lv_cls[i].cls;i++)
		if(lv_cls[i].type==type)
			return lv_cls[i].cls;
	return NULL;
}

const char*lvgl_type_to_string(enum lvgl_obj_type type){
	for(size_t i=0;lv_cls[i].cls;i++)
		if(lv_cls[i].type==type)
			return lv_cls[i].name;
	return NULL;
}

enum lvgl_obj_type lvgl_class_to_type(const lv_obj_class_t*cls){
	if(!cls)return LV_OBJ_NONE;
	for(size_t i=0;lv_cls[i].cls;i++)
		if(lv_cls[i].cls==cls)
			return lv_cls[i].type;
	if(cls->base_class!=NULL&&cls->base_class!=cls)
		return lvgl_class_to_type(cls->base_class);
	return LV_OBJ_NONE;
}

enum lvgl_obj_type lvgl_obj_to_type(const lv_obj_t*obj){
	return obj?lvgl_class_to_type(lv_obj_get_class(obj)):LV_OBJ_NONE;
}

enum lvgl_obj_type lvgl_string_to_type(const char*name){
	if(!name)return LV_OBJ_NONE;
	for(size_t i=0;lv_cls[i].name;i++)
		if(strcasecmp(name,lv_cls[i].name)==0)
			return lv_cls[i].type;
	return LV_OBJ_NONE;
}

const lv_obj_class_t*lvgl_string_to_class(const char*name){
	return lvgl_type_to_class(lvgl_string_to_type(name));
}

const char*lvgl_class_to_string(const lv_obj_class_t*cls){
	return lvgl_type_to_string(lvgl_class_to_type(cls));
}

const char*lvgl_obj_to_string(const lv_obj_t*obj){
	return lvgl_type_to_string(lvgl_obj_to_type(obj));
}

int lua_lvgl_obj_tostring(lua_State*L){
	struct lua_lvgl_obj_data*o=luaL_checkudata(L,1,LUA_LVGL_OBJ);
	const char*type=lvgl_obj_to_string(o->obj);
	if(!type)type="(unknown)";
	lua_pushfstring(L,"LittleVGL %s object (%p)",type,o->obj);
	return 1;
}

#endif
#endif

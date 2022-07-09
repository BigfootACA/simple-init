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
#define GET_COORD_FUNC(target)\
	int lua_lvgl_obj_get_##target(lua_State*L){\
		GET_OBJ(o,1);\
		lua_pushinteger(L,lv_obj_get_##target(o->obj));\
		return 1;\
	}
#define GET_BOOL_FUNC(target)\
	int lua_lvgl_obj_##target(lua_State*L){\
		GET_OBJ(o,1);\
		lua_pushboolean(L,lv_obj_##target(o->obj));\
		return 1;\
	}

void lvgl_obj_to_lua(lua_State*L,lv_obj_t*obj){
	struct lua_lvgl_obj_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_lvgl_obj_data));
	luaL_getmetatable(L,LUA_LVGL_OBJ);
	lua_setmetatable(L,-2);
	e->obj=obj;
	e->type=lvgl_obj_to_type(obj);
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
	switch(type){
		#if LV_USE_ANIMIMG
		case LV_OBJ_ANIMIMG:obj=lv_animimg_create(o->obj);break;
		#endif
		#if LV_USE_ARC
		case LV_OBJ_ARC:obj=lv_arc_create(o->obj);break;
		#endif
		#if LV_USE_BAR
		case LV_OBJ_BAR:obj=lv_bar_create(o->obj);break;
		#endif
		#if LV_USE_BTN
		case LV_OBJ_BTN:obj=lv_btn_create(o->obj);break;
		#endif
		#if LV_USE_BTNMATRIX
		case LV_OBJ_BTNMATRIX:obj=lv_btnmatrix_create(o->obj);break;
		#endif
		#if LV_USE_CALENDAR
		case LV_OBJ_CALENDAR:obj=lv_calendar_create(o->obj);break;
		#endif
		#if LV_USE_CANVAS
		case LV_OBJ_CANVAS:obj=lv_canvas_create(o->obj);break;
		#endif
		#if LV_USE_CHART
		case LV_OBJ_CHART:obj=lv_chart_create(o->obj);break;
		#endif
		#if LV_USE_CHECKBOX
		case LV_OBJ_CHECKBOX:obj=lv_checkbox_create(o->obj);break;
		#endif
		#if LV_USE_DROPDOWN
		case LV_OBJ_DROPDOWN:obj=lv_dropdown_create(o->obj);break;
		#endif
		#if LV_USE_FFMPEG
		case LV_OBJ_FFMPEG_PLAYER:obj=lv_ffmpeg_player_create(o->obj);break;
		#endif
		#if LV_USE_GIF
		case LV_OBJ_GIF:obj=lv_gif_create(o->obj);break;
		#endif
		#if LV_USE_IMGBTN
		case LV_OBJ_IMGBTN:obj=lv_imgbtn_create(o->obj);break;
		#endif
		#if LV_USE_IMG
		case LV_OBJ_IMG:obj=lv_img_create(o->obj);break;
		#endif
		#if LV_USE_KEYBOARD
		case LV_OBJ_KEYBOARD:obj=lv_keyboard_create(o->obj);break;
		#endif
		#if LV_USE_LABEL
		case LV_OBJ_LABEL:obj=lv_label_create(o->obj);break;
		#endif
		#if LV_USE_LED
		case LV_OBJ_LED:obj=lv_led_create(o->obj);break;
		#endif
		#if LV_USE_LINE
		case LV_OBJ_LINE:obj=lv_line_create(o->obj);break;
		#endif
		#if LV_USE_LIST
		case LV_OBJ_LIST:obj=lv_list_create(o->obj);break;
		#endif
		#if LV_USE_MENU
		case LV_OBJ_MENU:obj=lv_menu_create(o->obj);break;
		case LV_OBJ_MENU_CONT:obj=lv_menu_cont_create(o->obj);break;
		case LV_OBJ_MENU_PAGE:obj=lv_menu_page_create(
			o->obj,
			strdup(luaL_checkstring(L,3)) // title
		);break;
		case LV_OBJ_MENU_SECTION:obj=lv_menu_section_create(o->obj);break;
		case LV_OBJ_MENU_SEPARATOR:obj=lv_menu_separator_create(o->obj);break;
		#endif
		#if LV_USE_METER
		case LV_OBJ_METER:obj=lv_meter_create(o->obj);break;
		#endif
		#if LV_USE_MSGBOX
		case LV_OBJ_MSGBOX:{
			size_t cnt=luaL_len(L,5);
			size_t size=(cnt+2)*sizeof(char*);
			const char**texts=NULL;
			if(!(texts=malloc(size)))
				return luaL_error(L,"alloc array failed");
			for(size_t i=0;i<cnt;i++){
				lua_rawgeti(L,5,i+1);
				texts[i]=luaL_checkstring(L,lua_gettop(L));
				lua_pop(L,1);
			}
			texts[cnt]="",texts[cnt+1]=NULL;
			obj=lv_msgbox_create(
				o->obj,
				luaL_checkstring(L,3), // title
				luaL_checkstring(L,4), // text
				texts,                 // buttons text
				lua_toboolean(L,6)     // add close btn
			);
			free(texts);
		}break;
		#endif
		#if LV_USE_QRCODE
		case LV_OBJ_QRCODE:obj=lv_qrcode_create(
			o->obj,
			luaL_checkinteger(L,3), // size
			lv_color_black(),       // dark color
			lv_color_white()        // light color
		);break;
		#endif
		#if LV_USE_ROLLER
		case LV_OBJ_ROLLER:obj=lv_roller_create(o->obj);break;
		#endif
		#if LV_USE_SLIDER
		case LV_OBJ_SLIDER:obj=lv_slider_create(o->obj);break;
		#endif
		#if LV_USE_SPAN
		case LV_OBJ_SPANGROUP:obj=lv_spangroup_create(o->obj);break;
		#endif
		#if LV_USE_SPINBOX
		case LV_OBJ_SPINBOX:obj=lv_spinbox_create(o->obj);break;
		#endif
		#if LV_USE_SPINNER
		case LV_OBJ_SPINNER:obj=lv_spinner_create(
			o->obj,
			luaL_checkinteger(L,4), // time
			luaL_checkinteger(L,5)  // arc len
		);break;
		#endif
		#if LV_USE_SWITCH
		case LV_OBJ_SWITCH:obj=lv_switch_create(o->obj);break;
		#endif
		#if LV_USE_TABLE
		case LV_OBJ_TABLE:obj=lv_table_create(o->obj);break;
		#endif
		#if LV_USE_TABVIEW
		case LV_OBJ_TABVIEW:obj=lv_tabview_create(
			o->obj,
			LV_DIR_NONE/*FIXME*/,  // tab pos
			luaL_checkinteger(L,4) // tab size
		);break;
		#endif
		#if LV_USE_TEXTAREA
		case LV_OBJ_TEXTAREA:obj=lv_textarea_create(o->obj);break;
		#endif
		#if LV_USE_TILEVIEW
		case LV_OBJ_TILEVIEW:obj=lv_tileview_create(o->obj);break;
		#endif
		#if LV_USE_WIN
		case LV_OBJ_WIN:obj=lv_win_create(
			o->obj,
			luaL_checkinteger(L,3) // header height
		);break;
		#endif
		default:return luaL_error(L,"lvgl object unsupported");
	}
	if(!obj)return luaL_error(L,"create lvgl object failed");
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
COORD_FUNC(height)
COORD_FUNC(content_width)
COORD_FUNC(content_height)
GET_COORD_FUNC(x2)
GET_COORD_FUNC(y2)
GET_COORD_FUNC(x_aligned)
GET_COORD_FUNC(y_aligned)
GET_COORD_FUNC(self_width)
GET_COORD_FUNC(self_height)
GET_BOOL_FUNC(refr_size)
GET_BOOL_FUNC(is_visible)
GET_BOOL_FUNC(is_layout_positioned)
GET_BOOL_FUNC(refresh_self_size)
VOID_FUNC(clean)
VOID_FUNC(center)
VOID_FUNC(del)
VOID_FUNC(del_async)
VOID_FUNC(invalidate)
VOID_FUNC(move_foreground)
VOID_FUNC(move_background)
VOID_FUNC(mark_layout_as_dirty)
VOID_FUNC(update_layout)
VOID_FUNC(refr_pos)

#endif
#endif

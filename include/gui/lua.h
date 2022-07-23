/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _GUI_LUA_H
#define _GUI_LUA_H
#include"gui.h"
#include"xlua.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#define LUA_LVGL_OBJ "LittleVGL Object"
#define LUA_GUI_ACTIVITY "GUI Activity"
#define LUA_GUI_REGISTER "GUI Activity Register"

enum lvgl_obj_type{
	LV_OBJ_NONE=0,
	LV_OBJ_OBJ,
	LV_OBJ_ANIMIMG,
	LV_OBJ_ARC,
	LV_OBJ_BAR,
	LV_OBJ_BTN,
	LV_OBJ_BTNMATRIX,
	LV_OBJ_CALENDAR,
	LV_OBJ_CALENDAR_HEADER_ARROW,
	LV_OBJ_CALENDAR_HEADER_DROPDOWN,
	LV_OBJ_CANVAS,
	LV_OBJ_CHART,
	LV_OBJ_CHECKBOX,
	LV_OBJ_COLORWHEEL,
	LV_OBJ_DROPDOWN,
	LV_OBJ_DROPDOWNLIST,
	LV_OBJ_FFMPEG_PLAYER,
	LV_OBJ_GIF,
	LV_OBJ_IMGBTN,
	LV_OBJ_IMG,
	LV_OBJ_KEYBOARD,
	LV_OBJ_LABEL,
	LV_OBJ_LED,
	LV_OBJ_LINE,
	LV_OBJ_LIST,
	LV_OBJ_LIST_BTN,
	LV_OBJ_LIST_TEXT,
	LV_OBJ_MENU,
	LV_OBJ_MENU_CONT,
	LV_OBJ_MENU_MAIN_CONT,
	LV_OBJ_MENU_MAIN_HEADER_CONT,
	LV_OBJ_MENU_PAGE,
	LV_OBJ_MENU_SECTION,
	LV_OBJ_MENU_SEPARATOR,
	LV_OBJ_MENU_SIDEBAR_CONT,
	LV_OBJ_MENU_SIDEBAR_HEADER_CONT,
	LV_OBJ_METER,
	LV_OBJ_MSGBOX,
	LV_OBJ_MSGBOX_BACKDROP,
	LV_OBJ_MSGBOX_CONTENT,
	LV_OBJ_QRCODE,
	LV_OBJ_RLOTTIE,
	LV_OBJ_ROLLER,
	LV_OBJ_SLIDER,
	LV_OBJ_SPANGROUP,
	LV_OBJ_SPINBOX,
	LV_OBJ_SPINNER,
	LV_OBJ_SWITCH,
	LV_OBJ_TABLE,
	LV_OBJ_TABVIEW,
	LV_OBJ_TEXTAREA,
	LV_OBJ_TILEVIEW,
	LV_OBJ_TILEVIEW_TILE,
	LV_OBJ_WIN,
	LV_OBJ_LAST,
};
struct lua_lvgl_obj_data{
	lv_obj_t*obj;
	enum lvgl_obj_type type;
};
struct lua_gui_activity_data{
	struct gui_activity*act;
};
struct lua_gui_register_data{
	struct gui_register*reg;
};
extern const lv_obj_class_t*lvgl_type_to_class(enum lvgl_obj_type type);
extern const char*lvgl_type_to_string(enum lvgl_obj_type type);
extern enum lvgl_obj_type lvgl_obj_to_type(const lv_obj_t*obj);
extern enum lvgl_obj_type lvgl_class_to_type(const lv_obj_class_t*cls);
extern const lv_obj_class_t*lvgl_string_to_class(const char*name);
extern const char*lvgl_class_to_string(const lv_obj_class_t*cls);
extern const char*lvgl_obj_to_string(const lv_obj_t*obj);
extern enum lvgl_obj_type lvgl_string_to_type(const char*name);
extern void lvgl_obj_to_lua(lua_State*L,lv_obj_t*obj);
extern void guiact_to_lua(lua_State*L,struct gui_activity*act);
extern void guireg_to_lua(lua_State*L,struct gui_register*reg);
extern void lua_gui_init(lua_State*L);
#endif

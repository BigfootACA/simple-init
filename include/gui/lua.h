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
	LV_OBJ_ARC,
	LV_OBJ_BAR,
	LV_OBJ_BTN,
	LV_OBJ_BTNMATRIX,
	LV_OBJ_CALENDAR,
	LV_OBJ_CANVAS,
	LV_OBJ_CHART,
	LV_OBJ_CHECKBOX,
	LV_OBJ_CONT,
	LV_OBJ_CPICKER,
	LV_OBJ_DROPDOWN,
	LV_OBJ_GAUGE,
	LV_OBJ_IMGBTN,
	LV_OBJ_IMG,
	LV_OBJ_KEYBOARD,
	LV_OBJ_LABEL,
	LV_OBJ_LED,
	LV_OBJ_LINE,
	LV_OBJ_LINEMETER,
	LV_OBJ_LIST,
	LV_OBJ_MSGBOX,
	LV_OBJ_OBJMASK,
	LV_OBJ_PAGE,
	LV_OBJ_ROLLER,
	LV_OBJ_SLIDER,
	LV_OBJ_SPINBOX,
	LV_OBJ_SPINNER,
	LV_OBJ_SWITCH,
	LV_OBJ_TABLE,
	LV_OBJ_TABVIEW,
	LV_OBJ_TEXTAREA,
	LV_OBJ_TILEVIEW,
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
extern enum lvgl_obj_type lvgl_string_to_type(const char*name);
extern lv_obj_t*lvgl_object_create(lv_obj_t*obj,enum lvgl_obj_type type);
extern void lvgl_obj_to_lua(lua_State*L,lv_obj_t*obj);
extern void guiact_to_lua(lua_State*L,struct gui_activity*act);
extern void guireg_to_lua(lua_State*L,struct gui_register*reg);
#endif

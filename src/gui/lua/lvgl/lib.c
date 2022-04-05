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

static const char*types[]={
	[LV_OBJ_NONE]      = NULL,
	[LV_OBJ_ARC]       = "arc",
	[LV_OBJ_BAR]       = "bar",
	[LV_OBJ_BTN]       = "btn",
	[LV_OBJ_BTNMATRIX] = "btnmatrix",
	[LV_OBJ_CALENDAR]  = "calendar",
	[LV_OBJ_CANVAS]    = "canvas",
	[LV_OBJ_CHART]     = "chart",
	[LV_OBJ_CHECKBOX]  = "checkbox",
	[LV_OBJ_CONT]      = "cont",
	[LV_OBJ_CPICKER]   = "cpicker",
	[LV_OBJ_DROPDOWN]  = "dropdown",
	[LV_OBJ_GAUGE]     = "gauge",
	[LV_OBJ_IMGBTN]    = "imgbtn",
	[LV_OBJ_IMG]       = "img",
	[LV_OBJ_KEYBOARD]  = "keyboard",
	[LV_OBJ_LABEL]     = "label",
	[LV_OBJ_LED]       = "led",
	[LV_OBJ_LINE]      = "line",
	[LV_OBJ_LINEMETER] = "linemeter",
	[LV_OBJ_LIST]      = "list",
	[LV_OBJ_MSGBOX]    = "msgbox",
	[LV_OBJ_OBJMASK]   = "objmask",
	[LV_OBJ_PAGE]      = "page",
	[LV_OBJ_ROLLER]    = "roller",
	[LV_OBJ_SLIDER]    = "slider",
	[LV_OBJ_SPINBOX]   = "spinbox",
	[LV_OBJ_SPINNER]   = "spinner",
	[LV_OBJ_SWITCH]    = "switch",
	[LV_OBJ_TABLE]     = "table",
	[LV_OBJ_TABVIEW]   = "tabview",
	[LV_OBJ_TEXTAREA]  = "textarea",
	[LV_OBJ_TILEVIEW]  = "tileview",
	[LV_OBJ_WIN]       = "win",
	[LV_OBJ_LAST]      = NULL,
};

enum lvgl_obj_type lvgl_string_to_type(const char*name){
	if(!name)return LV_OBJ_NONE;
	for(enum lvgl_obj_type i=LV_OBJ_NONE;i<LV_OBJ_LAST;i++)
		if(types[i]&&strcasecmp(name,types[i])==0)
			return i;
	return LV_OBJ_NONE;
}

lv_obj_t*lvgl_object_create(lv_obj_t*obj,enum lvgl_obj_type type){
	if(!obj)return NULL;
	switch(type){
		#if LV_USE_ARC
		case LV_OBJ_ARC:return lv_arc_create(obj,NULL);
		#endif
		#if LV_USE_BAR
		case LV_OBJ_BAR:return lv_bar_create(obj,NULL);
		#endif
		#if LV_USE_BTN
		case LV_OBJ_BTN:return lv_btn_create(obj,NULL);
		#endif
		#if LV_USE_BTNMATRIX
		case LV_OBJ_BTNMATRIX:return lv_btnmatrix_create(obj,NULL);
		#endif
		#if LV_USE_CALENDAR
		case LV_OBJ_CALENDAR:return lv_calendar_create(obj,NULL);
		#endif
		#if LV_USE_CANVAS
		case LV_OBJ_CANVAS:return lv_canvas_create(obj,NULL);
		#endif
		#if LV_USE_CHART
		case LV_OBJ_CHART:return lv_chart_create(obj,NULL);
		#endif
		#if LV_USE_CHECKBOX
		case LV_OBJ_CHECKBOX:return lv_checkbox_create(obj,NULL);
		#endif
		#if LV_USE_CONT
		case LV_OBJ_CONT:return lv_cont_create(obj,NULL);
		#endif
		#if LV_USE_CPICKER
		case LV_OBJ_CPICKER:return lv_cpicker_create(obj,NULL);
		#endif
		#if LV_USE_DROPDOWN
		case LV_OBJ_DROPDOWN:return lv_dropdown_create(obj,NULL);
		#endif
		#if LV_USE_GAUGE
		case LV_OBJ_GAUGE:return lv_gauge_create(obj,NULL);
		#endif
		#if LV_USE_IMGBTN
		case LV_OBJ_IMGBTN:return lv_imgbtn_create(obj,NULL);
		#endif
		#if LV_USE_IMG
		case LV_OBJ_IMG:return lv_img_create(obj,NULL);
		#endif
		#if LV_USE_KEYBOARD
		case LV_OBJ_KEYBOARD:return lv_keyboard_create(obj,NULL);
		#endif
		#if LV_USE_LABEL
		case LV_OBJ_LABEL:return lv_label_create(obj,NULL);
		#endif
		#if LV_USE_LED
		case LV_OBJ_LED:return lv_led_create(obj,NULL);
		#endif
		#if LV_USE_LINE
		case LV_OBJ_LINE:return lv_line_create(obj,NULL);
		#endif
		#if LV_USE_LINEMETER
		case LV_OBJ_LINEMETER:return lv_linemeter_create(obj,NULL);
		#endif
		#if LV_USE_LIST
		case LV_OBJ_LIST:return lv_list_create(obj,NULL);
		#endif
		#if LV_USE_MSGBOX
		case LV_OBJ_MSGBOX:return lv_msgbox_create(obj,NULL);
		#endif
		#if LV_USE_OBJMASK
		case LV_OBJ_OBJMASK:return lv_objmask_create(obj,NULL);
		#endif
		#if LV_USE_PAGE
		case LV_OBJ_PAGE:return lv_page_create(obj,NULL);
		#endif
		#if LV_USE_ROLLER
		case LV_OBJ_ROLLER:return lv_roller_create(obj,NULL);
		#endif
		#if LV_USE_SLIDER
		case LV_OBJ_SLIDER:return lv_slider_create(obj,NULL);
		#endif
		#if LV_USE_SPINBOX
		case LV_OBJ_SPINBOX:return lv_spinbox_create(obj,NULL);
		#endif
		#if LV_USE_SPINNER
		case LV_OBJ_SPINNER:return lv_spinner_create(obj,NULL);
		#endif
		#if LV_USE_SWITCH
		case LV_OBJ_SWITCH:return lv_switch_create(obj,NULL);
		#endif
		#if LV_USE_TABLE
		case LV_OBJ_TABLE:return lv_table_create(obj,NULL);
		#endif
		#if LV_USE_TABVIEW
		case LV_OBJ_TABVIEW:return lv_tabview_create(obj,NULL);
		#endif
		#if LV_USE_TEXTAREA
		case LV_OBJ_TEXTAREA:return lv_textarea_create(obj,NULL);
		#endif
		#if LV_USE_TILEVIEW
		case LV_OBJ_TILEVIEW:return lv_tileview_create(obj,NULL);
		#endif
		#if LV_USE_WIN
		case LV_OBJ_WIN:return lv_win_create(obj,NULL);
		#endif
		default:return NULL;
	}
}

int lua_lvgl_obj_tostring(lua_State*L){
	const char*type=NULL;
	struct lua_lvgl_obj_data*o=luaL_checkudata(L,1,LUA_LVGL_OBJ);
	if(o->type>LV_OBJ_NONE&&o->type<LV_OBJ_LAST)type=types[o->type];
	if(!type)type="(unknown)";
	lua_pushfstring(L,"LittleVGL %s object (%p)",type,o->obj);
	return 1;
}

#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "backlight"

static lv_obj_t*box,*value,*slider,*arr_left,*arr_right;

static void backlight_change(lv_event_t*e __attribute__((unused))){
	int16_t val=lv_slider_get_value(slider);
	if(val<=0)val=1;
	lv_label_set_text_fmt(value,"%d%%",val);
	guidrv_set_brightness(val);
}

static void backlight_click(lv_event_t*e){
	int16_t val=lv_slider_get_value(slider);
	if(e->target==arr_left)val--;
	else if(e->target==arr_right)val++;
	else return;
	if(val<=1)val=1;
	if(val>=100)val=100;
	lv_label_set_text_fmt(value,"%d%%",val);
	lv_slider_set_value(slider,val,LV_ANIM_ON);
	guidrv_set_brightness(val);
}


static int backlight_menu_init(struct gui_activity*act __attribute__((unused))){
	int level=guidrv_get_brightness();
	if(level<0){
		msgbox_alert("get brightness failed: %m");
		return -1;
	}
	return 0;
}

static int backlight_menu_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	int level=guidrv_get_brightness();

	box=lv_obj_create(act->page);
	lv_obj_set_flex_flow(box,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(box,gui_font_size/2,0);
	lv_obj_set_style_pad_row(box,gui_font_size/2,0);
	lv_obj_set_style_max_width(box,lv_pct(80),0);
	lv_obj_set_style_max_height(box,lv_pct(80),0);
	lv_obj_set_style_min_width(box,gui_dpi*2,0);
	lv_obj_set_style_min_height(box,gui_font_size*2,0);
	lv_obj_set_height(box,LV_SIZE_CONTENT);
	lv_obj_center(box);

	lv_obj_t*txt=lv_label_create(box);
	lv_label_set_text(txt,_("Change backlight level"));
	lv_label_set_long_mode(txt,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(txt,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(txt,lv_pct(100));

	value=lv_label_create(box);
	lv_label_set_text_fmt(value,"%d%%",level);
	lv_obj_set_style_text_align(value,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(value,lv_pct(100));

	lv_obj_t*fields=lv_obj_create(box);
	lv_obj_set_style_radius(fields,0,0);
	lv_obj_set_style_border_width(fields,0,0);
	lv_obj_set_style_bg_opa(fields,LV_OPA_0,0);

	lv_obj_set_style_pad_column(fields,gui_font_size,0);
	lv_obj_set_size(fields,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_grid_dsc_array(fields,grid_col,grid_row);

	arr_left=lv_btn_create(fields);
	lv_obj_add_event_cb(arr_left,backlight_click,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(arr_left,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_arr_left=lv_label_create(arr_left);
	lv_label_set_text(lbl_arr_left,LV_SYMBOL_LEFT);
	lv_obj_center(lbl_arr_left);

	slider=lv_slider_create(fields);
	lv_slider_set_range(slider,0,100);
	lv_slider_set_value(slider,level,LV_ANIM_OFF);
	lv_obj_add_event_cb(slider,backlight_change,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_set_grid_cell(slider,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,0,1);

	arr_right=lv_btn_create(fields);
	lv_obj_add_event_cb(arr_right,backlight_click,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(arr_right,LV_GRID_ALIGN_END,2,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_arr_right=lv_label_create(arr_right);
	lv_label_set_text(lbl_arr_right,LV_SYMBOL_RIGHT);
	lv_obj_center(lbl_arr_right);

	return 0;
}

static int backlight_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,slider);
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(slider);
	return 0;
}

static int backlight_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(slider);
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(arr_right);
	return 0;
}

struct gui_register guireg_backlight={
	.name="backlight-menu",
	.title="Backlight",
	.icon="backlight.svg",
	.show_app=true,
	.init=backlight_menu_init,
	.draw=backlight_menu_draw,
	.lost_focus=backlight_lost_focus,
	.get_focus=backlight_get_focus,
	.back=true,
	.mask=true,
};
#endif

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
#include"gui/tools.h"
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

	box=lv_draw_dialog_box(act->page,NULL,"Change backlight level");

	value=lv_label_create(box);
	lv_label_set_text_fmt(value,"%d%%",level);
	lv_obj_set_style_text_align(value,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(value,lv_pct(100));

	lv_obj_t*fields=lv_draw_line_wrapper(box,grid_col,grid_row);
	lv_obj_set_style_pad_column(fields,gui_font_size,0);

	slider=lv_slider_create(fields);
	lv_slider_set_range(slider,0,100);
	lv_slider_set_value(slider,level,LV_ANIM_OFF);
	lv_obj_add_event_cb(slider,backlight_change,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_set_grid_cell(slider,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,0,1);

	lv_draw_buttons_arg(
		fields,
		#define BTN(tgt,title,x)&(struct button_dsc){\
			&tgt,true,title,backlight_click,NULL,x,1,0,1,NULL\
		}
		BTN(arr_left,  LV_SYMBOL_LEFT,  0),
		BTN(arr_right, LV_SYMBOL_RIGHT, 2),
		#undef BTN
		NULL
	);

	return 0;
}

static int backlight_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,slider);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(slider);
	return 0;
}

static int backlight_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(slider);
	lv_group_remove_obj(arr_right);
	return 0;
}

struct gui_register guireg_backlight={
	.name="backlight-menu",
	.title="Backlight",
	.show_app=true,
	.init=backlight_menu_init,
	.draw=backlight_menu_draw,
	.lost_focus=backlight_lost_focus,
	.get_focus=backlight_get_focus,
	.back=true,
	.mask=true,
};
#endif

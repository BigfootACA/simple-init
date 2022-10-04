/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/tools.h"
#include"gui/guidrv.h"
#include"gui/activity.h"
#define TAG "mouse"

extern int gui_mouse_scale;
static lv_obj_t*box,*value,*slider,*arr_left,*arr_right;

static void mouse_scale_change(lv_event_t*e){
	gui_mouse_scale=lv_slider_get_value(slider);
	if(gui_mouse_scale<=0)gui_mouse_scale=1;
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	confd_set_integer("gui.mouse_scale",gui_mouse_scale);
}

static void mouse_scale_click(lv_event_t*e){
	gui_mouse_scale=lv_slider_get_value(slider);
	if(e->target==arr_left)gui_mouse_scale--;
	else if(e->target==arr_right)gui_mouse_scale++;
	else return;
	if(gui_mouse_scale<=0)gui_mouse_scale=1;
	if(gui_mouse_scale>=64)gui_mouse_scale=64;
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	lv_slider_set_value(slider,gui_mouse_scale,LV_ANIM_ON);
	confd_set_integer("gui.mouse_scale",gui_mouse_scale);
}

static int mouse_menu_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};

	box=lv_draw_dialog_box(act->page,NULL,"Change mouse scale level");

	value=lv_label_create(box);
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	lv_obj_set_style_text_align(value,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(value,lv_pct(100));

	lv_obj_t*fields=lv_draw_line_wrapper(box,grid_col,grid_row);
	lv_obj_set_style_pad_column(fields,gui_font_size,0);

	slider=lv_slider_create(fields);
	lv_slider_set_range(slider,1,64);
	lv_slider_set_value(slider,gui_mouse_scale,LV_ANIM_OFF);
	lv_obj_add_event_cb(slider,mouse_scale_change,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_set_grid_cell(slider,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,0,1);

	lv_draw_buttons_arg(
		fields,
		#define BTN(tgt,title,x)&(struct button_dsc){\
			&tgt,true,title,mouse_scale_click,NULL,x,1,0,1,NULL\
		}
		BTN(arr_left,  LV_SYMBOL_LEFT,  0),
		BTN(arr_right, LV_SYMBOL_RIGHT, 2),
		#undef BTN
		NULL
	);

	return 0;
}

static int mouse_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,slider);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(slider);
	return 0;
}

static int mouse_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(slider);
	lv_group_remove_obj(arr_right);
	return 0;
}

struct gui_register guireg_mouse_menu={
	.name="mouse-tools-menu",
	.title="Mouse Tools",
	.show_app=true,
	.draw=mouse_menu_draw,
	.lost_focus=mouse_lost_focus,
	.get_focus=mouse_get_focus,
	.back=true,
	.mask=true,
};
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _TOOLS_H
#define _TOOLS_H
#include"gui.h"
struct button_dsc{
	lv_obj_t**tgt;
	bool enabled;
	const char*title;
	lv_event_cb_t cb;
	void*user_data;
	uint8_t col_pos,col_span;
	uint8_t row_pos,row_span;
	lv_style_t*style;
};
extern const char*lv_event_string[];
extern lv_style_t*lv_style_opa_mask(void);
extern void lv_style_set_btn_item(lv_obj_t*btn);
extern void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_enabled(lv_obj_t*obj,bool enabled);
extern bool lv_obj_is_checked(lv_obj_t*obj);
extern void lv_obj_set_checked(lv_obj_t*obj,bool checked);
extern void lv_obj_set_hidden(lv_obj_t*obj,bool hidden);
extern void lv_default_dropdown_cb(lv_event_t*e);
extern void lv_textarea_remove_text(lv_obj_t*ta,uint32_t start,uint32_t len);
extern void lv_img_fill_image(lv_obj_t*img,lv_coord_t w,lv_coord_t h);
extern void lv_drag_border(lv_obj_t*obj,lv_coord_t width,lv_coord_t height,lv_coord_t border);
extern lv_obj_t*lv_draw_side_clear_btn(lv_obj_t*box,lv_obj_t*txt,lv_coord_t size);
extern lv_obj_t*lv_draw_input(lv_obj_t*box,const char*title,lv_obj_t**chk,lv_obj_t**clr,lv_obj_t**txt,lv_obj_t**btn);
extern lv_obj_t*lv_draw_button(lv_obj_t*parent,const char*title,bool enabled,lv_event_cb_t cb,void*user_data);
extern lv_obj_t*lv_draw_button_grid(
	lv_obj_t*parent,const char*title,
	bool enabled,lv_event_cb_t cb,void*user_data,
	uint8_t col_pos,uint8_t col_span,
	uint8_t row_pos,uint8_t row_span
);
extern lv_obj_t*lv_draw_buttons_grid(lv_obj_t*parent,lv_coord_t*grid_col,lv_coord_t*grid_row,struct button_dsc**btns);
extern lv_obj_t*lv_draw_buttons_grid_va(lv_obj_t*parent,lv_coord_t*grid_col,lv_coord_t*grid_row,va_list va);
extern lv_obj_t*lv_draw_buttons_grid_arg(lv_obj_t*parent,lv_coord_t*grid_col,lv_coord_t*grid_row,...);
extern lv_obj_t*lv_draw_buttons_auto(lv_obj_t*parent,struct button_dsc**btns);
extern lv_obj_t*lv_draw_buttons_auto_va(lv_obj_t*parent,va_list va);
extern lv_obj_t*lv_draw_buttons_auto_arg(lv_obj_t*parent,...);
extern void lv_draw_buttons(lv_obj_t*parent,struct button_dsc**btns);
extern void lv_draw_buttons_va(lv_obj_t*parent,va_list va);
extern void lv_draw_buttons_arg(lv_obj_t*parent,...);
extern lv_obj_t*lv_draw_checkbox(
	lv_obj_t*parent,const char*title,
	bool checked,lv_event_cb_t cb,void*user_data
);
extern lv_obj_t*lv_draw_checkbox_grid(
	lv_obj_t*parent,const char*title,
	bool checked,lv_event_cb_t cb,void*user_data,
	lv_grid_align_t x_align,uint8_t col_pos,uint8_t col_span,
	lv_grid_align_t y_align,uint8_t row_pos,uint8_t row_span
);
extern lv_obj_t*lv_draw_dropdown(lv_obj_t*parent,char*title,lv_obj_t**dd);
extern lv_obj_t*lv_draw_dialog_box(lv_obj_t*parent,lv_obj_t**lbl,const char*title);
extern lv_obj_t*lv_draw_wrapper(
	lv_obj_t*parent,
	lv_coord_t*grid_col,lv_coord_t*grid_row,
	lv_coord_t width,lv_coord_t height
);
extern lv_obj_t*lv_draw_btns_ok_cancel(
	lv_obj_t*parent,
	lv_obj_t**btn_ok,lv_obj_t**btn_cancel,
	lv_event_cb_t ok_cb,void*user_data
);
extern void lv_input_cb(lv_event_t*e);
extern bool lv_parse_color_string(const char*val,lv_color_t*color);
extern void lv_obj_add_drag(lv_obj_t*o);
extern void lv_event_dump_handler(lv_event_t*e);
extern void lv_img_src_try(lv_obj_t*img,const char*prefix,const char*id,const char*path);
static inline lv_coord_t lv_coord_max(lv_coord_t a,lv_coord_t b){return a>b?a:b;}
static inline lv_coord_t lv_coord_min(lv_coord_t a,lv_coord_t b){return a<b?a:b;}
static inline lv_coord_t lv_coord_border(lv_coord_t val,lv_coord_t max,lv_coord_t min){
	return lv_coord_max(min,lv_coord_min(max,val));
}
#define lv_draw_line_wrapper(parent,grid_col,grid_row) lv_draw_wrapper(parent,grid_col,grid_row,lv_pct(100),LV_SIZE_CONTENT)
#endif

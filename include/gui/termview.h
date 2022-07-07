/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef LV_TERMVIEW_H
#define LV_TERMVIEW_H
#ifdef ENABLE_GUI
#ifdef ENABLE_LIBTSM
#ifdef __cplusplus
extern "C" {
#endif
#include"lvgl.h"
#include"libtsm.h"
#if LV_USE_CANVAS==0
#error "lv_termview: lv_canvas is required. Enable it in lv_conf.h (LV_USE_CANVAS 1)"
#endif
typedef void(*termview_write_cb)(lv_obj_t*tv,const char *u8,size_t len);
typedef void(*termview_osc_cb)(lv_obj_t*tv,const char *u8,size_t len);
typedef void(*termview_resize_cb)(lv_obj_t*tv,uint32_t cols,uint32_t rows);
typedef struct{
	lv_canvas_t canvas;
	lv_coord_t glyph_height;
	lv_coord_t glyph_width;
	lv_coord_t width;
	lv_coord_t height;
	lv_coord_t drag_y_last;
	uint32_t cols;
	uint32_t rows;
	uint32_t mods;
	uint32_t max_sb;
	bool cust_font;
	tsm_age_t age;
	size_t mem_size;
	lv_color_t*buffer;
	lv_obj_t*virt_input;
	struct tsm_screen*screen;
	struct tsm_vte*vte;
	const lv_font_t*font_reg;
	const lv_font_t*font_bold;
	const lv_font_t*font_ital;
	const lv_font_t*font_bold_ital;
	termview_osc_cb osc_cb;
	termview_write_cb write_cb;
	termview_resize_cb resize_cb;
}lv_termview_t;

extern void lv_termview_set_mods(lv_obj_t*tv,uint32_t mods);
extern void lv_termview_set_max_scroll_buffer_size(lv_obj_t*tv,uint32_t max_sb);
extern void lv_termview_set_osc_cb(lv_obj_t*tv,termview_osc_cb cb);
extern void lv_termview_set_write_cb(lv_obj_t*tv,termview_write_cb cb);
extern void lv_termview_set_resize_cb(lv_obj_t*tv,termview_resize_cb cb);
extern void lv_termview_update(lv_obj_t*tv);
extern void lv_termview_set_font(lv_obj_t*tv,const lv_font_t*font);
extern void lv_termview_set_font_regular(lv_obj_t*tv,lv_font_t*font);
extern void lv_termview_set_font_bold(lv_obj_t*tv,lv_font_t*font);
extern void lv_termview_set_font_italic(lv_obj_t*tv,lv_font_t*font);
extern void lv_termview_set_font_bold_italic(lv_obj_t*tv,lv_font_t*font);
extern void lv_termview_resize(lv_obj_t*tv);
extern uint32_t lv_termview_get_cols(lv_obj_t*tv);
extern uint32_t lv_termview_get_rows(lv_obj_t*tv);
extern void lv_termview_input(lv_obj_t*tv,const char*str);
extern void lv_termview_input_fmt(lv_obj_t*tv,const char*fmt,...);
extern void lv_termview_line_printf(lv_obj_t*tv,const char*fmt,...);
extern struct tsm_screen*lv_termview_get_screen(lv_obj_t*tv);
extern struct tsm_vte*lv_termview_get_vte(lv_obj_t*tv);
extern lv_obj_t*lv_termview_get_virtual_input(lv_obj_t*tv);
extern lv_obj_t*lv_termview_create(lv_obj_t*par);
#ifdef __cplusplus
}
#endif
#endif
#endif
#endif

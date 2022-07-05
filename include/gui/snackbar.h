/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _SNACKBAR_H
#define _SNACKBAR_H
#include"gui.h"
extern void snackbar_show(void);
extern void snackbar_hide(void);
extern void snackbar_set_text(const char*text);
extern void snackbar_set_text_fmt(const char*fmt,...);
extern void snackbar_show_text(const char*text);
extern void snackbar_show_text_fmt(const char*fmt,...);
extern void snackbar_set_on_click(runnable_t cb,void*data);
extern void snackbar_set_on_dismiss(runnable_t cb,void*data);
extern void snackbar_set_show_time(uint32_t time);
extern void snackbar_set_anim_time(uint32_t time);
extern void snackbar_set_font(const lv_font_t*font);
extern void snackbar_set_height(lv_coord_t height);
extern void snackbar_set_color(lv_color_t bg_color,lv_color_t txt_color);
extern void snackbar_draw(lv_obj_t*scr,lv_coord_t off,lv_coord_t height);
#endif

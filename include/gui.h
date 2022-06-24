/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef GUI_H
#define GUI_H
#include"lvgl.h"
#include"lock.h"
#include"defines.h"
#ifdef ENABLE_LUA
#include"xlua.h"
extern lua_State*gui_global_lua;
#endif
extern mutex_t gui_lock;
typedef void (*draw_func)(lv_obj_t*);
extern int gui_dpi_def,gui_dpi_force;
extern int gui_font_size;
extern int default_backlight;
extern lv_coord_t gui_w,gui_h;
extern lv_coord_t gui_sw,gui_sh,gui_sx,gui_sy;
extern uint16_t gui_rotate;
extern const lv_font_t*gui_font;
extern const lv_font_t*gui_font_small;
extern lv_group_t*gui_grp;
extern lv_obj_t*gui_cursor;
extern bool gui_sleep,gui_run,gui_dark;
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int gui_pre_init(void);
extern int gui_screen_init(void);
extern int gui_init(void);
extern int gui_main(void);
extern int gui_draw(void);
extern void gui_quit_sleep(void);
extern void gui_do_quit(void);
extern void gui_set_run_exit(runnable_t*run);
extern void gui_run_and_exit(runnable_t*run);
extern uint32_t custom_tick_get(void);
extern int register_guiapp(void);
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _SPLASH_H
#define _SPLASH_H
#include"gui.h"
extern int gui_splash_draw();
extern void gui_splash_exit(bool out,lv_async_cb_t after_exit);
extern void gui_splash_set_text(bool out,const char*fmt,...);
#endif

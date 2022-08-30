/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _GUIDRV_H
#define _GUIDRV_H
#include<stdint.h>
#include<stdbool.h>
#include"gui.h"
#include"src/draw/sw/lv_draw_sw.h"

struct display_mode{
	char name[24];
	uint32_t width;
	uint32_t height;
};

struct gui_driver{
	char name[32];
	int(*drv_register)(void);
	void(*drv_getsize)(lv_coord_t*w,lv_coord_t*h);
	void(*drv_getdpi)(int*dpi);
	void(*drv_taskhandler)(void);
	void(*drv_exit)(void);
	uint32_t(*drv_tickget)(void);
	void(*drv_setbrightness)(int);
	int(*drv_getbrightness)(void);
	bool(*drv_cansleep)(void);
	int(*drv_get_modes)(int*,struct display_mode**);
};

struct input_driver{
	char name[32];
	char*compatible[32];
	int(*drv_register)(void);
	void(*drv_exit)(void);
};

// src/gui/drivers.c: builtin display modes
extern struct display_mode builtin_modes[];

// src/gui/drivers.c: initial gui drivers storage
extern struct gui_driver*gui_drvs[];

// src/gui/drivers.c: initial input drivers storage
extern struct input_driver*input_drvs[];

// src/gui/guidrv.c: get screen width and height from driver
extern int guidrv_getsize(lv_coord_t*w,lv_coord_t*h);

// src/gui/guidrv.c: get screen dpi from driver
extern int guidrv_getdpi(int*dpi);

// src/gui/guidrv.c: get screen width from driver
extern lv_coord_t guidrv_get_width(void);

// src/gui/guidrv.c: get screen height from driver
extern lv_coord_t guidrv_get_height(void);

// src/gui/guidrv.c: get screen dpi from driver
extern int guidrv_get_dpi(void);

// src/gui/guidrv.c: get driver name
extern const char*guidrv_getname(void);

// src/gui/guidrv.c: check driver name
extern bool guidrv_isname(const char*name);

// src/gui/guidrv.c: gui driver custom lvgl taskhandler
extern int guidrv_taskhandler(void);

// src/gui/guidrv.c: gui driver custom lvgl tickget
extern uint32_t guidrv_tickget(void);

// src/gui/guidrv.c: register current gui driver
extern int guidrv_register(void);

// src/gui/guidrv.c: call gui driver set brightness percent (0-100)
extern int guidrv_set_brightness(int percent);

// src/gui/guidrv.c: call gui driver get brightness percent (0-100)
extern int guidrv_get_brightness(void);

// src/gui/guidrv.c: get gui driver is screen suspendable
extern bool guidrv_can_sleep(void);

// src/gui/guidrv.c: get gui driver display modes
extern int guidrv_get_modes(int*cnt,struct display_mode**modes);

// src/gui/guidrv.c: try init all drivers and return screen width height dpi
extern int guidrv_init(lv_coord_t*w,lv_coord_t*h,int*dpi);

// src/gui/guidrv.c: get gui driver by name
extern struct gui_driver*guidrv_get_by_name(const char*name);

// src/gui/guidrv.c: call gui driver exit handler
extern void guidrv_exit(void);

// src/gui/guidrv.c: set current gui driver
extern void guidrv_set_driver(struct gui_driver*driver);

// src/gui/guidrv.c: get current gui driver
extern struct gui_driver*guidrv_get_driver(void);

// src/gui/guidrv.c: init input drivers
extern int indrv_init(void);
#endif

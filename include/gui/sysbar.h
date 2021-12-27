/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _SYSBAR_H
#define _SYSBAR_H
#include"gui.h"
struct sysbar{
	int size;
	lv_obj_t*screen;
	lv_obj_t*content;
	struct{
		lv_obj_t*bar;
		struct{
			lv_obj_t*time;
			#ifndef ENABLE_UEFI
			lv_obj_t*level;
			lv_obj_t*battery;
			#endif
		}content;
	}top;
	lv_task_t*hide;
	lv_obj_t*keyboard;
	lv_obj_t*edit_menu;
	lv_obj_t*edit_btns[10];
	lv_obj_t*focus_input;
	lv_obj_t*bar_btn;
	bool full_screen;
	struct{
		lv_obj_t*bar;
		lv_style_t btn_style;
		bool style_inited;
		struct{
			lv_obj_t*back;
			lv_obj_t*home;
			lv_obj_t*keyboard;
			lv_obj_t*power;
		}content;
	}bottom;
};
extern struct sysbar sysbar;

// src/gui/interface/core/sysbar.c: init sysbar in screen
extern int sysbar_draw(lv_obj_t*scr);

// src/gui/interface/core/sysbar.c: set sysbar full screen status
extern void sysbar_set_full_screen(bool fs);

// src/gui/interface/core/sysbar.c: show top bar and bottom bar in full screen
extern void sysbar_show_bar(void);

// src/gui/interface/core/sysbar.c: hide full screen sysbar toggle button
extern void sysbar_hide_full_screen_btn(void);

// src/gui/interface/core/sysbar.c: toggle keyboard status
extern void sysbar_keyboard_toggle(void);

// src/gui/interface/core/sysbar.c: close keyboard
extern void sysbar_keyboard_close(void);

// src/gui/interface/core/sysbar.c: open keyboard
extern void sysbar_keyboard_open(void);

// src/gui/interface/core/sysbar.c: focus textarea
extern void sysbar_focus_input(lv_obj_t*obj);
#endif

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
	lv_timer_t*hide;
	lv_obj_t*keyboard;
	lv_obj_t*edit_menu;
	lv_obj_t*edit_btns[11];
	lv_obj_t*focus_input;
	lv_obj_t*bar_btn;
	bool full_screen;
	struct{
		lv_obj_t*bar;
		lv_style_t btn_style;
		bool style_inited;
		lv_obj_t*buttons;
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

// src/gui/interface/core/sysbar.c: show edit menu
extern void sysbar_edit_menu_show(void);

// src/gui/interface/core/sysbar.c: hide edit menu
extern void sysbar_edit_menu_hide(void);

// src/gui/interface/core/sysbar.c: focus textarea
extern void sysbar_focus_input(lv_obj_t*obj);

// src/gui/interface/core/ctrl_pad.c: draw control pad body
extern void ctrl_pad_draw(void);

// src/gui/interface/core/ctrl_pad.c: show control pad
extern void ctrl_pad_show(void);

// src/gui/interface/core/ctrl_pad.c: hide control pad
extern void ctrl_pad_hide(void);

// src/gui/interface/core/ctrl_pad.c: is control pad show
extern bool ctrl_pad_is_show(void);

// src/gui/interface/core/ctrl_pad.c: toggle control pad show
extern void ctrl_pad_toggle(void);

// src/gui/interface/core/ctrl_pad.c: set control pad target object
extern void ctrl_pad_set_target(lv_obj_t*target);
#endif

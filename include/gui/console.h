/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef CONSOLE_H
#define CONSOLE_H
#include"gui/activity.h"

enum alt_pad_btn_type{
	ALT_PAD_EMPTY,
	ALT_PAD_ACT_CLOSE,
	ALT_PAD_ACT_COPY,
	ALT_PAD_ACT_MOVE,
	ALT_PAD_ACT_SCR_UP,
	ALT_PAD_ACT_SCR_DOWN,
	ALT_PAD_ACT_RESET,
	ALT_PAD_KEY_TAB,
	ALT_PAD_KEY_ESC,
	ALT_PAD_KEY_UP,
	ALT_PAD_KEY_DOWN,
	ALT_PAD_KEY_RIGHT,
	ALT_PAD_KEY_LEFT,
	ALT_PAD_KEY_PGUP,
	ALT_PAD_KEY_PGDN,
	ALT_PAD_KEY_ENTER,
	ALT_PAD_KEY_CAPS,
	ALT_PAD_KEY_SUPER,
	ALT_PAD_KEY_SHIFT,
	ALT_PAD_KEY_HOME,
	ALT_PAD_KEY_END,
	ALT_PAD_KEY_CTRL,
	ALT_PAD_KEY_ALT,
	ALT_PAD_KEY_BACKSPACE,
	ALT_PAD_KEY_DEL,
	ALT_PAD_KEY_F1,
	ALT_PAD_KEY_F2,
	ALT_PAD_KEY_F3,
	ALT_PAD_KEY_F4,
	ALT_PAD_KEY_F5,
	ALT_PAD_KEY_F6,
	ALT_PAD_KEY_F7,
	ALT_PAD_KEY_F8,
	ALT_PAD_KEY_F9,
	ALT_PAD_KEY_F10,
	ALT_PAD_KEY_F11,
	ALT_PAD_KEY_F12,
	ALT_PAD_CNT
};

struct alt_pad_btn{
	struct console*con;
	struct alt_pad_btn_map*map;
	lv_obj_t*btn;
	lv_obj_t*lbl;
};

struct alt_pad_btn_map{
	const char*title;
	enum alt_pad_btn_type type;
	bool hold;
	void(*callback)(struct alt_pad_btn*);
	uint32_t attr;
};

struct console{
	struct gui_activity*act;
	lv_obj_t*termview;
	lv_obj_t*alt_btns;
	lv_obj_t*toggle_btn;
	struct alt_pad_btn btns[6][6];
	uint32_t mods;
	bool allow_exit;
	void*data;
};

extern int console_init(struct gui_activity*act);
extern int console_draw(struct gui_activity*act);
extern int console_resize(struct gui_activity*d);
extern int console_get_focus(struct gui_activity*d);
extern int console_lost_focus(struct gui_activity*d);
extern int console_clean(struct gui_activity*d);
extern int console_do_back(struct gui_activity*act);
#endif

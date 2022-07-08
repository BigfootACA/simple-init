/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _INPUTBOX_H
#define _INPUTBOX_H
#include"gui.h"
struct inputbox;
typedef bool(*inputbox_callback)(bool ok,const char*content,void*user_data);

// src/gui/interface/widgets/inputbox.c: create a dialog to input string
extern struct inputbox*inputbox_create(inputbox_callback callback,const char*title,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/inputbox.c: set inputbox textarea initial content
extern void inputbox_set_content(struct inputbox*input,const char*content,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/inputbox.c: set inputbox textarea holder string
extern void inputbox_set_holder(struct inputbox*input,const char*holder,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/inputbox.c: set inputbox title
extern void inputbox_set_title(struct inputbox*input,const char*title,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/inputbox.c: set inputbox textarea accept chars
extern void inputbox_set_accept(struct inputbox*input,const char*accept);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea is one line
extern void inputbox_set_one_line(struct inputbox*input,bool one_line);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea allow text select
extern void inputbox_set_text_select(struct inputbox*input,bool sel);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea password mode
extern void inputbox_set_pwd_mode(struct inputbox*input,bool pwd);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea text align
extern void inputbox_set_input_align(struct inputbox*input,lv_text_align_t align);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea max length
extern void inputbox_set_max_length(struct inputbox*input,uint32_t max);

// src/gui/interface/widgets/inputbox.c: set inputbox textarea input event callback
extern void inputbox_set_input_event_cb(struct inputbox*input,lv_event_cb_t cb);

// src/gui/interface/widgets/inputbox.c: set inputbox callback
extern void inputbox_set_callback(struct inputbox*input,inputbox_callback cb);

// src/gui/interface/widgets/inputbox.c: set inputbox user data
extern void inputbox_set_user_data(struct inputbox*input,void*user_data);
#endif

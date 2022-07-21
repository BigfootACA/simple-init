/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _WEB_GUI_RENDER_H
#define _WEB_GUI_RENDER_H
#include<stdbool.h>
#include<sys/types.h>
#include<emscripten.h>
extern EMSCRIPTEN_KEEPALIVE int web_gui_start(const char*url);
extern EMSCRIPTEN_KEEPALIVE size_t web_gui_get_bytes();
extern EMSCRIPTEN_KEEPALIVE size_t web_gui_get_frames();
extern EMSCRIPTEN_KEEPALIVE void web_gui_set_paused(bool paused);
extern EMSCRIPTEN_KEEPALIVE void web_gui_set_video_paused(bool paused);
extern EMSCRIPTEN_KEEPALIVE void web_gui_set_input_paused(bool paused);
extern EMSCRIPTEN_KEEPALIVE void web_gui_set_compressed(bool compressed);
extern EMSCRIPTEN_KEEPALIVE void web_gui_disconnect();
extern EMSCRIPTEN_KEEPALIVE void web_gui_refresh();
extern EMSCRIPTEN_KEEPALIVE void web_gui_initialize();
#endif

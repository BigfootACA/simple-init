/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _RECOVERY_H
#define _RECOVERY_H
extern int recovery_out_fd;
extern void recovery_ui_print(const char*str);
extern void recovery_progress(const float frac,const int sec);
extern void recovery_set_progress(const float frac);
extern void recovery_log(const char*str);
extern void recovery_clear_display();
extern void recovery_ui_printf(const char*fmt,...);
extern void recovery_logf(const char*fmt,...);
#endif

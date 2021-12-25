/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _CLIPBOARD_H
#define _CLIPBOARD_H

enum clipboard_type{
	CLIP_NULL,
	CLIP_TEXT,
	CLIP_FILE,
};

extern void clipboard_reset(void);
extern void clipboard_clear(void);
extern void clipboard_init(void);
extern int clipboard_set(enum clipboard_type type,const char*content,size_t len);
extern enum clipboard_type clipboard_get_type(void);
extern char*clipboard_get_content(void);

#endif

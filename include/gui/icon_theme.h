/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _ICON_THEME_H
#define _ICON_THEME_H
#include"list.h"
#include"regexp.h"
#include"filesystem.h"
#define ICON_THEME_COMPATIBLE_LEVEL 0x00000001

typedef struct icon_theme_search_path{
	fsh*folder;
	char*path;
	char*id;
}icon_theme_search_path;

typedef struct icon_theme_name_mapping{
	char*name;
	Reprog*regex;
	char*type;
	char*path;
	char*search;
}icon_theme_name_mapping;

typedef struct icon_theme{
	char zid[32];
	fsh*root;
	char id[32];
	char name[128];
	char icon[32];
	char version[32];
	char author[64];
	char desc[512];
	bool zip;
	list*search_path;
	list*name_mapping;
}icon_theme;

extern list*gui_icon_themes;
extern bool icon_theme_load_fsh(fsh*f);
extern bool icon_theme_load_url(url*u);
extern bool icon_theme_load(const char*path);
extern void icon_theme_load_from_confd();
#endif

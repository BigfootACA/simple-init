/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef LANGUAGE_H
#define LANGUAGE_H
#include<stdbool.h>
#include"defines.h"

struct language{
	char*lang;
	char*region;
	char*charset;
	char*name;
};

extern struct language languages[];

// src/lib/locale.c: init i18n locale
extern void lang_init_locale(void);

// src/lib/locale.c: load new locale
extern void lang_load_locale(const char*dir,const char*lang,const char*domain);

// src/lib/locale.c: get current locale
extern char*lang_get_locale(char*def);

// src/lib/locale.c: gettext implementation
extern char*lang_gettext(const char*msgid) __fa(1);

// src/lib/locale.c: concat language name
extern const char*lang_concat(struct language*lang,bool region,bool charset);

// src/lib/locale.c: compare language name
extern bool lang_compare(struct language*lang,const char*name);

// src/lib/locale.c: set current language
extern int lang_set(const char*lang);

#ifndef _
#define _ lang_gettext
#endif

#endif

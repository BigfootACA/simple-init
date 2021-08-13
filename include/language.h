#ifndef LANGUAGE_H
#define LANGUAGE_H

struct language{
	char*lang;
	char*region;
	char*charset;
	char*name;
};

extern struct language languages[];

// src/lib/locale.c: init i18n locale
extern void lang_init_locale();

// src/lib/locale.c: load new locale
extern void lang_load_locale(const char*dir,const char*lang,const char*domain);

// src/lib/locale.c: get current locale
extern char*lang_get_locale(char*def);

// src/lib/locale.c: gettext implementation
extern char*lang_gettext(const char*msgid) __attribute_format_arg__(1);

// src/lib/locale.c: concat language name
extern const char*lang_concat(struct language*lang,bool region,bool charset);

#ifndef _
#define _ lang_gettext
#endif

#endif

#ifndef	_STRINGS_H
#define	_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "compatible.h"

#define __NEED_size_t
#define __NEED_locale_t
#include <bits/alltypes.h>

int bcmp (const void *, const void *, size_t);
void bcopy (const void *, void *, size_t);
void bzero (void *, size_t);
char *index (const char *, int);
char *rindex (const char *, int);

int ffs (int);
int ffsl (long);
int ffsll (long long);

int strcasecmp (const char *, const char *);
int strncasecmp (const char *, const char *, size_t);

int strcasecmp_l (const char *, const char *, locale_t);
int strncasecmp_l (const char *, const char *, size_t, locale_t);

#ifdef __cplusplus
}
#endif

#endif

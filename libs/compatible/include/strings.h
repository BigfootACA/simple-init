#ifndef	_STRINGS_H
#define	_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif


#define __NEED_size_t
#define __NEED_locale_t
#include <bits/alltypes.h>

#define bcmp comp_bcmp
#define bcopy comp_bcopy
#define bzero comp_bzero
#define index comp_index
#define rindex comp_rindex
#define ffs comp_ffs
#define ffsl comp_ffsl
#define ffsll comp_ffsll
#define strcasecmp comp_strcasecmp
#define strncasecmp comp_strncasecmp
#define strcasecmp_l comp_strcasecmp_l
#define strncasecmp_l comp_strncasecmp_l
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

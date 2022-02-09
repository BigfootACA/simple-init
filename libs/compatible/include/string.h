#ifndef	_STRING_H
#define	_STRING_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stddef.h"

#define __NEED_size_t
#define __NEED_locale_t

#include <bits/alltypes.h>

#define memcpy comp_memcpy
#define memmove comp_memmove
#define memset comp_memset
#define memcmp comp_memcmp
#define memchr comp_memchr
#define strcpy comp_strcpy
#define strncpy comp_strncpy
#define strncpyX comp_strncpyX
#define strcat comp_strcat
#define strncat comp_strncat
#define strcmp comp_strcmp
#define strncmp comp_strncmp
#define strcoll comp_strcoll
#define strxfrm comp_strxfrm
#define strchr comp_strchr
#define strrchr comp_strrchr
#define strcspn comp_strcspn
#define strspn comp_strspn
#define strpbrk comp_strpbrk
#define strstr comp_strstr
#define strtok comp_strtok
#define strlen comp_strlen
#define strerror comp_strerror
#define strtok_r comp_strtok_r
#define strerror_r comp_strerror_r
#define stpcpy comp_stpcpy
#define stpncpy comp_stpncpy
#define strnlen comp_strnlen
#define strdup comp_strdup
#define strndup comp_strndup
#define strsignal comp_strsignal
#define strerror_l comp_strerror_l
#define strcoll_l comp_strcoll_l
#define strxfrm_l comp_strxfrm_l
#define memccpy comp_memccpy
#define strsep comp_strsep
#define strlcat comp_strlcat
#define strlcpy comp_strlcpy
#define explicit_bzero comp_explicit_bzero
#define strverscmp comp_strverscmp
#define strchrnul comp_strchrnul
#define strcasestr comp_strcasestr
#define memmem comp_memmem
#define memrchr comp_memrchr
#define mempcpy comp_mempcpy

void *memcpy (void *__restrict, const void *__restrict, size_t);
void *memmove (void *, const void *, size_t);
void *memset (void *, int, size_t);
int memcmp (const void *, const void *, size_t);
void *memchr (const void *, int, size_t);

char *strcpy (char *__restrict, const char *__restrict);
char *strncpy (char *__restrict, const char *__restrict, size_t);
int strncpyX(char*__restrict s1,const char*__restrict s2,size_t n);
char *strcat (char *__restrict, const char *__restrict);
char *strncat (char *__restrict, const char *__restrict, size_t);

int strcmp (const char *, const char *);
int strncmp (const char *, const char *, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *__restrict, const char *__restrict, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strstr (const char *, const char *);
char *strtok (char *__restrict, const char *__restrict);

size_t strlen (const char *);

char *strerror (int);
#include <strings.h>

char *strtok_r (char *__restrict, const char *__restrict, char **__restrict);
int strerror_r (int, char *, size_t);
char *stpcpy(char *__restrict, const char *__restrict);
char *stpncpy(char *__restrict, const char *__restrict, size_t);
size_t strnlen (const char *, size_t);
char *strdup (const char *);
char *strndup (const char *, size_t);
char *strsignal(int);
char *strerror_l (int, locale_t);
int strcoll_l (const char *, const char *, locale_t);
size_t strxfrm_l (char *__restrict, const char *__restrict, size_t, locale_t);
void *memccpy (void *__restrict, const void *__restrict, int, size_t);
char *strsep(char **, const char *);
size_t strlcat (char *, const char *, size_t);
size_t strlcpy (char *, const char *, size_t);
void explicit_bzero (void *, size_t);
#define	strdupa(x)	strcpy(alloca(strlen(x)+1),x)
int strverscmp (const char *, const char *);
char *strchrnul(const char *, int);
char *strcasestr(const char *, const char *);
void *memmem(const void *, size_t, const void *, size_t);
void *memrchr(const void *, int, size_t);
void *mempcpy(void *, const void *, size_t);
#ifndef __cplusplus
char *basename();
#endif

#ifdef __cplusplus
}
#endif

#endif

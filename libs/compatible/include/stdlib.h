#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define __NEED_size_t
#define __NEED_wchar_t

#include <bits/alltypes.h>
#define atoi comp_atoi
#define atol comp_atol
#define atoll comp_atoll
#define atof comp_atof
#define strtof comp_strtof
#define strtod comp_strtod
#define strtold comp_strtold
#define strtol comp_strtol
#define strtoul comp_strtoul
#define strtoll comp_strtoll
#define strtoull comp_strtoull
#define rand comp_rand
#define srand comp_srand
#define malloc comp_malloc
#define calloc comp_calloc
#define realloc comp_realloc
#define free comp_free
#define aligned_alloc comp_aligned_alloc
#define abort comp_abort
#define atexit comp_atexit
#define exit comp_exit
#define _Exit comp__Exit
#define at_quick_exit comp_at_quick_exit
#define quick_exit comp_quick_exit
#define getenv comp_getenv
#define system comp_system
#define bsearch comp_bsearch
#define qsort comp_qsort
#define abs comp_abs
#define labs comp_labs
#define llabs comp_llabs
#define div comp_div
#define ldiv comp_ldiv
#define lldiv comp_lldiv
#define mblen comp_mblen
#define mbtowc comp_mbtowc
#define wctomb comp_wctomb
#define mbstowcs comp_mbstowcs
#define wcstombs comp_wcstombs

int atoi (const char *);
long atol (const char *);
long long atoll (const char *);
double atof (const char *);

float strtof (const char *__restrict, char **__restrict);
double strtod (const char *__restrict, char **__restrict);
long double strtold (const char *__restrict, char **__restrict);

long strtol (const char *__restrict, char **__restrict, int);
unsigned long strtoul (const char *__restrict, char **__restrict, int);
long long strtoll (const char *__restrict, char **__restrict, int);
unsigned long long strtoull (const char *__restrict, char **__restrict, int);

int rand (void);
void srand (unsigned);

void *malloc (size_t);
void *calloc (size_t, size_t);
void *realloc (void *, size_t);
void free (void *);
void *aligned_alloc(size_t, size_t);

_Noreturn void abort (void);
int atexit (void (*) (void));
_Noreturn void exit (int);
_Noreturn void _Exit (int);
int at_quick_exit (void (*) (void));
_Noreturn void quick_exit (int);

char *getenv (const char *);

int system (const char *);

void *bsearch (const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
void qsort (void *, size_t, size_t, int (*)(const void *, const void *));

int abs (int);
long labs (long);
long long llabs (long long);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t div (int, int);
ldiv_t ldiv (long, long);
lldiv_t lldiv (long long, long long);

int mblen (const char *, size_t);
int mbtowc (wchar_t *__restrict, const char *__restrict, size_t);
int wctomb (char *, wchar_t);
size_t mbstowcs (wchar_t *__restrict, const char *__restrict, size_t);
size_t wcstombs (char *__restrict, const wchar_t *__restrict, size_t);

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define MB_CUR_MAX 4
#define RAND_MAX (0x7fffffff)

#ifdef __cplusplus
}
#endif

#endif

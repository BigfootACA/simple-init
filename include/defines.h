#ifndef _DEFINES_H
#define _DEFINES_H
#ifndef __linux__
#error "This program only run on Linux"
#endif
#include<time.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdarg.h>
#include<limits.h>
#include<unistd.h>
#include<sys/types.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 65536
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifdef O_CLOEXEC
#define UL_CLOEXECSTR   "e"
#else
#define UL_CLOEXECSTR   ""
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef O_DIR
#define O_DIR O_RDONLY|O_DIRECTORY
#endif
#ifdef assert
#error "Do not use assert.h"
#endif
#define assert(expr){if(!(expr))exit_stderr_printf(-1,"assert failed.\n");}
#define _ lang_gettext
#define MIN(a,b)((b)>(a)?(a):(b))
#define MAX(a,b)((b)<(a)?(a):(b))
#define ENUM(err) -(errno=(err))
#define ERET(err) return ENUM(err)
#define EPRET(err) {errno=(err);return NULL;}
#define IOVEC(data,len) ((struct iovec){.iov_base=(data),.iov_len=(len)})
extern char*lang_gettext(const char*msgid) __attribute_format_arg__(1);
static inline int min_int(int a,int b){return MIN(a,b);}
static inline long min_long(long a,long b){return MIN(a,b);}
static inline int max_int(int a,int b){return MAX(a,b);}
static inline long max_long(long a,long b){return MAX(a,b);}
typedef int runnable_t(void*);
extern char**environ;
#endif

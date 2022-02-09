#ifndef _SYS_TIME_H
#define _SYS_TIME_H
#ifdef __cplusplus
extern "C" {
#endif
#define __NEED_time_t
#define __NEED_suseconds_t
#define __NEED_struct_timeval
#include "bits/alltypes.h"
#include "time.h"

int gettimeofday (struct timeval *__restrict, void *__restrict);

#define ITIMER_REAL    0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF    2

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

int getitimer (int, struct itimerval *);
int setitimer (int, const struct itimerval *__restrict, struct itimerval *__restrict);
int utimes (const char *, const struct timeval [2]);

#ifdef __cplusplus
}
#endif
#endif

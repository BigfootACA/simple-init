#ifndef	_TIME_H
#define _TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "compatible.h"
#include <stddef.h>
#include <Uefi.h>

#define __NEED_size_t
#define __NEED_time_t
#define __NEED_clock_t
#define __NEED_struct_timespec
#define __NEED_clockid_t
#define __NEED_timer_t
#define __NEED_pid_t
#define __NEED_locale_t
#include <bits/alltypes.h>

#define __tm_gmtoff tm_gmtoff
#define __tm_zone tm_zone

struct tm {
	int     tm_year;      // years since 1900
	int     tm_mon;       // months since January  [0, 11]
	int     tm_mday;      // day of the month  [1, 31]
	int     tm_hour;      // hours since midnight  [0, 23]
	int     tm_min;       // minutes after the hour  [0, 59]
	int     tm_sec;       // seconds after the minute  [0, 60]
	int     tm_wday;      // days since Sunday  [0, 6]
	int     tm_yday;      // days since January 1  [0, 365]
	int     tm_isdst;     // Daylight Saving Time flag
	int     tm_zoneoff;   // EFI TimeZone offset, -1440 to 1440 or 2047
	int     tm_daylight;  // EFI Daylight flags
	UINT32  tm_Nano;      // EFI Nanosecond value
};

clock_t clock (void);
time_t time (time_t *);
double difftime (time_t, time_t);
time_t mktime (struct tm *);
size_t strftime (char *__restrict, size_t, const char *__restrict, const struct tm *__restrict);
struct tm *gmtime (const time_t *);
struct tm *localtime (const time_t *);
char *asctime (const struct tm *);
char *ctime (const time_t *);
int timespec_get(struct timespec *, int);

#define CLOCKS_PER_SEC 1000000L

#define TIME_UTC 1

size_t strftime_l (char *  __restrict, size_t, const char *  __restrict, const struct tm *  __restrict, locale_t);

struct tm *gmtime_r (const time_t *__restrict, struct tm *__restrict);
struct tm *localtime_r (const time_t *__restrict, struct tm *__restrict);
char *asctime_r (const struct tm *__restrict, char *__restrict);
char *ctime_r (const time_t *, char *);

void tzset (void);

struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};

#define TIMER_ABSTIME 1

int nanosleep (const struct timespec *, struct timespec *);

struct sigevent;

extern char *tzname[2];


char *strptime (const char *__restrict, const char *__restrict, struct tm *__restrict);
extern int daylight;
extern long timezone;
extern int getdate_err;
struct tm *getdate (const char *);


int stime(const time_t *);
time_t timegm(struct tm *);

#ifdef __cplusplus
}
#endif


#endif

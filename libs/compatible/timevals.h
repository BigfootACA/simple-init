/** @file
    Definitions private to the Implementation of <time.h>.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Portions derived from the NIH time zone package files,
  which contain the following notice:

    This file is in the public domain, so clarified as of
    1996-06-05 by Arthur David Olson (arthur_david_olson@nih.gov).
**/
#ifndef _TIMEVAL_H
#define _TIMEVAL_H
#include "limits.h"
#define TYPE_BIT(type)  (sizeof (type) * CHAR_BIT)
#define TYPE_SIGNED(type) (((type) -1) < 0)
#define INT_STRLEN_MAXIMUM(type) ((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + 1 + TYPE_SIGNED(type))
#define TM_YEAR_BASE  1900
#define TM_SUNDAY     0
#define TM_MONDAY     1
#define TM_TUESDAY    2
#define TM_WEDNESDAY  3
#define TM_THURSDAY   4
#define TM_FRIDAY     5
#define TM_SATURDAY   6
#define TM_JANUARY     0
#define TM_FEBRUARY    1
#define TM_MARCH       2
#define TM_APRIL       3
#define TM_MAY         4
#define TM_JUNE        5
#define TM_JULY        6
#define TM_AUGUST      7
#define TM_SEPTEMBER   8
#define TM_OCTOBER     9
#define TM_NOVEMBER   10
#define TM_DECEMBER   11
extern struct state * lclptr;
extern struct state * gmtptr;
extern char         * tzname[2];
extern const char     gmt[4];
extern const char     wildabbr[9];
extern const int      year_lengths[2];
extern const int      mon_lengths[2][MONSPERYEAR];
extern long int       timezone;
extern int            daylight;

#define EFI_UNSPECIFIED_TIMEZONE  0x07FF

/*
** The DST rules to use if TZ has no rules and we can't load TZDEFRULES.
** We default to US rules as of 1999-08-17.
** POSIX 1003.1 section 8.1.1 says that the default DST rules are
** implementation dependent; for historical reasons, US rules are a
** common default.
*/
#ifndef TZDEFRULESTRING
#define TZDEFRULESTRING ",M4.1.0,M10.5.0"
#endif

// Facilities for external time-zone definition files do not currently exist
#define NO_ZONEINFO_FILES

#define EPOCH_DAY     5
#define DAY_TO_uSEC   86400000000

/* Rule type values for the r_type member of a rule structure */
#define JULIAN_DAY            0   /* Jn - Julian day */
#define DAY_OF_YEAR           1   /* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK 2   /* Mm.n.d - month, week, day of week */

#ifdef TZNAME_MAX
  #define MY_TZNAME_MAX TZNAME_MAX
#endif /* defined TZNAME_MAX */

#ifndef TZNAME_MAX
  #define MY_TZNAME_MAX 255
#endif /* !defined TZNAME_MAX */

/* Unlike <ctype.h>'s isdigit, this also works if c < 0 | c > UCHAR_MAX.  */
#define is_digit(c) ((unsigned)(c) - '0' <= 9)

#define LEAPS_THRU_END_OF(y)  ((y) / 4 - (y) / 100 + (y) / 400)

#define BIGGEST(a, b) (((a) > (b)) ? (a) : (b))

#ifndef INITIALIZE
#define INITIALIZE(x) ((x) = 0)
#endif /* !defined INITIALIZE */

struct ttinfo {       /* time type information */
  INT32    tt_gmtoff;  /* UTC offset in seconds */
  int   tt_isdst; /* used to set tm_isdst */
  int   tt_abbrind; /* abbreviation list index */
  int   tt_ttisstd; /* TRUE if transition is std time */
  int   tt_ttisgmt; /* TRUE if transition is UTC */
};

struct lsinfo {       /* leap second information */
  time_t    ls_trans; /* transition time */
  INT32    ls_corr;  /* correction to apply */
};

struct state {
  int           leapcnt;
  int           timecnt;
  int           typecnt;
  int           charcnt;
  time_t        ats[TZ_MAX_TIMES];
  unsigned char types[TZ_MAX_TIMES];
  struct ttinfo ttis[TZ_MAX_TYPES];
  char          chars[BIGGEST(BIGGEST(TZ_MAX_CHARS + 1, sizeof gmt), (2 * (MY_TZNAME_MAX + 1)))];
  struct lsinfo lsis[TZ_MAX_LEAPS];
};

struct rule {
  int     r_type;   /* type of rule--see below */
  int     r_day;    /* day number of rule */
  int     r_week;   /* week number of rule */
  int     r_mon;    /* month number of rule */
  INT32    r_time;   /* transition time of rule */
};

#define JULIAN_DAY    0 /* Jn - Julian day */
#define DAY_OF_YEAR   1 /* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK 2 /* Mm.n.d - month, week, day of week */

extern void gmtload(struct state * const sp);
extern void tzset(void);

#endif  /* _TIMEVAL_H */

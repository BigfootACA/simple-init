/* $NetBSD: strtold_subr.c,v 1.1 2006/03/15 17:35:18 kleink Exp $ */

/*
 * Written by Klaus Klein <kleink@NetBSD.org>, November 16, 2005.
 * Public domain.
 */

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * the format-specific strtold_*.c, like so:
 *
 *  #define GDTOA_LD_FMT  <gdtoa extended-precision format code>
 *  #include "strtold_subr.c"
 */
#include  "namespace.h"
#include  <math.h>
#include  <stdint.h>
#include  <stdlib.h>
#include  "gdtoa.h"

weak_decl long double
strtold(const char *nptr, char **endptr)
{
  long double ld;

  (void)strtopx(nptr, endptr, &ld);
  return ld;
}

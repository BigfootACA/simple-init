#include "libufdt_sysdeps.h"

#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/* Codes from
 * https://android.googlesource.com/platform/bionic.git/+/eclair-release/libc/stdlib/qsort.c
 * Start
 */

/* $OpenBSD: qsort.c,v 1.10 2005/08/08 08:05:37 espie Exp $ */
/*-
 * Copyright (c) 1992, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

void dto_qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *)) {
  qsort(base, nmemb, size, compar);
}

/* Assuming the following functions are already defined in the
 * bootloader source with the names conforming to POSIX.
 */

void *dto_malloc(size_t size) { return malloc(size); }

void dto_free(void *ptr) { free(ptr); }

char *dto_strchr(const char *s, int c) { return strchr(s, c); }

unsigned long int dto_strtoul(const char *nptr, char **endptr, int base) {
  return strtoul(nptr, endptr, base);
}

size_t dto_strlen(const char *s) { return strlen(s); }

int dto_memcmp(const void *lhs, const void *rhs, size_t n) {
  return memcmp(lhs, rhs, n);
}

void *dto_memcpy(void *dest, const void *src, size_t n) {
  return memcpy(dest, src, n);
}

int dto_strcmp(const char *s1, const char *s2) { return strcmp(s1, s2); }

int dto_strncmp(const char *s1, const char *s2, size_t n) {
  return strncmp(s1, s2, n);
}

void *dto_memchr(const void *s, int c, size_t n) { return memchr(s, c, n); }

void *dto_memset(void *s, int c, size_t n) { return memset(s, c, n); }

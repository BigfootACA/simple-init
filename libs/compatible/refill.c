/*
    Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available
    under the terms and conditions of the BSD License that accompanies this
    distribution.  The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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

    NetBSD: refill.c,v 1.13 2003/08/07 16:43:30 agc Exp
    refill.c  8.1 (Berkeley) 6/4/93
*/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "reentrant.h"
#include "local.h"

#ifdef _REENTRANT
extern rwlock_t __sfp_lock;
#endif

/*
 * Refill a stdio buffer.
 * Return EOF on eof or error, 0 otherwise.
 */
int
__srefill(FILE *fp)
{

  if(fp == NULL) {
    errno = EINVAL;
    return (EOF);
  }

  /* make sure stdio is set up */
  if (!__sdidinit)
    __sinit();

  fp->_r = 0;   /* largely a convenience for callers */

  /* SysV does not make this test; take it out for compatibility */
  if (fp->_flags & __SEOF) {
    return (EOF);
  }

  /* if not already reading, have to be reading and writing */
  if ((fp->_flags & __SRD) == 0) {
    if ((fp->_flags & __SRW) == 0) {
      errno = EBADF;
      fp->_flags |= __SERR;   //<dvm> Allows differentiation between errors and EOF
      return (EOF);
    }
    /* switch to reading */
    if (fp->_flags & __SWR) {
      if (__sflush(fp)) {
        return (EOF);
      }
      fp->_flags &= ~__SWR;
      fp->_w = 0;
      fp->_lbfsize = 0;
    }
    fp->_flags |= __SRD;
  } else {
    /*
     * We were reading.  If there is an ungetc buffer,
     * we must have been reading from that.  Drop it,
     * restoring the previous buffer (if any).  If there
     * is anything in that buffer, return.
     */
    if (HASUB(fp)) {
      FREEUB(fp);
      if ((fp->_r = fp->_ur) != 0) {
        fp->_p = fp->_up;
        return (0);
      }
    }
  }


  fp->_p = fp->_bf._base;
  fp->_r = (*fp->_read)(fp->_cookie, (char *)fp->_p, fp->_bf._size);
  fp->_flags &= ~__SMOD;  /* buffer contents are again pristine */
  if (fp->_r <= 0) {
    if (fp->_r == 0)
      fp->_flags |= __SEOF;
    else {
      fp->_r = 0;
      fp->_flags |= __SERR;
    }
    return (EOF);
  }
  return (0);
}

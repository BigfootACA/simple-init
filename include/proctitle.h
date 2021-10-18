/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef PROCTITLE_H
#define PROCTITLE_H

// src/lib/proctitle.c: copy environ
extern int spt_copyenv(char*oldenv[]);

// src/lib/proctitle.c: copy argvs
extern int spt_copyargs(int argc,char*argv[]);

// src/lib/proctitle.c: init proctitle
extern void spt_init(int argc,char*argv[]);

// src/lib/proctitle.c: set proctitle
extern void setproctitle(const char*fmt,...) __attribute__((format(printf,1,2)));;
#endif

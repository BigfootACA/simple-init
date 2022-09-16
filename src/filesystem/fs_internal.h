/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifndef _FILESYSTEM_INTERNAL_H
#define _FILESYSTEM_INTERNAL_H
#include<stdlib.h>
#include<string.h>
#include"filesystem.h"
#include"fsdrv.h"
#include"logger.h"
#include"lock.h"
#include"str.h"
#define TAG "fs"
#define FS_BUF_SIZE 4096
#define fsh_new(drv,uri,data,flag)\
	fsh_get_new(drv,uri,(void**)&(data),sizeof(*(data)),flag)
#define RET(e) return (errno=(e))
#define DONE(e) {(errno=(e));goto done;}
#define XRET(e,d) return (errno=((e)?:(d)))
#define EXRET(e) return errno?:e
#endif

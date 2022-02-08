/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _PARAM_H
#define _PARAM_H
#include"keyval.h"

// src/lib/param.c: read string from fd and convert to keyval array
extern keyval**read_params(int fd);

// src/lib/param.c: parse cmdline items from string with length
extern keyval**param_s_parse_items(char*cmdline,size_t len,size_t*length);

// src/lib/param.c: parse cmdline items from string
extern keyval**param_parse_items(char*cmdline,size_t*length);

#endif

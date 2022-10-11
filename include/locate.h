/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LOCATE_H
#define _LOCATE_H
#include<Uefi.h>
#include<sys/types.h>

// src/locate/locate.c: find a available locate name
extern char*locate_find_name(char*buf,size_t len);

// src/locate/locate.c: find a efi handle by locate tag name
extern EFI_HANDLE*locate_get_handle_by_tag(const char*tag);

#endif

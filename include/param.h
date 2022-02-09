/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _PARAM_H
#define _PARAM_H
#include<stdbool.h>
#include"keyval.h"

// src/lib/param.c: read string from fd and convert to keyval array
extern keyval**read_params(int fd);

// src/lib/param.c: parse cmdline items from string with length
extern keyval**param_s_parse_items(char*cmdline,size_t len,size_t*length);

// src/lib/param.c: parse cmdline items from string
extern keyval**param_parse_items(char*cmdline,size_t*length);

// src/lib/param.c: get androidboot.mode
extern char*param_get_android_boot_mode(keyval**items);

// src/lib/param.c: get androidboot.hardware
extern char*param_get_android_hardware(keyval**items);

// src/lib/param.c: get androidboot.bootdevice
extern char*param_get_android_boot_device(keyval**items);

// src/lib/param.c: get androidboot.serialno
extern char*param_get_android_serial_number(keyval**items);

// src/lib/param.c: get androidboot.slot_suffix
extern char*param_get_android_slot_suffix(keyval**items);

// src/lib/param.c: is androidboot.mode=charger
extern bool param_is_android_charger_mode(keyval**items);

// src/lib/param.c: has skip_initramfs
extern bool param_is_android_recovery_mode(keyval**items);
#endif

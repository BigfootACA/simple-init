/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef FILEPICKER_H
#define FILEPICKER_H
#include<stdint.h>
#include<stdbool.h>
struct filepicker;
typedef bool(*filepicker_callback)(bool ok,const char**path,uint16_t cnt,void*user_data);

// src/gui/interface/widgets/filepicker.c: create a dialog to select files
extern struct filepicker*filepicker_create(filepicker_callback callback,const char*title,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/filepicker.c: set filepicker title
extern void filepicker_set_title(struct filepicker*fp,const char*title,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/filepicker.c: set filepicker show path
extern void filepicker_set_path(struct filepicker*fp,const char*path,...) __attribute__((format(printf,2,3)));

// src/gui/interface/widgets/filepicker.c: set filepicker user data
extern void filepicker_set_user_data(struct filepicker*fp,void*user_data);

// src/gui/interface/widgets/filepicker.c: set filepicker maximum selectable items
extern void filepicker_set_max_item(struct filepicker*fp,uint16_t max);
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _IMAGE_H
#define _IMAGE_H
#include<time.h>
#include<stdint.h>
#include<stddef.h>
#include<limits.h>
#include"gui.h"
#include"defines.h"
typedef struct image_data{
	time_t last;
	char path[PATH_MAX];
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint8_t*pixels;
}image_data;
typedef int(*image_decode_cb)(unsigned char*data,size_t len,image_data*img);
typedef struct image_decoder{
	image_decode_cb decode_cb;
	char**types;
}image_decoder;
#endif

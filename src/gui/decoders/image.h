/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _IMAGE_H
#define _IMAGE_H
#include"gui.h"
typedef lv_res_t(*image_info_cb)(unsigned char*data,size_t len,uint32_t*w,uint32_t*h,uint32_t*cf);
typedef lv_res_t(*image_decode_cb)(unsigned char*data,size_t len,uint8_t**img);
typedef struct image_decoder{
	image_info_cb info_cb;
	image_decode_cb decode_cb;
	char**types;
}image_decoder;
#endif

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

extern image_decoder*image_get_decoder(char*ext);
extern void image_decoder_init(void);
extern void image_set_cache_time(time_t time);
extern void image_cache_clean(void);
extern int image_cache_gc(void);
extern long image_get_cache_hits();
extern long image_get_cache_misses();
extern long image_get_load_fails();
static inline long image_get_process_count(){return image_get_cache_hits()+image_get_cache_misses()+image_get_load_fails();}
static inline float image_get_cache_hit_percent(){return (float)image_get_cache_hits()/image_get_process_count()*100;}
static inline float image_get_cache_miss_percent(){return (float)image_get_cache_misses()/image_get_process_count()*100;}
static inline float image_get_load_fail_percent(){return (float)image_get_load_fails()/image_get_process_count()*100;}
extern void image_print_stat();
#endif

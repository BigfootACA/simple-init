/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FRAME_PROTOCOL_H
#define _FRAME_PROTOCOL_H
#include<time.h>
#include<stdint.h>
#include<stdbool.h>
#define FRAME_MAGIC "!FRAME!"
#define FRAME_VERSION 0x02
typedef enum frame_type{
	TYPE_NONE=0,
	TYPE_RAW,
	TYPE_BMP,
	TYPE_JPG,
	TYPE_PNG,
	TYPE_LAST,
	TYPE_MAX=0xFF
}frame_type;
typedef enum frame_pixel{
	PIXEL_NONE=0,
	PIXEL_RGB24,
	PIXEL_BGR24,
	PIXEL_ARGB32,
	PIXEL_ABGR32,
	PIXEL_RGBA32,
	PIXEL_BGRA32,
	PIXEL_LAST,
	PIXEL_MAX=0xFF
}frame_pixel;
typedef struct frame_data{
	char magic[7];
	bool compressed:1;
	uint32_t version;
	uint32_t cost_time;
	uint64_t gen_time;
	uint64_t src_size,size;
	uint32_t src_x,src_y;
	uint32_t dst_x,dst_y;
	frame_type type:16;
	frame_pixel pixel:16;
	char frame[];
}frame_data;
static inline uint8_t pixel_size(frame_pixel mode){
	switch(mode){
		case PIXEL_RGB24:
		case PIXEL_BGR24:return 3;
		case PIXEL_ARGB32:
		case PIXEL_ABGR32:
		case PIXEL_RGBA32:
		case PIXEL_BGRA32:return 4;
		default:return 0;
	}
}
static inline void pixel_copy(frame_pixel src_type,char*dst,char*src){
	switch(src_type){
		case PIXEL_ARGB32:
			src++;
		//fallthrough
		case PIXEL_RGB24:
		case PIXEL_RGBA32:
			dst[0]=src[0];
			dst[1]=src[1];
			dst[2]=src[2];
		break;
		case PIXEL_ABGR32:
			src++;
		//fallthrough
		case PIXEL_BGR24:
		case PIXEL_BGRA32:
			dst[0]=src[2];
			dst[1]=src[1];
			dst[2]=src[0];
		break;
		default:;
	}
}
#endif

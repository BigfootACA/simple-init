/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LODEPNG
#include<lodepng.h>
#include"lvgl.h"
#include"image.h"
#include"logger.h"
#define TAG "png"

static lv_res_t image_info(unsigned char*data,size_t len __attribute__((unused)),uint32_t*w,uint32_t*h,uint32_t*cf){
	uint32_t size[2];
	memcpy(size,data+16,8);
	*cf=LV_IMG_CF_RAW_ALPHA;
	*w=(uint32_t)((size[0]&0xff000000)>>24)+((size[0]&0x00ff0000)>>8);
	*h=(uint32_t)((size[1]&0xff000000)>>24)+((size[1]&0x00ff0000)>>8);
	return LV_RES_OK;
}

static lv_res_t image_decode(unsigned char*data,size_t len,uint8_t**img){
	uint32_t w,h,e;
	if((e=lodepng_decode32(img,&w,&h,data,len)))
		return trlog_warn(LV_RES_INV,"decode png failed %u: %s",e,lodepng_error_text(e));
	for(uint32_t i=0;i<w*h;i++){
		lv_color32_t*src=((lv_color32_t*)*img)+i;
		lv_color_t*dst=((lv_color_t*)*img)+i;
		lv_color_t c=LV_COLOR_MAKE(src->ch.red,src->ch.green,src->ch.blue);
		dst->ch.red=c.ch.blue,dst->ch.blue=c.ch.red;
	}
	return LV_RES_OK;
}

image_decoder image_decoder_png={
	.info_cb=image_info,
	.decode_cb=image_decode,
	.types=(char*[]){"png",NULL}
};
#endif
#endif

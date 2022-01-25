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
#include"gui/image.h"
#include"logger.h"
#define TAG "png"

static int image_decode(unsigned char*data,size_t len,image_data*img){
	uint32_t e;
	img->format=LV_IMG_CF_RAW_ALPHA;
	if((e=lodepng_decode32(&img->pixels,&img->width,&img->height,data,len)))
		return trlog_warn(-1,"decode png failed %u: %s",e,lodepng_error_text(e));
	for(uint32_t i=0;i<img->width*img->height;i++){
		lv_color32_t*src=((lv_color32_t*)img->pixels)+i;
		lv_color_t*dst=((lv_color_t*)img->pixels)+i;
		lv_color_t c=LV_COLOR_MAKE(src->ch.red,src->ch.green,src->ch.blue);
		dst->ch.red=c.ch.blue,dst->ch.blue=c.ch.red;
	}
	return 0;
}

image_decoder image_decoder_png={
	.decode_cb=image_decode,
	.types=(char*[]){"png",NULL}
};
#endif
#endif

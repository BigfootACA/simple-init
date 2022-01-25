/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include"gui/image.h"

static int image_decode(unsigned char*data,size_t len,image_data*img){
	int row;
	uint16_t bpp;
	lv_color32_t*cs=NULL;
	lv_coord_t off;
	if(len<54||data[0]!=0x42||data[1]!=0x4D)goto fail;
	memcpy(&off,data+10,4);
	memcpy(&img->width,data+18,4);
	memcpy(&img->height,data+22,4);
	memcpy(&bpp,data+28,2);
	img->format=bpp==32?LV_IMG_CF_TRUE_COLOR_ALPHA:LV_IMG_CF_TRUE_COLOR;
	row=(bpp*img->width)/8;
	if(!(cs=malloc(img->width*img->height*sizeof(lv_color32_t))))goto fail;
	uint8_t*file=(uint8_t*)img+off;
	for(lv_coord_t h=0;h<(lv_coord_t)img->height;h++){
		lv_coord_t rh=img->height-h-1;
		uint8_t*hs=file+(row*h);
		for(lv_coord_t w=0;w<(lv_coord_t)img->width;w++){
			uint8_t*ws=hs+(w*(bpp/8));
			lv_color32_t*c=cs+(w+(rh*img->width));
			if((unsigned char*)ws+2>data+len)goto fail;
			c->ch.red=ws[2],c->ch.green=ws[1],c->ch.blue=ws[0];
			c->ch.alpha=bpp==32?ws[3]:0xFF;
		}
	}
	img->pixels=(uint8_t*)cs;
	return 0;
	fail:
	if(cs)free(cs);
	return -1;
}

image_decoder image_decoder_bmp={
	.decode_cb=image_decode,
	.types=(char*[]){"bmp",NULL}
};
#endif

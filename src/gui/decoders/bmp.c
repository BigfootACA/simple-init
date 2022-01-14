/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include<string.h>
#include"lvgl.h"
#include"image.h"

static lv_res_t image_info(unsigned char*data,size_t len,uint32_t*w,uint32_t*h,uint32_t*cf){
	uint16_t bpp=0;
	if(
		len<54||
		data[0]!=0x42||
		data[1]!=0x4D
	)return LV_RES_INV;
	memcpy(w,data+18,4);
	memcpy(h,data+22,4);
	memcpy(&bpp,data+28,2);
	*cf=bpp==32?LV_IMG_CF_TRUE_COLOR_ALPHA:LV_IMG_CF_TRUE_COLOR;
	return LV_RES_OK;
}

static lv_res_t image_decode(unsigned char*data,size_t len,uint8_t**img){
	int row;
	uint16_t bpp;
	lv_color32_t*cs=NULL;
	lv_coord_t pw,ph,off;
	if(data[0]!=0x42||data[1]!=0x4D)goto fail;
	memcpy(&off,data+10,4);
	memcpy(&pw,data+18,4);
	memcpy(&ph,data+22,4);
	memcpy(&bpp,data+28,2);
	row=(bpp*pw)/8;
	if(!(cs=malloc(pw*ph*sizeof(lv_color32_t))))goto fail;
	uint8_t*file=(uint8_t*)img+off;
	for(lv_coord_t h=0;h<ph;h++){
		lv_coord_t rh=ph-h-1;
		uint8_t*hs=file+(row*h);
		for(lv_coord_t w=0;w<pw;w++){
			uint8_t*ws=hs+(w*(bpp/8));
			lv_color32_t*c=cs+(w+(rh*pw));
			if((unsigned char*)ws+2>data+len)goto fail;
			c->ch.red=ws[2],c->ch.green=ws[1],c->ch.blue=ws[0];
			c->ch.alpha=bpp==32?ws[3]:0xFF;
		}
	}
	*img=(uint8_t*)cs;
	return LV_RES_OK;
	fail:
	if(cs)free(cs);
	return LV_RES_INV;
}

image_decoder image_decoder_bmp={
	.info_cb=image_info,
	.decode_cb=image_decode,
	.types=(char*[]){"bmp",NULL}
};
#endif

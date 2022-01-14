/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_NANOSVG
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include"defines.h"
#include"nanosvg.h"
#include"nanosvgrast.h"
#include"lvgl.h"
#include"image.h"
#define TAG "svg"

static lv_res_t image_info(unsigned char*data,size_t len __attribute__((unused)),uint32_t*w,uint32_t*h,uint32_t*cf){
	NSVGimage*img=nsvgParse((char*)data,"px",(float)gui_dpi);
	if(!img)return LV_RES_INV;
	*cf=LV_IMG_CF_RAW_ALPHA;
	*w=img->width,*h=img->height;
	nsvgDelete(img);
	return LV_RES_OK;
}

static lv_res_t image_decode(unsigned char*data,size_t len __attribute__((unused)),uint8_t**img){
	NSVGimage*m=NULL;
	NSVGrasterizer*rast=NULL;
	lv_res_t s=LV_RES_INV;
	if(!(m=nsvgParse((char*)data,"px",(float)gui_dpi)))goto fail;
	if(!(rast=nsvgCreateRasterizer()))goto fail;
	if(!(*img=malloc(m->width*m->height*4)))goto fail;
	nsvgRasterize(rast,m,0,0,1,*img,m->width,m->height,m->width*4);
	for(size_t i=0;i<m->width*m->height;i++){
		uint8_t*b=(*img)+(i*4),k;
		k=b[0],b[0]=b[2],b[2]=k;
	}
	s=LV_RES_OK;
	done:
	if(rast)nsvgDeleteRasterizer(rast);
	if(m)nsvgDelete(m);
	return s;
	fail:
	if(*img)free(*img);
	s=LV_RES_INV;
	goto done;
}

image_decoder image_decoder_svg={
	.info_cb=image_info,
	.decode_cb=image_decode,
	.types=(char*[]){"svg",NULL}
};
#endif
#endif

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
#include"image.h"
#define TAG "svg"

static int image_decode(unsigned char*data,size_t len __attribute__((unused)),struct image_data*img){
	int s=-1;
	NSVGimage*m=NULL;
	NSVGrasterizer*rast=NULL;
	if(!(m=nsvgParse((char*)data,"px",(float)gui_dpi)))goto fail;
	if(!(rast=nsvgCreateRasterizer()))goto fail;
	if(!(img->pixels=malloc(m->width*m->height*4)))goto fail;
	nsvgRasterize(rast,m,0,0,1,img->pixels,m->width,m->height,m->width*4);
	for(size_t i=0;i<m->width*m->height;i++){
		uint8_t*b=img->pixels+(i*4),k;
		k=b[0],b[0]=b[2],b[2]=k;
	}
	img->width=m->width;
	img->height=m->height;
	img->format=LV_IMG_CF_RAW_ALPHA;
	s=0;
	done:
	if(rast)nsvgDeleteRasterizer(rast);
	if(m)nsvgDelete(m);
	return s;
	fail:
	if(img->pixels)free(img->pixels);
	img->pixels=NULL;
	s=-1;
	goto done;
}

image_decoder image_decoder_svg={
	.decode_cb=image_decode,
	.types=(char*[]){"svg",NULL}
};
#endif
#endif

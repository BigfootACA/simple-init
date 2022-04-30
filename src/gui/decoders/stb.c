/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_STB
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#include<unistd.h>
#include"stb_image.h"
#include"gui/image.h"
#define TAG "stb"

static int image_decode(unsigned char*data,size_t len,struct image_data*img){
	int x=0,y=0;
	img->pixels=(uint8_t*)stbi_load_from_memory(
		(const stbi_uc*)data,len,&x,&y,NULL,4
	);
	if(img->pixels){
		img->width=x,img->height=y;
		img->format=LV_IMG_CF_RAW_ALPHA;
		uint8_t s,*p;
		for(p=img->pixels;p<img->pixels+((size_t)x*y*4);p+=4)
			s=p[0],p[0]=p[2],p[2]=s;
	}
	return img->pixels?0:-1;
}

image_decoder image_decoder_stb={
	.decode_cb=image_decode,
	.types=(char*[]){
		#ifndef STBI_NO_JPEG
		"jpeg","jpg",
		#endif
		#ifndef STBI_NO_PNG
		"png",
		#endif
		#ifndef STBI_NO_BMP
		"bmp",
		#endif
		#ifndef STBI_NO_PSD
		"psd",
		#endif
		#ifndef STBI_NO_TGA
		"tga",
		#endif
		#ifndef STBI_NO_GIF
		"gif",
		#endif
		#ifndef STBI_NO_HDR
		"hdr",
		#endif
		#ifndef STBI_NO_PIC
		"pic",
		#endif
		#ifndef STBI_NO_PNM
		"pnm","ppm","pgm",
		#endif
		NULL
	}
};
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_LIBJPEG
#include<stdio.h>
#include<stddef.h>
#include<setjmp.h>
#include<jpeglib.h>
#include"gui/image.h"
#include"logger.h"
#define TAG "jpeg"

struct jpeg_error{
	struct jpeg_error_mgr pub;
	jmp_buf buf;
};

static void jpeg_output_message(j_common_ptr ci){
	char buffer[JMSG_LENGTH_MAX];
	(*ci->err->format_message)(ci,buffer);
	tlog_error("%s",buffer);
}

static void jpeg_error_exit(j_common_ptr ci){
	struct jpeg_error*e=(struct jpeg_error*)ci->client_data;
	jpeg_output_message(ci);
	longjmp(e->buf,1);
}

static int image_decode(unsigned char*data,size_t len,image_data*img){
	int s=0;
	lv_color32_t*cs=NULL;
	size_t dlen,blen;
	uint8_t*buf=NULL;
	struct jpeg_error e;
	struct jpeg_decompress_struct ci;
	memset(&ci,0,sizeof(ci));
	ci.err=jpeg_std_error(&e.pub);
	ci.client_data=&e;
	e.pub.error_exit=jpeg_error_exit;
	e.pub.output_message=jpeg_output_message;
	if(setjmp(e.buf))goto fail;
	jpeg_create_decompress(&ci);
	jpeg_mem_src(&ci,data,len);
	jpeg_read_header(&ci,true);
	img->width=ci.image_width;
	img->height=ci.image_height;
	img->format=LV_IMG_CF_TRUE_COLOR;
	jpeg_start_decompress(&ci);
	dlen=ci.output_width*ci.output_components;
	blen=ci.image_width*ci.image_height*sizeof(lv_color32_t);
	if(!(cs=malloc(blen))||!(buf=malloc(dlen)))goto fail;
	memset(cs,0,blen);
	memset(buf,0,dlen);
	while(ci.output_scanline<ci.output_height){
		jpeg_read_scanlines(&ci,&buf,1);
		for(size_t i=0;i<ci.output_width;i++){
			uint8_t*b=buf+(i*ci.output_components);
			lv_color32_t*c=cs+(ci.output_width*(ci.output_scanline-1))+i;
			c->ch.red=(*b),c->ch.green=*(b+1),c->ch.blue=*(b+2);
		}
	}
	jpeg_finish_decompress(&ci);
	img->pixels=(uint8_t*)cs;
	done:
	jpeg_destroy_decompress(&ci);
	if(buf)free(buf);
	return s;
	fail:
	if(cs)free(cs);
	s=-1;
	goto done;
}

image_decoder image_decoder_jpeg={
	.decode_cb=image_decode,
	.types=(char*[]){"jpg","jpeg",NULL}
};
#endif
#endif

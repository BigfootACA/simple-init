#ifdef ENABLE_GUI
#ifdef ENABLE_LIBJPEG
#include<fcntl.h>
#include<stdio.h>
#include<stddef.h>
#include<setjmp.h>
#include<jpeglib.h>
#include"lvgl.h"
#include"logger.h"
#include"assets.h"
#define TAG "jpeg"

struct jpeg_error{
	struct jpeg_error_mgr pub;
	jmp_buf buf;
};

void jpeg_output_message(j_common_ptr ci){
	char buffer[JMSG_LENGTH_MAX];
	(*ci->err->format_message)(ci,buffer);
	tlog_error("%s",buffer);
}

void jpeg_error_exit(j_common_ptr ci){
	struct jpeg_error*e=(struct jpeg_error*)ci->client_data;
	jpeg_output_message(ci);
	longjmp(e->buf,1);
}

static lv_res_t decoder_info(lv_img_decoder_t*d __attribute__((unused)),const void* src,lv_img_header_t*i){
	FILE*fe=NULL;
	lv_res_t s=LV_RES_OK;
	char*fn=(char*)src;
	struct jpeg_error e;
	struct jpeg_decompress_struct ci;
	if(lv_img_src_get_type(src)!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(fn+strlen(fn)-3,"jpg")!=0)return LV_RES_INV;
	memset(&ci,0,sizeof(ci));
	ci.err=jpeg_std_error(&e.pub);
	ci.client_data=&e;
	e.pub.error_exit=jpeg_error_exit;
	e.pub.output_message=jpeg_output_message;
	if(setjmp(e.buf))goto fail;
	jpeg_create_decompress(&ci);
	if((fe=fopen(fn,"rb")))jpeg_stdio_src(&ci,fe);
	else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=128)goto fail;
		jpeg_mem_src(&ci,(unsigned char*)f->content,f->length);
	}
	jpeg_read_header(&ci,true);
	i->w=ci.image_width;
	i->h=ci.image_height;
	i->always_zero=0;
	i->cf=LV_IMG_CF_TRUE_COLOR;
	done:
	jpeg_destroy_decompress(&ci);
	if(fe)fclose(fe);
	return s;
	fail:
	s=LV_RES_INV;
	goto done;
}

static lv_res_t decoder_open(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	FILE*fe=NULL;
	lv_res_t s=LV_RES_OK;
	lv_color32_t*cs=NULL;
	unsigned char*buf=NULL;
	char*fn=(char*)dsc->src;
	struct jpeg_error e;
	struct jpeg_decompress_struct ci;
	if(dsc->src_type!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(fn+strlen(fn)-3,"jpg")!=0)return LV_RES_INV;
	memset(&ci,0,sizeof(ci));
	ci.err=jpeg_std_error(&e.pub);
	ci.client_data=&e;
	e.pub.error_exit=jpeg_error_exit;
	e.pub.output_message=jpeg_output_message;
	if(setjmp(e.buf))goto fail;
	jpeg_create_decompress(&ci);
	if((fe=fopen(fn,"rb")))jpeg_stdio_src(&ci,fe);
	else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=128)goto fail;
		jpeg_mem_src(&ci,(unsigned char*)f->content,f->length);
	}
	jpeg_read_header(&ci,true);
	jpeg_start_decompress(&ci);
	if(!(cs=malloc(ci.image_width*ci.image_height*sizeof(lv_color32_t))))goto fail;
	if(!(buf=malloc(ci.output_width*ci.output_components)))goto fail;
	while(ci.output_scanline<ci.output_height){
		jpeg_read_scanlines(&ci,&buf,1);
		for(size_t i=0;i<ci.output_width;i++){
			unsigned char*b=buf+(i*ci.output_components);
			lv_color32_t*c=cs+(ci.output_width*(ci.output_scanline-1))+i;
			c->ch.red=(*b),c->ch.green=*(b+1),c->ch.blue=*(b+2);
		}
	}
	jpeg_finish_decompress(&ci);
	dsc->img_data=(uint8_t*)cs;
	done:
	jpeg_destroy_decompress(&ci);
	if(buf)free(buf);
	if(fe)fclose(fe);
	return s;
	fail:
	if(cs)free(cs);
	s=LV_RES_INV;
	goto done;
}

static void decoder_close(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	if(dsc->img_data)free((uint8_t*)dsc->img_data);
	dsc->img_data=NULL;
}

void jpeg_decoder_init(){
	lv_img_decoder_t*dec=lv_img_decoder_create();
	lv_img_decoder_set_info_cb(dec,decoder_info);
	lv_img_decoder_set_open_cb(dec,decoder_open);
	lv_img_decoder_set_close_cb(dec,decoder_close);
}
#endif
#endif

#ifdef ENABLE_GUI
#ifdef ENABLE_LODEPNG
#include<lodepng.h>
#include<stdlib.h>
#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include"lvgl.h"
#include"logger.h"
#include"assets.h"
#define TAG "png"

static void convert_color_depth(uint8_t*img,uint32_t px_cnt){
	lv_color32_t*img_argb=(lv_color32_t*)img;
	lv_color_t c,*img_c=(lv_color_t*)img;
	for(uint32_t i=0;i<px_cnt;i++){
		c=LV_COLOR_MAKE(img_argb[i].ch.red,img_argb[i].ch.green,img_argb[i].ch.blue);
		img_c[i].ch.red=c.ch.blue,img_c[i].ch.blue=c.ch.red;
	}
}

static lv_res_t decoder_info(struct _lv_img_decoder*decoder __attribute__((unused)),const void*src,lv_img_header_t*header){
	switch(lv_img_src_get_type(src)){
		case LV_IMG_SRC_FILE:{
			char*fn=(char*)src;
			if(strcmp(&fn[strlen(fn)-3],"png")!=0)break;
			uint32_t size[2];
			int fd;
			if((fd=open(fn,O_RDONLY))>=0){
				lseek(fd,16,SEEK_SET);
				read(fd,size,8);
				close(fd);
			}else{
				struct entry_file*f=rootfs_get_assets_file(fn);
				if(!f||f->length<=32)return LV_RES_INV;
				memcpy(size,f->content+16,8);
			}
			header->always_zero=0;
			header->cf=LV_IMG_CF_RAW_ALPHA;
			header->w=(lv_coord_t)((size[0]&0xff000000)>>24)+((size[0]&0x00ff0000)>>8);
			header->h=(lv_coord_t)((size[1]&0xff000000)>>24)+((size[1]&0x00ff0000)>>8);
			return LV_RES_OK;
		}
		case LV_IMG_SRC_VARIABLE:{
			const lv_img_dsc_t*img_dsc=src;
			header->always_zero=0;
			header->cf=img_dsc->header.cf;
			header->w=img_dsc->header.w;
			header->h=img_dsc->header.h;
			return LV_RES_OK;
		}
	}
	return LV_RES_INV;
}

static lv_res_t decoder_open(lv_img_decoder_t*decoder __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	uint8_t*img_data=NULL;
	uint32_t png_width,png_height,error=1;
	switch(dsc->src_type){
		case LV_IMG_SRC_FILE:{
			char*fn=(char*)dsc->src;
			if(strcmp(&fn[strlen(fn)-3],"png")!=0)break;
			unsigned char*png_data;
			size_t png_data_size;
			error=lodepng_load_file(&png_data,&png_data_size,fn);
			if(error){
				struct entry_file*f=rootfs_get_assets_file(fn);
				if(!f)return trlog_warn(LV_RES_INV,"load png failed %u: %s",error,lodepng_error_text(error));
				png_data_size=f->length;
				png_data=(unsigned char*)f->content;
			}
			error=lodepng_decode32(&img_data,&png_width,&png_height,png_data,png_data_size);
			if(error)return trlog_warn(LV_RES_INV,"decode png failed %u: %s",error,lodepng_error_text(error));
			convert_color_depth(img_data,png_width*png_height);
			dsc->img_data = img_data;
			return LV_RES_OK;
		}
		case LV_IMG_SRC_VARIABLE:{
			const lv_img_dsc_t*img_dsc=dsc->src;
			error=lodepng_decode32(&img_data,&png_width,&png_height,img_dsc->data,img_dsc->data_size);
		}
	}
	if(error)return LV_RES_INV;
	convert_color_depth(img_data,png_width*png_height);
	dsc->img_data=img_data;
	return LV_RES_OK;
}

static void decoder_close(lv_img_decoder_t*decoder __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	if(dsc->img_data)free((uint8_t*)dsc->img_data);
	dsc->img_data=NULL;
}

void png_decoder_init(){
	lv_img_decoder_t*dec=lv_img_decoder_create();
	lv_img_decoder_set_info_cb(dec,decoder_info);
	lv_img_decoder_set_open_cb(dec,decoder_open);
	lv_img_decoder_set_close_cb(dec,decoder_close);
}
#endif
#endif

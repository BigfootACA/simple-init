#ifdef ENABLE_GUI
#ifdef ENABLE_LODEPNG
#include<lodepng.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include"lvgl.h"
#include"logger.h"
#include"assets.h"
#define TAG "png"

static lv_res_t decoder_info(struct _lv_img_decoder*decoder __attribute__((unused)),const void*src,lv_img_header_t*header){
	int fd;
	uint32_t size[2];
	char*fn=(char*)src;
	if(lv_img_src_get_type(src)!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(&fn[strlen(fn)-3],"png")!=0)return LV_RES_INV;
	if((fd=open(fn,O_RDONLY,0644))>=0){
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

static lv_res_t decoder_open(lv_img_decoder_t*decoder __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	uint8_t*img_data=NULL;
	uint32_t w,h,e;
	size_t size;
	char*fn=(char*)dsc->src;
	unsigned char*data;
	if(dsc->src_type!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(fn+strlen(fn)-3,"png")!=0)return LV_RES_INV;
	if((e=lodepng_load_file(&data,&size,fn))){
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f)return trlog_warn(LV_RES_INV,"load png failed %u: %s",e,lodepng_error_text(e));
		size=f->length,data=(unsigned char*)f->content;
	}
	if((e=lodepng_decode32(&img_data,&w,&h,data,size)))
		return trlog_warn(LV_RES_INV,"decode png failed %u: %s",e,lodepng_error_text(e));
	for(uint32_t i=0;i<w*h;i++){
		lv_color32_t*src=((lv_color32_t*)img_data)+i;
		lv_color_t*dst=((lv_color_t*)img_data)+i;
		lv_color_t c=LV_COLOR_MAKE(src->ch.red,src->ch.green,src->ch.blue);
		dst->ch.red=c.ch.blue,dst->ch.blue=c.ch.red;
	}
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

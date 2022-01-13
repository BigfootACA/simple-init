#ifdef ENABLE_GUI
#ifdef ENABLE_NANOSVG
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include"defines.h"
#include"nanosvg.h"
#include"nanosvgrast.h"
#include"lvgl.h"
#include"assets.h"
#define TAG "svg"

static lv_res_t decoder_info(struct _lv_img_decoder*decoder __attribute__((unused)),const void*src,lv_img_header_t*header){
	int fd;
	ssize_t r=-1;
	struct stat st;
	lv_res_t s=LV_RES_INV;
	char*fn=(char*)src,*file=NULL;
	if(lv_img_src_get_type(src)!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(&fn[strlen(fn)-3],"svg")!=0)return LV_RES_INV;
	if((fd=open(fn,O_RDONLY,0644))>=0){
		if(fstat(fd,&st)==0&&(file=malloc(st.st_size+1))){
			memset(file,0,st.st_size+1);
			r=read(fd,file,st.st_size);
		}
		close(fd);
		if(r<0||r!=st.st_size){
			free(file);
			return LV_RES_INV;
		}
	}else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=32)return LV_RES_INV;
		if(!(file=malloc(f->length+1)))return LV_RES_INV;
		memset(file,0,f->length+1);
		memcpy(file,f->content,f->length);
	}
	NSVGimage*img=nsvgParse(file,"px",(float)gui_dpi);
	if(img){
		header->always_zero=0;
		header->cf=LV_IMG_CF_RAW_ALPHA;
		header->w=img->width;
		header->h=img->height;
		nsvgDelete(img);
		s=LV_RES_OK;
	}
	free(file);
	return s;
}

static lv_res_t decoder_open(lv_img_decoder_t*decoder __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	int fd;
	ssize_t r=-1;
	struct stat st;
	NSVGimage*img=NULL;
	NSVGrasterizer*rast=NULL;
	lv_res_t s=LV_RES_INV;
	unsigned char*data=NULL;
	char*fn=(char*)dsc->src,*file=NULL;
	if(dsc->src_type!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(&fn[strlen(fn)-3],"svg")!=0)return LV_RES_INV;
	if((fd=open(fn,O_RDONLY,0644))>=0){
		if(fstat(fd,&st)==0&&(file=malloc(st.st_size+1))){
			memset(file,0,st.st_size+1);
			r=read(fd,file,st.st_size);
		}
		close(fd);
		if(r<0||r!=st.st_size){
			free(file);
			return LV_RES_INV;
		}
	}else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=32)return LV_RES_INV;
		if(!(file=malloc(f->length+1)))return LV_RES_INV;
		memset(file,0,f->length+1);
		memcpy(file,f->content,f->length);
	}
	if(!(img=nsvgParse(file,"px",(float)gui_dpi)))goto fail;
	if(!(rast=nsvgCreateRasterizer()))goto fail;
	if(!(data=malloc(img->width*img->height*4)))goto fail;
	nsvgRasterize(rast,img,0,0,1,data,img->width,img->height,img->width*4);
	for(size_t i=0;i<img->width*img->height;i++){
		unsigned char*b=data+(i*4),k;
		k=b[0],b[0]=b[2],b[2]=k;
	}
	dsc->img_data=data;
	s=LV_RES_OK;
	done:
	if(rast)nsvgDeleteRasterizer(rast);
	if(img)nsvgDelete(img);
	if(file)free(file);
	return s;
	fail:
	if(data)free(data);
	s=LV_RES_INV;
	goto done;
}

static void decoder_close(lv_img_decoder_t*decoder __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	if(dsc->img_data)free((uint8_t*)dsc->img_data);
	dsc->img_data=NULL;
}

void svg_decoder_init(){
	lv_img_decoder_t*dec=lv_img_decoder_create();
	lv_img_decoder_set_info_cb(dec,decoder_info);
	lv_img_decoder_set_open_cb(dec,decoder_open);
	lv_img_decoder_set_close_cb(dec,decoder_close);
}
#endif
#endif

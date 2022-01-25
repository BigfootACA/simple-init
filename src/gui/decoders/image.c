/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include"list.h"
#include"confd.h"
#include"assets.h"
#include"logger.h"
#include"gui/image.h"
#include"gui/fsext.h"
#define TAG "image"
#define EGOTO(e) {e;goto fail;}

extern image_decoder image_decoder_bmp;
extern image_decoder image_decoder_png;
extern image_decoder image_decoder_svg;
extern image_decoder image_decoder_jpeg;
typedef int(*img_backend)(char*,unsigned char**,size_t*);
static int image_open_source_lvgl(char*,unsigned char**,size_t*);
static int image_open_source_native(char*,unsigned char**,size_t*);
static int image_open_source_assets(char*,unsigned char**,size_t*);

static img_backend img_backends[]={
	image_open_source_lvgl,
	image_open_source_native,
	image_open_source_assets,
	NULL
};

image_decoder*img_decoders[]={
	&image_decoder_bmp,
	#ifdef ENABLE_NANOSVG
	&image_decoder_svg,
	#endif
	#ifdef ENABLE_LODEPNG
	&image_decoder_png,
	#endif
	#ifdef ENABLE_LIBJPEG
	&image_decoder_jpeg,
	#endif
	NULL
};

static list*caches=NULL;
static time_t cache_time=5;

static int image_open_source_lvgl(char*path,unsigned char**data,size_t*len){
	lv_fs_res_t r;
	lv_fs_file_t f;
	uint32_t size=0,buf=0;
	if(strlen(path)<=3||path[0]=='/'||path[1]!=':')return -1;
	if((r=lv_fs_open(&f,path,LV_FS_MODE_RD))!=LV_FS_RES_OK)return -1;
	if((r=lv_fs_size(&f,&size)!=LV_FS_RES_OK))
		EGOTO(tlog_warn("get image %s size failed: %s",path,lv_fs_res_to_i18n_string(r)));
	if(size<=64)EGOTO(tlog_warn("invalid image %s",path));
	if(!(*data=malloc(size+1)))EGOTO();
	memset(*data,0,size+1);
	if((r=lv_fs_read(&f,*data,size,&buf))!=LV_FS_RES_OK)
		EGOTO(tlog_warn("read image %s failed: %s",path,lv_fs_res_to_i18n_string(r)));
	if(buf!=size)EGOTO(tlog_warn("read image %s failed",path));
	lv_fs_close(&f);
	*len=(size_t)size;
	return 0;
	fail:
	if(f.file_d)lv_fs_close(&f);
	if(*data)free(*data);
	*data=NULL,*len=0;
	return -1;
}

static int image_open_source_native(char*path,unsigned char**data,size_t*len){
	int fd=-1;
	struct stat st;
	if((fd=open(path,O_RDONLY,0644))<0)return -1;
	if(fstat(fd,&st)!=0)
		EGOTO(telog_warn("stat image %s failed",path));
	if(!S_ISREG(st.st_mode)||st.st_size<=64)
		EGOTO(tlog_warn("invalid image %s",path));
	if(!(*data=malloc(st.st_size+1)))EGOTO();
	memset(*data,0,st.st_size+1);
	if(read(fd,*data,st.st_size)!=st.st_size)
		EGOTO(telog_warn("read image %s failed",path));
	close(fd);
	*len=st.st_size;
	return 0;
	fail:
	if(fd>=0)close(fd);
	if(*data)free(*data);
	*data=NULL,*len=0;
	return -1;
}

static int image_open_source_assets(char*path,unsigned char**data,size_t*len){
	struct entry_file*f=rootfs_get_assets_file(path);
	if(!f)return -1;
	if(f->length<=64)return trlog_warn(-1,"invalid image %s",path);
	if(!(*data=malloc(f->length+1)))return -1;
	memset(*data,0,f->length+1);
	memcpy(*data,f->content,f->length);
	*len=f->length;
	return 0;
}

static int image_open_source(char*path,unsigned char**data,size_t*len){
	if(!path||!data||!len)return -1;
	for(size_t i=0;img_backends[i];i++){
		*len=0,*data=NULL;
		if(img_backends[i](path,data,len)==0)return 0;
	}
	*len=0,*data=NULL;
	return -1;
}

static image_decoder*image_get_decoder(char*path){
	if(!path)return NULL;
	char*e=NULL,*ext;
	image_decoder*d=NULL;
	if(!(ext=strrchr(path,'.')))return NULL;
	if(!*(++ext))return NULL;
	for(size_t i=0;(d=img_decoders[i]);i++){
		if(!d->types)continue;
		for(size_t t=0;(e=d->types[t]);t++)
			if(strcasecmp(ext,e)==0)return d;
	}
	return NULL;
}

static int image_free_data(void*d){
	image_data*c=d;
	if(c){
		if(c->pixels)free(c->pixels);
		free(c);
	}
	return 0;
}

static image_data*image_get_cache(char*path){
	if(!path||!caches)return NULL;
	list*l=list_first(caches);
	if(l)do{
		LIST_DATA_DECLARE(c,l,image_data*);
		if(!c||strcmp(path,c->path)!=0)continue;
		time(&c->last);
		return c;
	}while((l=l->next));
	return NULL;
}

int image_cache_gc(void){
	int cnt=0;
	time_t t;
	list*l=list_first(caches),*n;
	time(&t);
	if(l)do{
		n=l->next;
		LIST_DATA_DECLARE(c,l,image_data*);
		if(!c)continue;
		if(t-c->last>5){
			list_obj_del(&caches,l,image_free_data);
			cnt++;
		}
	}while((l=n));
	return cnt;
}

void image_cache_clean(void){
	list_free_all(caches,image_free_data);
	caches=NULL;
}

static void image_add_cache(image_data*img){
	if(!img||!img->pixels||!img->path[0])return;
	list*l=list_first(caches),*n;
	if(l)do{
		n=l->next;
		LIST_DATA_DECLARE(c,l,image_data*);
		if(!c)continue;
		if(c==img)return;
		if(strcmp(img->path,c->path)==0)
			list_obj_del(&caches,l,image_free_data);
	}while((l=n));
	time(&img->last);
	list_obj_add_new_notnull(&caches,img);
}

static image_data*image_decode(char*path){
	size_t len=0;
	image_data*img=NULL;
	image_decoder*d=NULL;
	unsigned char*data=NULL;
	if(!(d=image_get_decoder(path))||!d->decode_cb)return NULL;
	if(image_open_source(path,&data,&len)==-1||!data||len<=0)goto fail;
	if(!(img=malloc(sizeof(image_data))))goto fail;
	memset(img,0,sizeof(image_data));
	strncpy(img->path,path,sizeof(img->path)-1);
	if(d->decode_cb(data,len,img)!=0)goto fail;
	if(img->width<=0||img->height<=0||!img->pixels)goto fail;
	free(data);
	return img;
	fail:
	image_free_data(img);
	if(data)free(data);
	return NULL;
}

static image_data*image_get(char*path){
	image_data*img=NULL;
	if(!path)return NULL;
	if((img=image_get_cache(path)))return img;
	if(!(img=image_decode(path)))return NULL;
	image_add_cache(img);
	return img;
}

static lv_res_t decoder_info(lv_img_decoder_t*d __attribute__((unused)),const void*src,lv_img_header_t*m){
	image_data*img;
	if(lv_img_src_get_type(src)!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(!(img=image_get((char*)src)))return LV_RES_INV;
	m->h=img->height;
	m->w=img->width;
	m->cf=img->format;
	m->always_zero=0;
	return LV_RES_OK;
}

static lv_res_t decoder_open(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	image_data*img;
	if(dsc->src_type!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(!(img=image_get((char*)dsc->src)))return LV_RES_INV;
	dsc->img_data=img->pixels;
	return LV_RES_OK;
}

static void decoder_close(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	image_cache_gc();
	dsc->img_data=NULL;
}

void image_set_cache_time(time_t time){
	cache_time=time;
	confd_set_integer("gui.image_cache_time",time);
}

void image_decoder_init(){
	lv_img_decoder_t*dec=lv_img_decoder_create();
	lv_img_decoder_set_info_cb(dec,decoder_info);
	lv_img_decoder_set_open_cb(dec,decoder_open);
	lv_img_decoder_set_close_cb(dec,decoder_close);
	cache_time=confd_get_integer("gui.image_cache_time",5);
}
#endif

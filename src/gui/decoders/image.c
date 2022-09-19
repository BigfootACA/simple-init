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
#include<sys/stat.h>
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"gui/image.h"
#include"gui/icon_theme.h"
#include"filesystem.h"
#define TAG "image"

extern image_decoder image_decoder_bmp;
extern image_decoder image_decoder_png;
extern image_decoder image_decoder_svg;
extern image_decoder image_decoder_jpeg;
extern image_decoder image_decoder_stb;

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
	#ifdef ENABLE_STB
	&image_decoder_stb,
	#endif
	NULL
};

static list*caches=NULL;
static time_t cache_time=5;
static long cache_hit=0,cache_miss=0,load_fail=0;

image_decoder*image_get_decoder(char*ext){
	char*e=NULL;
	image_decoder*d=NULL;
	static list*unsupports=NULL;
	if(!ext)return NULL;
	for(size_t i=0;(d=img_decoders[i]);i++){
		if(!d->types)continue;
		for(size_t t=0;(e=d->types[t]);t++)
			if(strcasecmp(ext,e)==0)return d;
	}
	if(list_search_string(unsupports,ext)){
		tlog_warn("unsupported image decoder for type %s",ext);
		list_obj_add_new_strdup(&unsupports,ext);
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

static bool load_icon(
	struct icon_theme*theme,
	struct icon_theme_search_path*s,
	char*type,char*path,
	unsigned char**data,
	size_t*len,
	image_decoder**d
){
	int r=0;
	fsh*f=NULL;
	if(!path||!data||!len||!d)return false;
	if(!type){
		if(!(type=strrchr(path,'.'))||strchr(type,'/')){
			tlog_warn("'%s' image type not set",path);
			return false;
		}
		type++;
	}
	if(!(*d=image_get_decoder(type))||!(*d)->decode_cb)return false;
	r=fs_open(s?s->folder:theme->root,&f,path,FILE_FLAG_READ);
	if(r!=0)return false;
	r=fs_read_all(f,(void**)data,len);
	fs_close(&f);
	return r==0;
}

static bool load_search_path(
	struct icon_theme*theme,
	struct icon_theme_search_path*s,
	char*path,
	unsigned char**data,
	size_t*len,
	image_decoder**d
){
	list*l;
	bool ret=false;
	if(!theme||!path||!data||!len||!d)return false;
	if(path[0]=='@'){
		path++;
		if((l=list_first(theme->name_mapping)))do{
			LIST_DATA_DECLARE(icon,l,struct icon_theme_name_mapping*);
			if(!icon)continue;
			if(icon->name&&strcmp(path,icon->name)!=0)continue;
			if(icon->regex&&regexp_exec(icon->regex,path,NULL,0)!=0)continue;
			if(icon->search&&(!s||!s->id||strcmp(icon->search,s->id)!=0))continue;
			if(load_icon(theme,s,icon->type,icon->path,data,len,d))return true;
		}while((l=l->next));
	}else ret=load_icon(theme,s,NULL,path,data,len,d);
	return ret;
}

static bool load_theme(
	char*path,
	unsigned char**data,
	size_t*len,
	image_decoder**d
){
	list*l,*s;
	static bool no_any=false;
	if(!path||!data||!len||!d)return false;
	*len=0,*data=NULL,*d=NULL;
	if(!gui_icon_themes){
		if(!no_any)tlog_warn("no any icon themes found");
		no_any=true;
		return false;
	}
	if((l=list_first(gui_icon_themes)))do{
		LIST_DATA_DECLARE(theme,l,struct icon_theme*);
		if(!theme)continue;
		if((s=list_first(theme->search_path)))do{
			LIST_DATA_DECLARE(search,s,struct icon_theme_search_path*);
			if(!search)continue;
			if(load_search_path(
				theme,search,
				path,data,len,d
			))return true;
		}while((s=s->next));
		if(load_search_path(
			theme,NULL,
			path,data,len,d
		))return true;
	}while((l=l->next));
	tlog_warn("icon %s not found",path);
	return false;
}

static image_data*image_decode(char*path){
	size_t len=0;
	image_data*img=NULL;
	image_decoder*d=NULL;
	unsigned char*data=NULL;
	if(!load_theme(path,&data,&len,&d))goto done;
	if(!data||len<=0||!d||!d->decode_cb)goto done;
	if(!(img=malloc(sizeof(image_data))))goto done;
	memset(img,0,sizeof(image_data));
	strncpy(img->path,path,sizeof(img->path)-1);
	if(d->decode_cb(data,len,img)!=0)goto done;
	if(img->width<=0||img->height<=0||!img->pixels)goto done;
	free(data);
	return img;
	done:
	image_free_data(img);
	if(data)free(data);
	return NULL;
}

static image_data*image_get(char*path){
	image_data*img=NULL;
	if(!path)return NULL;
	if((img=image_get_cache(path))){
		cache_hit++;
		return img;
	}
	if(!(img=image_decode(path))){
		load_fail++;
		return NULL;
	}
	cache_miss++;
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
	icon_theme_load_from_confd();
}

long image_get_cache_hits(){return cache_hit;}
long image_get_cache_misses(){return cache_miss;}
long image_get_load_fails(){return load_fail;}

void image_print_stat(){
	tlog_debug(
		"cache hit: %ld (%0.2f%%), miss: %ld(%0.2f%%), fail: %ld(%0.2f%%)",
		image_get_cache_hits(),image_get_cache_hit_percent(),
		image_get_cache_misses(),image_get_cache_miss_percent(),
		image_get_load_fails(),image_get_load_fail_percent()
	);
}
#endif

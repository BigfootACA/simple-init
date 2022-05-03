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
#ifdef ENABLE_UEFI
#include<Library/MemoryAllocationLib.h>
#include<Guid/FileInfo.h>
#include"uefi.h"
#include"locate.h"
#endif
#include"list.h"
#include"confd.h"
#include"assets.h"
#include"logger.h"
#include"gui/image.h"
#include"gui/fsext.h"
#define TAG "image"

#define IMG_RES1 _PATH_USR "/share/pixmaps/simple-init"
#define IMG_RES2 _PATH_USR"/share/pixmaps/mime"
extern image_decoder image_decoder_bmp;
extern image_decoder image_decoder_png;
extern image_decoder image_decoder_svg;
extern image_decoder image_decoder_jpeg;
extern image_decoder image_decoder_stb;
typedef int(*img_backend)(char*,unsigned char**,size_t*);
static int image_open_source_lvgl(char*,unsigned char**,size_t*);
#ifdef ENABLE_UEFI
static int image_open_source_locate(char*,unsigned char**,size_t*);
#else
static int image_open_source_native(char*,unsigned char**,size_t*);
#endif
static int image_open_source_assets(char*,unsigned char**,size_t*);

static img_backend img_backends[]={
	image_open_source_lvgl,
	#ifdef ENABLE_UEFI
	image_open_source_locate,
	#else
	image_open_source_native,
	#endif
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
	#ifdef ENABLE_STB
	&image_decoder_stb,
	#endif
	NULL
};

static list*caches=NULL;
static time_t cache_time=5;
static long cache_hit=0,cache_miss=0,load_fail=0;

static int image_open_source_lvgl(char*path,unsigned char**data,size_t*len){
	lv_fs_res_t r;
	lv_fs_file_t f;
	uint32_t size=0,buf=0;
	if(strlen(path)<=3||path[0]=='/'||path[1]!=':')return -1;
	if((r=lv_fs_open(&f,path,LV_FS_MODE_RD))!=LV_FS_RES_OK)return -1;
	if((r=lv_fs_size(&f,&size)!=LV_FS_RES_OK))
		EDONE(tlog_warn("get image %s size failed: %s",path,lv_fs_res_to_i18n_string(r)));
	if(size<=64)EDONE(tlog_warn("invalid image %s",path));
	if(!(*data=malloc(size+1)))EDONE();
	memset(*data,0,size+1);
	if((r=lv_fs_read(&f,*data,size,&buf))!=LV_FS_RES_OK)
		EDONE(tlog_warn("read image %s failed: %s",path,lv_fs_res_to_i18n_string(r)));
	if(buf!=size)EDONE(tlog_warn("read image %s failed",path));
	lv_fs_close(&f);
	*len=(size_t)size;
	return 0;
	done:
	if(f.file_d)lv_fs_close(&f);
	if(*data)free(*data);
	*data=NULL,*len=0;
	return -1;
}

#ifdef ENABLE_UEFI
static int image_open_source_locate(char*path,unsigned char**data,size_t*len){
	EFI_STATUS st;
	UINTN infos=0,read;
	EFI_FILE_INFO*info=NULL;
	locate_ret*loc=AllocateZeroPool(sizeof(locate_ret));
	if(!loc||path[0]=='/'||!boot_locate_quiet(loc,path))goto done;
	if(loc->type!=LOCATE_FILE)
		EDONE(telog_warn("unsupported locate type for %s",path));

	// get file info
	st=efi_file_get_file_info(loc->file,&infos,&info);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get file info of %s failed: %s",
		path,efi_status_to_string(st)
	));
	if(info->FileSize<=0)
		EDONE(tlog_warn("file %s size too small",path));
	if(info->FileSize>=0x8000000)
		EDONE(tlog_warn("file %s size too big",path));
	if(info->Attribute&EFI_FILE_DIRECTORY)
		EDONE(tlog_warn("file %s is a directory",path));
	*len=info->FileSize;
	FreePool(info);
	info=NULL;

	// read file to memory
	read=*len;
	if(!(*data=malloc(read+1)))
		EDONE(tlog_error("allocate pool failed"));
	memset(*data,0,read+1);
	st=loc->file->Read(loc->file,&read,*data);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"read file %s failed: %s",
		path,efi_status_to_string(st)
	));
	if(read!=*len)EDONE(tlog_warn(
		"read file size %s not match %llu != %zu",
		path,(unsigned long long)read,*len
	));
	loc->file->Close(loc->file);
	FreePool(loc);
	return 0;
	done:
	if(info)FreePool(info);
	if(loc->file)loc->file->Close(loc->file);
	if(loc)FreePool(loc);
	if(*data)free(*data);
	*data=NULL,*len=0;
	return -1;
}
#else
static int image_open_source_native(char*path,unsigned char**data,size_t*len){
	int fd=-1;
	struct stat st;
	if((fd=open(path,O_RDONLY,0644))<0)return -1;
	if(fstat(fd,&st)!=0)
		EDONE(telog_warn("stat image %s failed",path));
	if(!S_ISREG(st.st_mode)||st.st_size<=64)
		EDONE(tlog_warn("invalid image %s",path));
	if(!(*data=malloc(st.st_size+1)))EDONE();
	memset(*data,0,st.st_size+1);
	if(read(fd,*data,st.st_size)!=st.st_size)
		EDONE(telog_warn("read image %s failed",path));
	close(fd);
	*len=st.st_size;
	return 0;
	done:
	if(fd>=0)close(fd);
	if(*data)free(*data);
	*data=NULL,*len=0;
	return -1;
}
#endif

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

static int image_open_source_try(char*path,unsigned char**data,size_t*len){
	int r=0,tr=0;
	char rpath[PATH_MAX];
	if(!path||!path[0]||!data||!len)return -1;
	do{
		if(*data)free(*data);
		*data=NULL,*len=0;
		memset(rpath,0,sizeof(rpath));
		switch(tr){
			case 0:strncpy(rpath,path,sizeof(rpath)-1);break;
			case 1:snprintf(rpath,sizeof(rpath)-1,IMG_RES1"/%s",path);break;
			case 2:snprintf(rpath,sizeof(rpath)-1,IMG_RES2"/%s",path);break;
			default:return r;
		}
		r=image_open_source(rpath,data,len);
		tr++;
	}while(r==-1||!(*data)||(*len)<=0);
	return r;
}

static image_decoder*image_get_decoder(char*path){
	if(!path)return NULL;
	static list*unsupports=NULL;
	char*e=NULL,*ext;
	image_decoder*d=NULL;
	if(!(ext=strrchr(path,'.')))return NULL;
	if(!*(++ext))return NULL;
	for(size_t i=0;(d=img_decoders[i]);i++){
		if(!d->types)continue;
		for(size_t t=0;(e=d->types[t]);t++)
			if(strcasecmp(ext,e)==0)return d;
	}
	if(!list_search_string(unsupports,path)){
		tlog_warn("unsupported image decoder for type %s (%s)",ext,path);
		list_obj_add_new_strdup(&unsupports,path);
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
	if(image_open_source_try(path,&data,&len)==-1||!data||len<=0)goto done;
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

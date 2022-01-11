#ifdef ENABLE_GUI
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include"lvgl.h"
#include"assets.h"

static lv_res_t decoder_info(lv_img_decoder_t*d __attribute__((unused)),const void* src,lv_img_header_t*i){
	int fd;
	char*fn=(char*)src,hdr[54];
	uint16_t bpp;
	uint32_t w,h;
	if(lv_img_src_get_type(src)!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(fn+strlen(fn)-3,"bmp")!=0)return LV_RES_INV;
	if((fd=open(fn,O_RDONLY,0644))>=0){
		read(fd,hdr,54);
		close(fd);
	}else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=54)return LV_RES_INV;
		memcpy(hdr,f->content,54);
	}
	memcpy(&w,hdr+18,4);
	memcpy(&h,hdr+22,4);
	memcpy(&bpp,hdr+28,2);
	i->w=w,i->h=h,i->always_zero=0;
	i->cf=bpp==32?LV_IMG_CF_TRUE_COLOR_ALPHA:LV_IMG_CF_TRUE_COLOR;
	return LV_RES_OK;
}

static lv_res_t decoder_open(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	int fd,row;
	size_t len;
	uint16_t bpp;
	lv_color32_t*cs=NULL;
	lv_coord_t pw,ph,off;
	char*fn=(char*)dsc->src,*img=NULL;
	if(dsc->src_type!=LV_IMG_SRC_FILE)return LV_RES_INV;
	if(strcmp(fn+strlen(fn)-3,"bmp")!=0)return LV_RES_INV;
	if((fd=open(fn,O_RDONLY,0644))>=0){
		struct stat st;
		if(fstat(fd,&st)!=0)goto fail;
		if(!(img=malloc(st.st_size)))goto fail;
		ssize_t r=read(fd,img,st.st_size);
		close(fd);
		if(r!=st.st_size)goto fail;
		len=st.st_size;
	}else{
		struct entry_file*f=rootfs_get_assets_file(fn);
		if(!f||f->length<=54)return LV_RES_INV;
		img=f->content,len=f->length;
	}
	if(img[0]!=0x42||img[1]!=0x4D)goto fail;
	memcpy(&off,img+10,4);
	memcpy(&pw,img+18,4);
	memcpy(&ph,img+22,4);
	memcpy(&bpp,img+28,2);
	row=(bpp*pw)/8;
	if(!(cs=malloc(pw*ph*sizeof(lv_color32_t))))goto fail;
	uint8_t*data=(uint8_t*)img+off;
	for(lv_coord_t h=0;h<ph;h++){
		lv_coord_t rh=ph-h-1;
		uint8_t*hs=data+(row*h);
		for(lv_coord_t w=0;w<pw;w++){
			uint8_t*ws=hs+(w*(bpp/8));
			lv_color32_t*c=cs+(w+(rh*pw));
			if((char*)ws+2>img+len)goto fail;
			c->ch.red=ws[2],c->ch.green=ws[1],c->ch.blue=ws[0];
			c->ch.alpha=bpp==32?ws[3]:0xFF;
		}
	}
	dsc->img_data=(uint8_t*)cs;
	return LV_RES_OK;
	fail:
	if(cs)free(cs);
	if(fd>0){
		if(img)free(img);
		else close(fd);
	}
	return LV_RES_INV;
}

static void decoder_close(lv_img_decoder_t*d __attribute__((unused)),lv_img_decoder_dsc_t*dsc){
	if(dsc->img_data)free((uint8_t*)dsc->img_data);
	dsc->img_data=NULL;
}

void bmp_decoder_init(){
	lv_img_decoder_t*dec=lv_img_decoder_create();
	lv_img_decoder_set_info_cb(dec,decoder_info);
	lv_img_decoder_set_open_cb(dec,decoder_open);
	lv_img_decoder_set_close_cb(dec,decoder_close);
}
#endif

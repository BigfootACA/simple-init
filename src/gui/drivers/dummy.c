/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<string.h>
#include<stdlib.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/guidrv.h"
#define TAG "dummy"
#define WIDTH  540
#define HEIGHT 960

static int ww=WIDTH,hh=HEIGHT;

static void dummy_flush(
	lv_disp_drv_t*disp_drv,
	const lv_area_t*area __attribute__((unused)),
	lv_color_t*color_p __attribute__((unused))
){
	lv_disp_flush_ready(disp_drv);
}

static void dummy_apply_mode(){
	int cnt=0;
	char*name=NULL;
	struct display_mode*mode=NULL;
	for(cnt=0;builtin_modes[cnt].name[0];cnt++);
	name=confd_get_string("gui.mode",NULL);
	if(!name){
		char*n=getenv("GUIMODE");
		if(n)name=strdup(n);
	}
	if(!name)goto done;
	if(cnt<=0)EDONE(tlog_warn("no any modes found"));
	for(int i=0;i<cnt;i++)
		if(strcasecmp(name,builtin_modes[i].name)==0)
			mode=&builtin_modes[i];
	if(!mode)EDONE(tlog_warn("mode %s not found",name));
	tlog_info("set mode to %s",name);
	ww=mode->width;
	hh=mode->height;
	done:
	if(name)free(name);
}

static int dummy_register(){
	dummy_apply_mode();
	size_t s=ww*hh;
	static lv_color_t*buf=NULL;
	static lv_disp_draw_buf_t disp_buf;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,s);
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=dummy_flush;
	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
	disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}
	tlog_debug("screen resolution: %dx%d",ww,hh);
	lv_disp_drv_register(&disp_drv);
	return 0;
}

static void dummy_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}

static int dummy_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	for(*cnt=0;builtin_modes[*cnt].name[0];(*cnt)++);
	if(*cnt<=0)return 0;
	size_t size=((*cnt)+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memcpy(*modes,builtin_modes,size);
	return 0;
}

struct gui_driver guidrv_dummy={
	.name="dummy",
	.drv_register=dummy_register,
	.drv_getsize=dummy_get_sizes,
	.drv_get_modes=dummy_get_modes,
};
#endif

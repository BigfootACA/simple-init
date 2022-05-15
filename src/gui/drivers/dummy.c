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
#include"logger.h"
#include"version.h"
#include"gui/guidrv.h"
#define TAG "uefigop"
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

static int dummy_register(){
	size_t s=ww*hh;
	static lv_color_t*buf=NULL;
	static lv_disp_buf_t disp_buf;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_buf_init(&disp_buf,buf,NULL,s);
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.buffer=&disp_buf;
	disp_drv.flush_cb=dummy_flush;
	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
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

struct gui_driver guidrv_dummy={
	.name="dummy",
	.drv_register=dummy_register,
	.drv_getsize=dummy_get_sizes,
};
#endif

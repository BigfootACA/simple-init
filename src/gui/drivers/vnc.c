/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_VNCSERVER
#include<stdio.h>
#include<rfb/rfb.h>
#include<rfb/keysym.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"version.h"
#include"gui/guidrv.h"
#define TAG "vnc"
#define DPI    200

static rfbScreenInfoPtr server=NULL;
static uint32_t*fb=NULL;
static uint32_t kbd_key=0;
static int ptr_x=0,ptr_y=0;
static lv_indev_state_t kbd_state=LV_INDEV_STATE_REL;
static lv_indev_state_t ptr_state=LV_INDEV_STATE_REL;
static lv_indev_t*kbd_dev,*ptr_dev;
static uint32_t ww=540,hh=960;
static lv_color_t buf[846000];
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;

static inline uint32_t swap(uint32_t x){
	return
	       ((x&0xff000000))|
	       ((x&0x00ff0000)>>16)|
	       ((x&0x0000ff00))|
	       ((x&0x000000ff)<<16);
}

static void vnc_flush(lv_disp_drv_t*drv,const lv_area_t*area,lv_color_t*color_p){
	for(int32_t y=area->y1;y<=area->y2;y++)
		for(int32_t x=area->x1;x<=area->x2;x++)
			fb[y*ww+x]=swap(*(uint32_t*)(color_p++));
	rfbMarkRectAsModified(server,area->x1,area->y1,area->x2+1,area->y2+1);
	lv_disp_flush_ready(drv);
}

static void vnc_read(lv_indev_drv_t*drv,lv_indev_data_t*data){
	if(!drv||!data)return;
	switch(drv->type){
		case LV_INDEV_TYPE_POINTER:
			if(!ptr_dev->cursor)lv_indev_set_cursor(ptr_dev,gui_cursor);
			data->point.x=ptr_x;
			data->point.y=ptr_y;
			data->state=ptr_state;
		break;
		case LV_INDEV_TYPE_KEYPAD:
			data->key=kbd_key;
			data->state=kbd_state;
		break;
		default:;
	}
}

static void vnc_key(rfbBool down,rfbKeySym k,rfbClientPtr cl __attribute__((unused))){
	bool b=lv_group_get_editing(gui_grp);
	switch(k){
		case XK_Up:        kbd_key=b?LV_KEY_UP:LV_KEY_PREV;break;
		case XK_Left:      kbd_key=b?LV_KEY_LEFT:LV_KEY_PREV;break;
		case XK_Down:      kbd_key=b?LV_KEY_DOWN:LV_KEY_NEXT;break;
		case XK_Right:     kbd_key=b?LV_KEY_RIGHT:LV_KEY_NEXT;break;
		case XK_Return:    kbd_key=LV_KEY_ENTER;break;
		case XK_BackSpace: kbd_key=LV_KEY_BACKSPACE;break;
		case XK_Home:      kbd_key=LV_KEY_HOME;break;
		case XK_End:       kbd_key=LV_KEY_END;break;
		case XK_Delete:    kbd_key=LV_EVENT_DELETE;break;
		case XK_Escape:    kbd_key=LV_KEY_ESC;break;
		default:           kbd_key=k;break;
	}
	kbd_state=down?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
}

static void vnc_ptr(int btn,int x,int y,rfbClientPtr cl __attribute__((unused))){
	ptr_x=x,ptr_y=y;
	ptr_state=btn!=0?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
}

static void vnc_log_normal(const char*text,...){
	static char buff[BUFSIZ];
	va_list va;
	va_start(va,text);
	vsnprintf(buff,BUFSIZ-1,text,va);
	va_end(va);
	logger_print(LEVEL_DEBUG,TAG,buff);
}

static void vnc_log_warn(const char*text,...){
	static char buff[BUFSIZ];
	va_list va;
	va_start(va,text);
	vsnprintf(buff,BUFSIZ-1,text,va);
	va_end(va);
	logger_print(LEVEL_WARNING,TAG,buff);
}

static void vnc_apply_mode(){
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

static int vnc_register(){
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,sizeof(buf));
	lv_disp_drv_init(&disp_drv);
	size_t bpp=sizeof(*fb);
	vnc_apply_mode();
	if(!(fb=malloc(ww*hh*bpp)))return -1;
	if(!(server=rfbGetScreen(0,NULL,ww,hh,8,3,bpp))){
		free(fb);
		return -1;
	}

	server->desktopName=NAME" "VERSION;
	server->frameBuffer=(void*)fb;
	server->alwaysShared=true;
	server->httpDir=NULL;
	server->port=confd_get_integer("gui.vnc_port",5900);
	server->kbdAddEvent=vnc_key;
	server->ptrAddEvent=vnc_ptr;
	rfbLog=vnc_log_normal;
	rfbErr=vnc_log_warn;

	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=vnc_flush;
	disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}

	lv_disp_drv_register(&disp_drv);

	tlog_notice("screen resolution: %dx%d",ww,hh);
	rfbInitServer(server);
	rfbRunEventLoop(server,-1,1);

	return 0;
}

static int vnc_input_init(){
	static lv_indev_drv_t kbd_in,ptr_in;
	lv_indev_drv_init(&kbd_in);
	lv_indev_drv_init(&ptr_in);
	kbd_in.type=LV_INDEV_TYPE_KEYPAD;
	ptr_in.type=LV_INDEV_TYPE_POINTER;
	kbd_in.read_cb=vnc_read;
	ptr_in.read_cb=vnc_read;
	kbd_dev=lv_indev_drv_register(&kbd_in);
	ptr_dev=lv_indev_drv_register(&ptr_in);
	lv_indev_set_group(kbd_dev,gui_grp);
	lv_indev_set_group(ptr_dev,gui_grp);
	return 0;
}

static void vnc_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}

static bool vnc_cansleep(){
	return false;
}

static int vnc_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	for(*cnt=0;builtin_modes[*cnt].name[0];(*cnt)++);
	if(*cnt<=0)return 0;
	size_t size=((*cnt)+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memcpy(*modes,builtin_modes,size);
	return 0;
}

static void vnc_exit(){
	rfbShutdownServer(server,true);
	free(fb);
	rfbScreenCleanup(server);
	fb=NULL,server=NULL;
}

struct input_driver indrv_vnc={
	.name="vnc-input",
	.compatible={
		"vnc",
		NULL
	},
	.drv_register=vnc_input_init,
};
struct gui_driver guidrv_vnc={
	.name="vnc",
	.drv_register=vnc_register,
	.drv_getsize=vnc_get_sizes,
	.drv_cansleep=vnc_cansleep,
	.drv_get_modes=vnc_get_modes,
	.drv_exit=vnc_exit,
};
#endif
#endif
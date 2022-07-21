/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_MICROHTTPD
#ifdef ENABLE_WEBSOCKET
#include<zlib.h>
#include<stb_image_write.h>
#include"frame_protocol.h"
#include"gui_http.h"
#include"http.h"

void gui_http_send_frame(
	enum frame_pixel mode,
	uint64_t sx,uint64_t sy,
	uint64_t dx,uint64_t dy,
	const void*frame
){
	int r=1;
	const void*src;
	size_t ss,size;
	static frame_data*fd=NULL;
	static size_t ds=sizeof(frame_data),max=0;
	uint32_t t=lv_tick_get();
	if(!state.disp_ws||!gui_http_init_img_ctx())return;
	uint8_t bsp=pixel_size(mode);
	uint64_t w=dx-sx+1,h=dy-sy+1;
	MUTEX_LOCK(gui_http_ctx.lock);
	switch(state.disp_type){
		case TYPE_RAW:
			ss=w*h*bsp,src=frame;
			break;
		case TYPE_BMP:
			gui_http_ctx.last_pos=0;
			r=stbi_write_bmp_to_func(gui_http_img_write,&gui_http_ctx,w,h,bsp,frame);
			src=gui_http_ctx.buf,ss=gui_http_ctx.last_pos;
			break;
		case TYPE_JPG:
			gui_http_ctx.last_pos=0;
			r=stbi_write_jpg_to_func(gui_http_img_write,&gui_http_ctx,w,h,bsp,frame,90);
			src=gui_http_ctx.buf,ss=gui_http_ctx.last_pos;
			break;
		case TYPE_PNG:
			gui_http_ctx.last_pos=0;
			r=stbi_write_png_to_func(gui_http_img_write,&gui_http_ctx,w,h,bsp,frame,w*bsp);
			src=gui_http_ctx.buf,ss=gui_http_ctx.last_pos;
			break;
		default:MUTEX_UNLOCK(gui_http_ctx.lock);return;
	}
	if(r<=0)EDONE(tlog_warn("generate frame failed: %d",r));
	size=ds+ss;
	if(state.frame_compress){
		size_t ns=compressBound(ss);
		if(ns>ss)size=ds+ns;
	}
	if(size>max){
		if(fd){
			void*f=realloc(fd,size);
			if(!f)EDONE(telog_warn("realloc buffer failed"));
			fd=f;
		}else if((fd=malloc(size))){
			memset(fd,0,max);
			memcpy(
				fd->magic,FRAME_MAGIC,
				sizeof(fd->magic)
			);
			fd->version=FRAME_VERSION;
		}else EDONE(telog_warn("malloc buffer failed"));
		tlog_debug("resize buffer to %zu bytes",size);
		max=size;
	}
	if(state.frame_compress){
		uLong len=size-ds;
		int i=compress2((Bytef*)fd->frame,&len,src,ss,3);
		if(i!=Z_OK)EDONE(tlog_warn("zlib compress failed: %d",i));
		fd->size=len,fd->src_size=ss,size=ds+len;
	}else{
		memcpy(fd->frame,src,ss);
		fd->size=ss,fd->src_size=ss;
	}
	MUTEX_UNLOCK(gui_http_ctx.lock);
	fd->pixel=mode;
	fd->compressed=state.frame_compress;
	fd->src_x=sx,fd->src_y=sy;
	fd->dst_x=dx,fd->dst_y=dy;
	fd->type=state.disp_type;
	fd->gen_time=time(NULL);
	uint32_t e=lv_tick_get();
	fd->cost_time=e-t;
	state.bytes+=size;
	ws_send_payload(state.disp_ws,"FRAME",fd,size);
	struct timespec ts={.tv_sec=30,.tv_nsec=0};
	sem_timedwait(&state.disp_wait,&ts);
	return;
	done:
	MUTEX_UNLOCK(gui_http_ctx.lock);
	return;
}

void gui_http_send_frame_area(
	const lv_area_t*area,
	const lv_color_t*frame
){
	gui_http_send_frame(
		PIXEL_BGRA32,
		area->x1,area->y1,
		area->x2,area->y2,
		frame
	);
}

int gui_http_disp_ws_cmd_flush(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	if(state.disp_ws==d)
		sem_post(&state.disp_wait);
	return 0;
}

int gui_http_disp_ws_cmd_size(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	if(state.disp_ws!=d)return 2;
	ws_send_json_payload(d,"size",gui_http_get_size_json());
	return 0;
}

int gui_http_disp_ws_cmd_inv(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	if(state.disp_ws!=d)return 2;
	MUTEX_LOCK(gui_lock);
	lv_obj_invalidate(lv_scr_act());
	MUTEX_UNLOCK(gui_lock);
	return ws_send_cmd_r(0,d,"OKAY");
}

int gui_http_disp_ws_cmd_full_screen(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	if(state.disp_ws!=d)return 2;
	gui_http_send_frame(
		PIXEL_RGB24,0,0,
		state.ww-1,
		state.hh-1,
		state.buffer
	);
	return 0;
}

int gui_http_disp_ws_cmd_set_type(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	char*m=(char*)cmd->cmd;
	if(strncmp(m,"TYPE:",5)!=0)return 0;
	m+=5;
	if(strcmp(m,"JPG")==0)state.disp_type=TYPE_JPG;
	else if(strcmp(m,"PNG")==0)state.disp_type=TYPE_PNG;
	else if(strcmp(m,"BMP")==0)state.disp_type=TYPE_BMP;
	else if(strcmp(m,"RAW")==0)state.disp_type=TYPE_RAW;
	else return ws_send_cmd_r(1,d,"INVAL");
	return ws_send_cmd_r(1,d,"OKAY");
}

int gui_http_disp_ws_cmd_set_compress(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	char*m=(char*)cmd->cmd;
	if(strncmp(m,"COMP:",5)!=0)return 0;
	m+=5;
	if(strcmp(m,"FALSE")==0)state.frame_compress=false;
	else if(strcmp(m,"TRUE")==0)state.frame_compress=true;
	else return ws_send_cmd_r(1,d,"INVAL");
	return ws_send_cmd_r(1,d,"OKAY");
}

int gui_http_disp_ws_cmd_dragon_egg(
	struct ws_cmd_proc*cmd __attribute__((unused)),
	struct http_hand_websocket_data*d,
	char**dd __attribute__((unused)),
	size_t*dl __attribute__((unused))
){
	if(state.disp_ws!=d)return 2;
	ws_print_payload(d,"GOD","OH HAI SOPHON\n");
	return 0;
}

int gui_http_disp_ws_establish(
	struct http_hand_websocket_data*d
){
	if(state.disp_ws)return -1;
	tlog_debug("new display stream web socket connection");
	sem_init(&state.disp_wait,0,0);
	state.disp_ws=d;
	return 0;
}

int gui_http_disp_ws_disconnect(
	struct http_hand_websocket_data*d __attribute__((unused))
){
	tlog_debug("display stream web socket connection lost");
	if(state.disp_ws){
		sem_post(&state.disp_wait);
		usleep(50000);
		sem_destroy(&state.disp_wait);
	}
	state.disp_ws=NULL;
	return 0;
}

#endif
#endif
#endif

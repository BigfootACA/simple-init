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
#ifdef ENABLE_STB
#include<stb_image_write.h>
#endif
#include"frame_protocol.h"
#include"gui_http.h"
#include"http.h"

void gui_http_img_write(void*c __attribute__((unused)),void*data,int size){
	if(!gui_http_ctx.buf)return;
	size_t ns=gui_http_ctx.last_pos+size;
	if(ns>gui_http_ctx.mem_size){
		size_t s=gui_http_ctx.mem_size+MAX(65536,size);
		void*b=realloc(gui_http_ctx.buf,s);
		if(!b)return;
		gui_http_ctx.buf=b,gui_http_ctx.mem_size=s;
	}
	memcpy(gui_http_ctx.buf+gui_http_ctx.last_pos,data,size);
	gui_http_ctx.last_pos=ns;
}

bool gui_http_init_img_ctx(){
	if(gui_http_ctx.buf)return true;
	gui_http_ctx.mem_size=65536;
	if(!(gui_http_ctx.buf=malloc(gui_http_ctx.mem_size))){
		telog_warn("allocate buffer failed");
		return false;
	}
	MUTEX_INIT(gui_http_ctx.lock);
	return true;
}

#ifdef ENABLE_STB
static bool gen_jpeg_frame(void**buf,size_t*size,uint32_t*ft){
	static uint32_t last=0;
	if(!state.buffer)return false;
	if(!gui_http_init_img_ctx())return -1;
	gui_http_ctx.last_pos=0;
	uint32_t cur=lv_tick_get();
	uint32_t e=cur-last;
	MUTEX_LOCK(gui_http_ctx.lock);
	int r=stbi_write_jpg_to_func(
		gui_http_img_write,NULL,
		state.ww,state.hh,
		3,state.buffer,70
	);
	MUTEX_UNLOCK(gui_http_ctx.lock);
	if(ft)*ft=lv_tick_get()-cur;
	if(e<1000/30)usleep(e*1000);
	last=cur;
	if(r==0){
		tlog_warn("write jpeg failed: %d",r);
		return false;
	}
	if(buf)*buf=gui_http_ctx.buf;
	if(size)*size=gui_http_ctx.last_pos;
	return true;
}

static ssize_t stream_jpeg(
	void*cls __attribute__((unused)),
	uint64_t pos __attribute__((unused)),
	char*buf,
	size_t max
){
	size_t ws=0;
	static void*img=NULL;
	static size_t bs=0,bp=0;
	if(bp>=bs||!img){
		uint32_t ft=0;
		img=NULL,bp=0;
		if(!gen_jpeg_frame(&img,&bs,&ft))return -1;
		size_t hdr=snprintf(
			buf,max,"--"_BOUNDARY"\r\n"
			MHD_HTTP_HEADER_CONTENT_TYPE": image/jpeg\r\n"
			MHD_HTTP_HEADER_CONTENT_LENGTH": %zu\r\n"
			"X-Frame-Time: %u\r\n\r\n",bs,ft
		);
		ws+=hdr,bp=0;
	}
	size_t s=MIN(max-ws,bs-bp);
	memcpy(buf+ws,img,s);
	bp+=s,ws+=s,state.bytes+=ws;
	return (ssize_t)ws;
}

enum MHD_Result gui_http_hand_stream_jpeg(struct http_hand_info*i){
	struct MHD_Response*r;
	if(!state.buffer)return http_ret_code(i,MHD_HTTP_SERVICE_UNAVAILABLE);
	r=MHD_create_response_from_callback(MHD_SIZE_UNKNOWN,1024,&stream_jpeg,NULL,NULL);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"multipart/x-mixed-replace; boundary="_BOUNDARY);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
}

enum MHD_Result gui_http_hand_static_jpeg(struct http_hand_info*i){
	struct MHD_Response*r;
	if(!state.buffer)return http_ret_code(i,MHD_HTTP_SERVICE_UNAVAILABLE);
	if(!gui_http_init_img_ctx())return -1;
	gui_http_ctx.last_pos=0;
	MUTEX_LOCK(gui_http_ctx.lock);
	int x=stbi_write_jpg_to_func(
		gui_http_img_write,
		&gui_http_ctx,
		state.ww,state.hh,
		3,state.buffer,90
	);
	MUTEX_UNLOCK(gui_http_ctx.lock);
	if(x==0){
		tlog_warn("write jpeg failed: %d",x);
		return http_ret_code(i,MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
	state.bytes+=gui_http_ctx.last_pos;
	r=MHD_create_response_from_buffer(gui_http_ctx.last_pos,gui_http_ctx.buf,MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"image/jpeg");
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
}

enum MHD_Result gui_http_hand_static_bmp(struct http_hand_info*i){
	struct MHD_Response*r;
	if(!state.buffer)return http_ret_code(i,MHD_HTTP_SERVICE_UNAVAILABLE);
	if(!gui_http_init_img_ctx())return -1;
	gui_http_ctx.last_pos=0;
	MUTEX_LOCK(gui_http_ctx.lock);
	int x=stbi_write_bmp_to_func(
		gui_http_img_write,
		&gui_http_ctx,
		state.ww,state.hh,
		3,state.buffer
	);
	MUTEX_UNLOCK(gui_http_ctx.lock);
	if(x==0){
		tlog_warn("write bmp failed: %d",x);
		return http_ret_code(i,MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
	r=MHD_create_response_from_buffer(gui_http_ctx.last_pos,gui_http_ctx.buf,MHD_RESPMEM_PERSISTENT);
	state.bytes+=gui_http_ctx.last_pos;
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"image/bmp");
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
}

enum MHD_Result gui_http_hand_static_png(struct http_hand_info*i){
	struct MHD_Response*r;
	if(!state.buffer)return http_ret_code(i,MHD_HTTP_SERVICE_UNAVAILABLE);
	if(!gui_http_init_img_ctx())return -1;
	gui_http_ctx.last_pos=0;
	MUTEX_LOCK(gui_http_ctx.lock);
	int x=stbi_write_png_to_func(
		gui_http_img_write,
		&gui_http_ctx,
		state.ww,state.hh,
		3,state.buffer,
		state.ww*3
	);
	MUTEX_UNLOCK(gui_http_ctx.lock);
	if(x==0){
		tlog_warn("write png failed: %d",x);
		return http_ret_code(i,MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
	r=MHD_create_response_from_buffer(gui_http_ctx.last_pos,gui_http_ctx.buf,MHD_RESPMEM_PERSISTENT);
	state.bytes+=gui_http_ctx.last_pos;
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"image/png");
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
}
#endif

enum MHD_Result gui_http_hand_static_raw(struct http_hand_info*i){
	char buf[16];
	size_t size=state.ww*state.hh*3;
	struct MHD_Response*r;
	if(!state.buffer)return http_ret_code(i,MHD_HTTP_SERVICE_UNAVAILABLE);
	r=MHD_create_response_from_buffer(size,state.buffer,MHD_RESPMEM_PERSISTENT);
	state.bytes+=size;
	snprintf(buf,sizeof(buf),"%dx%d",state.ww,state.hh);
	MHD_add_response_header(r,"X-Screen-Size",buf);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"application/octet-stream");
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
}

#endif
#endif
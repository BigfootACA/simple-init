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
#include<zlib.h>
#include<json.h>
#include<microhttpd.h>
#include"frame_protocol.h"
#include"gui/guidrv.h"
#include"gui_http.h"
#include"confd.h"
#include"http.h"
#include"str.h"
#include"gui.h"
#define TAG "http"
#define WIDTH  540
#define HEIGHT 960

struct img_ctx gui_http_ctx;
struct gui_http_state state;

static void*bytes_mon(void*d __attribute__((unused))){
	char report[256],last_report[256];
	char buf1[64],buf2[64];
	size_t last=0,cur;
	memset(last_report,0,sizeof(last_report));
	for(;;){
		cur=state.bytes-last;
		if(cur>0){
			memset(report,0,sizeof(report));
			snprintf(
				report,sizeof(report)-1,
				"%s transferred, %s/s",
				make_readable_str_buf(buf1,sizeof(buf1),state.bytes,1,0),
				make_readable_str_buf(buf2,sizeof(buf2),cur,1,0)
			);
			if(strcmp(report,last_report)!=0)
				logger_print(LEVEL_DEBUG,TAG,report);
			strncpy(last_report,report,sizeof(report)-1);
		}
		last=state.bytes;
		sleep(1);
	}
	return NULL;
}

static void http_flush(
	lv_disp_drv_t*disp_drv,
	const lv_area_t*area,
	lv_color_t*color_p
){
	if(
		area->x2<0||area->y2<0||
		area->x1>disp_drv->hor_res-1||
		area->y1>disp_drv->ver_res-1
	){
		lv_disp_flush_ready(disp_drv);
		return;
	}
	#ifdef ENABLE_WEBSOCKET
	if(state.disp_ws)gui_http_send_frame_area(area,color_p);
	#endif
	int32_t y,x,p;
	uint8_t*fb=state.buffer;
	for(y=area->y1;y<=area->y2&&y<disp_drv->ver_res;y++){
		p=(y*disp_drv->hor_res+area->x1)*3;
		for(x=area->x1;x<=area->x2&&x<disp_drv->hor_res;x++){
			pixel_copy(PIXEL_BGRA32,(char*)fb+p,(char*)color_p);
			p+=3,color_p++;
		}
	}
	lv_disp_flush_ready(disp_drv);
}

static void http_apply_mode(){
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
	state.ww=mode->width;
	state.hh=mode->height;
	done:
	if(name)free(name);
}

json_object*gui_http_get_size_json(){
	json_object*jo=json_object_new_object();
	json_object_object_add(jo,"type",json_object_new_string("size"));
	json_object_object_add(jo,"width",json_object_new_int(state.ww));
	json_object_object_add(jo,"height",json_object_new_int(state.hh));
	return jo;
}

static enum MHD_Result hand_query_size(struct http_hand_info*i){
	struct MHD_Response*r;
	json_object*jo=gui_http_get_size_json();
	const char*str=json_object_to_json_string(jo);
	r=MHD_create_response_from_buffer(strlen(str),(char*)str,MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,"application/json");
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	json_object_put(jo);
	return MHD_YES;
}

#ifdef ENABLE_FFMPEG
#define CODECS(_codecs...) .codecs=(struct video_codec*[]){_codecs,NULL},
#define CODEC(_name,_fmt) &(struct video_codec){.name=_name,.fmt=_fmt}
#define VIDEO(_name,_type,_codecs...){\
	.enabled=true,.url="/stream/"_name,.handler=gui_http_hand_stream_video,\
	.spec.data=&(struct video_data){.name=_name,.type=_type,CODECS(_codecs)}\
},
#endif

static struct http_hand handlers[]={
	{.enabled=true, .url="/query/size",   .handler=hand_query_size},
	{.enabled=true, .url="/static/raw",   .handler=gui_http_hand_static_raw},
	#ifdef ENABLE_STB
	{.enabled=true, .url="/static/bmp",   .handler=gui_http_hand_static_bmp},
	{.enabled=true, .url="/static/png",   .handler=gui_http_hand_static_png},
	{.enabled=true, .url="/static/jpg",   .handler=gui_http_hand_static_jpeg},
	{.enabled=true, .url="/static/jpeg",  .handler=gui_http_hand_static_jpeg},
	{.enabled=true, .url="/stream/jpeg",  .handler=gui_http_hand_stream_jpeg},
	#endif
	#ifdef ENABLE_FFMPEG
	VIDEO("mjpeg", "video/mjpeg",    CODEC("mjpeg_vaapi",AV_PIX_FMT_VAAPI), CODEC("mjpeg",AV_PIX_FMT_YUVJ420P))
	VIDEO("amv",   "video/mjpeg",    CODEC("amv",AV_PIX_FMT_YUVJ420P))
	VIDEO("mpeg1", "video/mpeg",     CODEC("mpeg1video",AV_PIX_FMT_YUV420P))
	VIDEO("mpeg2", "video/mpeg",     CODEC("mpeg2_vaapi",AV_PIX_FMT_VAAPI), CODEC("mpeg2video",AV_PIX_FMT_YUV420P))
	VIDEO("mpeg4", "video/mpeg",     CODEC("mpeg4",AV_PIX_FMT_YUV420P))
	VIDEO("h261",  "video/h261",     CODEC("h261",AV_PIX_FMT_YUV420P))
	VIDEO("h262",  "video/h262",     CODEC("libx262",AV_PIX_FMT_YUV420P))
	VIDEO("h263",  "video/h263",     CODEC("h263",AV_PIX_FMT_YUV420P), CODEC("h263p",AV_PIX_FMT_YUV420P))
	VIDEO("h264",  "video/h264",     CODEC("h264_vaapi",AV_PIX_FMT_VAAPI), CODEC("libx264rgb",AV_PIX_FMT_RGB24), CODEC("libx264",AV_PIX_FMT_YUV420P))
	VIDEO("h265",  "video/h265",     CODEC("hevc_vaapi",AV_PIX_FMT_VAAPI), CODEC("libx265",AV_PIX_FMT_YUV420P))
	VIDEO("vp8",   "video/vp8",      CODEC("vp8_vaapi",AV_PIX_FMT_VAAPI),  CODEC("libvpx",AV_PIX_FMT_YUV420P))
	VIDEO("vp9",   "video/vp9",      CODEC("vp9_vaapi",AV_PIX_FMT_VAAPI),  CODEC("libvpx-vp9",AV_PIX_FMT_YUV420P))
	VIDEO("av1",   "video/av1",      CODEC("libaom-av1",AV_PIX_FMT_YUV420P))
	VIDEO("flv",   "video/x-flv",    CODEC("flv",AV_PIX_FMT_YUV420P))
	VIDEO("wmv",   "video/x-ms-wmv", CODEC("wmv2",AV_PIX_FMT_YUV420P))
	#endif
	#ifdef ENABLE_WEBSOCKET
	{
		.enabled=true,
		.url="/websocket",
		.handler=http_hand_websocket,
		.spec.websocket={
			.establish=gui_http_disp_ws_establish,
			.disconnect=gui_http_disp_ws_disconnect,
			.cmds=WS_CMDS(
				WS_CMD_PROC("FLUSH",gui_http_disp_ws_cmd_flush)
				WS_CMD_PROC("TYPE:JPG",gui_http_disp_ws_cmd_set_type)
				WS_CMD_PROC("TYPE:PNG",gui_http_disp_ws_cmd_set_type)
				WS_CMD_PROC("TYPE:BMP",gui_http_disp_ws_cmd_set_type)
				WS_CMD_PROC("TYPE:RAW",gui_http_disp_ws_cmd_set_type)
				WS_CMD_PROC("COMP:TRUE",gui_http_disp_ws_cmd_set_compress)
				WS_CMD_PROC("COMP:FALSE",gui_http_disp_ws_cmd_set_compress)
				WS_CMD_PROC("SIZE",gui_http_disp_ws_cmd_size)
				WS_CMD_PROC("READ",gui_http_disp_ws_cmd_full_screen)
				WS_CMD_PROC("GOD",gui_http_disp_ws_cmd_dragon_egg)
				WS_CMD_PROC("INV",gui_http_disp_ws_cmd_inv)
			),
			.hands=WS_HANDS(
				WS_HAND_PROC("input",gui_http_recv_input_json,NULL)
			)
		}
	},
	#endif
	{
		.enabled=true,
		.url="/input/trigger",
		.method=MHD_HTTP_METHOD_POST,
		.handler=gui_http_hand_input_trigger
	},{
		.enabled=true,
		.spec.assets={
			.dir=&assets_rootfs,
			.path="/usr/share/simple-init/gui-http",
			.index=(const char*[]){"index.html","index.htm",NULL},
		},
		.handler=http_hand_assets,
	},
	{.enabled=false}
};

static int http_register(){
	#ifdef ENABLE_WEBSOCKET
	state.disp_type=TYPE_RAW;
	#endif
	state.ww=WIDTH,state.hh=HEIGHT;
	http_apply_mode();
	size_t s=state.ww*state.hh;
	static lv_color_t*buf=NULL;
	static lv_disp_draw_buf_t disp_buf;
	uint16_t port=(uint16_t)confd_get_integer("gui.http_port",8080);
	errno=0;
	if(
		!(buf=malloc(s*sizeof(lv_color_t)))||
		!(state.buffer=malloc(s*3))
	){
		telog_error("malloc display buffer failed");
		if(state.buffer)free(state.buffer);
		if(buf)free(buf);
		state.buffer=buf=NULL;
		return -1;
	}
	if(!(state.hs=MHD_start_daemon(
		MHD_USE_POLL_INTERNAL_THREAD|
		MHD_USE_THREAD_PER_CONNECTION|
		MHD_ALLOW_UPGRADE|
		MHD_USE_ERROR_LOG,
		port,NULL,NULL,
		http_conn_handler,handlers,
		MHD_OPTION_EXTERNAL_LOGGER,&http_logger,NULL,
		MHD_OPTION_END
	))){
		telog_error("failed to initialize microhttpd on port %d",port);
		free(state.buffer);
		free(buf);
		return -1;
	}
	pthread_t p;
	pthread_create(&p,NULL,bytes_mon,NULL);
	sem_init(&state.input_wait,0,0);
	memset(buf,0,s*sizeof(lv_color_t));
	memset(state.buffer,0,s*3);
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,s);
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=http_flush;
	disp_drv.hor_res=state.ww;
	disp_drv.ver_res=state.hh;
	disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}
	tlog_debug("screen resolution: %dx%d",state.ww,state.hh);
	lv_disp_drv_register(&disp_drv);
	return 0;
}

static void http_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=state.ww,h=state.hh;break;
		case 90:case 270:w=state.hh,h=state.ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}

static int http_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	for(*cnt=0;builtin_modes[*cnt].name[0];(*cnt)++);
	if(*cnt<=0)return 0;
	size_t size=((*cnt)+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memcpy(*modes,builtin_modes,size);
	return 0;
}

static void http_exit(){
	if(state.hs)MHD_stop_daemon(state.hs);
	state.hs=NULL;
}

static bool http_cansleep(){return false;}

struct gui_driver guidrv_http={
	.name="http",
	.drv_register=http_register,
	.drv_getsize=http_get_sizes,
	.drv_get_modes=http_get_modes,
	.drv_exit=http_exit,
	.drv_cansleep=http_cansleep,
};
#endif
#endif

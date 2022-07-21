/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _GUI_HTTP_H
#define _GUI_HTTP_H
#ifdef ENABLE_MICROHTTPD
#include<json.h>
#include<semaphore.h>
#ifdef ENABLE_FFMPEG
#include<libavutil/opt.h>
#include<libavutil/imgutils.h>
#include<libavcodec/avcodec.h>
#include<libswscale/swscale.h>
#endif
#include"frame_protocol.h"
#include"logger.h"
#include"http.h"
#include"gui.h"
#define TAG "http"
#define _BOUNDARY "BoundaryString"

#ifdef ENABLE_FFMPEG
struct video_data{
	const char*name;
	const char*type;
	struct video_codec{
		const char*name;
		enum AVPixelFormat fmt;
	}**codecs;
};
struct video_ctx{
	const AVCodec*codec;
	struct SwsContext*sws;
	AVCodecContext*ctx;
	AVFrame*pic;
	AVPacket*pkt;
	int64_t pts;
};
#endif
struct post_data{
	char*data;
	size_t size;
};
struct img_ctx{
	char*buf;
	size_t mem_size;
	size_t last_pos;
	mutex_t lock;
};
struct gui_http_state{
	void*buffer;
	struct MHD_Daemon*hs;
	int ww,hh;
	size_t bytes;
	lv_indev_data_t kbd_data;
	lv_indev_data_t ptr_data;
	lv_indev_data_t enc_data;
	sem_t input_wait;
	#ifdef ENABLE_WEBSOCKET
	sem_t disp_wait;
	bool frame_compress;
	frame_type disp_type;
	struct http_hand_websocket_data*disp_ws;
	#endif
};
extern struct gui_http_state state;
extern struct img_ctx gui_http_ctx;
#ifdef ENABLE_FFMPEG
extern enum MHD_Result gui_http_hand_stream_video(struct http_hand_info*i);
#endif
#ifdef ENABLE_STB
extern enum MHD_Result gui_http_hand_stream_jpeg(struct http_hand_info*i);
extern enum MHD_Result gui_http_hand_static_jpeg(struct http_hand_info*i);
extern enum MHD_Result gui_http_hand_static_bmp(struct http_hand_info*i);
extern enum MHD_Result gui_http_hand_static_png(struct http_hand_info*i);
#endif
extern enum MHD_Result gui_http_hand_static_raw(struct http_hand_info*i);
extern bool gui_http_init_img_ctx();
extern json_object*gui_http_get_size_json();
extern void gui_http_img_write(void*c,void*data,int size);
extern int gui_http_recv_input_json(struct http_hand_websocket_data*d,struct ws_data_hand*hand,const json_object*jo);
extern enum MHD_Result gui_http_hand_input_trigger(struct http_hand_info*i);
#ifdef ENABLE_WEBSOCKET
extern int gui_http_disp_ws_cmd_flush(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_size(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_inv(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_full_screen(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_set_type(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_set_compress(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_cmd_dragon_egg(struct ws_cmd_proc*cmd,struct http_hand_websocket_data*d,char**dd,size_t*dl);
extern int gui_http_disp_ws_establish(struct http_hand_websocket_data*d);
extern int gui_http_disp_ws_disconnect(struct http_hand_websocket_data*d);
extern void gui_http_send_frame(enum frame_pixel mode,uint64_t sx,uint64_t sy,uint64_t dx,uint64_t dy,const void*frame);
extern void gui_http_send_frame_area(const lv_area_t*area,const lv_color_t*frame);
#endif
#endif
#endif
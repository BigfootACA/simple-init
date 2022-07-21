/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_FFMPEG
#ifdef ENABLE_MICROHTTPD
#include"frame_protocol.h"
#include"gui_http.h"
#include"http.h"

static bool write_video_frame(struct video_ctx*video){
	int r=AVERROR(EAGAIN);
	static uint32_t last=0;
	if(!state.buffer)return false;
	if(video->pts!=0)r=avcodec_receive_packet(video->ctx,video->pkt);
	if(r==AVERROR(EAGAIN)||r==AVERROR_EOF){
		uint32_t cur=lv_tick_get();
		uint32_t e=cur-last;
		if(e<1000/30)usleep(e*1000);
		last=cur;
		errno=0,r=av_frame_make_writable(video->pic);
		if(r<0)return terlog_warn(false,"frame unwritable: %d",r);
		video->pts++;
		int st=gui_w*3;
		const uint8_t*const src[]={state.buffer,NULL};
		sws_scale(
			video->sws,src,
			&st,0,state.hh,
			video->pic->data,
			video->pic->linesize
		);
		video->pic->pts=video->pts;
		errno=0,r=avcodec_send_frame(video->ctx,video->pic);
		if(r<0)return terlog_warn(false,"send frame failed: %d",r);
		return write_video_frame(video);
	}
	return true;
}
static ssize_t stream_video(
	void*cls,
	uint64_t pos __attribute__((unused)),
	char*buf,
	size_t max
){
	struct video_ctx*video=cls;
	static size_t bs=0,bp=0;
	if(bp>=bs){
		bp=0;
		if(!write_video_frame(video))return -1;
		bs=video->pkt->size;
	}
	size_t s=MIN(max,bs-bp);
	memcpy(buf,video->pkt->data+bp,s);
	bp+=s,state.bytes+=s;
	return (ssize_t)s;
}
static void clean_video_ctx(struct video_ctx*video){
	if(!video)return;
	if(video->ctx)avcodec_free_context(&video->ctx);
	if(video->pkt)av_packet_free(&video->pkt);
	if(video->pic)av_frame_free(&video->pic);
	if(video->sws)sws_freeContext(video->sws);
	free(video);
}

static bool get_encoder(struct video_ctx*video,struct video_codec*c){
	int x=0;
	errno=0,video->codec=avcodec_find_encoder_by_name(c->name);
	if(!video->codec)return terlog_warn(false,"codec %s not found",c->name);
	errno=0,video->ctx=avcodec_alloc_context3(video->codec);
	if(!video->ctx)EDONE(telog_warn("alloc context failed"));
	video->ctx->width=gui_w;
	video->ctx->height=state.hh;
	video->ctx->bit_rate=400000;
	video->ctx->time_base.den=30;
	video->ctx->time_base.num=1;
	video->ctx->gop_size=60;
	video->ctx->pix_fmt=c->fmt;
	video->ctx->thread_count=1;
	switch(video->codec->id){
		case AV_CODEC_ID_HEVC:
		case AV_CODEC_ID_MPEG2VIDEO:
		case AV_CODEC_ID_H264:
			video->ctx->flags|=AV_CODEC_FLAG_LOW_DELAY;
			break;
		default:break;
	}
	av_opt_set(video->ctx->priv_data,"tune","zerolatency",0);
	av_opt_set(video->ctx->priv_data,"preset","ultrafast",0);
	errno=0,x=avcodec_open2(video->ctx,video->codec,NULL);
	if(x<0)EDONE(telog_warn("open codec %s failed: %d",c->name,x));
	video->pic->width=video->ctx->width;
	video->pic->height=video->ctx->height;
	video->pic->format=video->ctx->pix_fmt;
	errno=0,x=av_frame_get_buffer(video->pic,0);
	if(x<0)EDONE(telog_warn("alloc buffer failed: %d",x));
	errno=0,x=av_frame_make_writable(video->pic);
	if(x<0)EDONE(telog_warn("frame not writable: %d",x));
	errno=0,video->sws=sws_getContext(
		gui_w,state.hh,AV_PIX_FMT_RGB24,
		gui_w,state.hh,video->ctx->pix_fmt,
		SWS_FAST_BILINEAR,NULL,NULL,NULL
	);
	if(!video->sws)EDONE(telog_warn("alloc swscale failed"));
	return true;
	done:
	if(video->ctx)avcodec_free_context(&video->ctx);
	video->ctx=NULL;
	video->codec=NULL;
	return false;
}

enum MHD_Result gui_http_hand_stream_video(struct http_hand_info*i){
	bool found=false;
	int ret=MHD_HTTP_SERVICE_UNAVAILABLE;
	struct video_ctx*video=NULL;
	struct video_codec*codec;
	struct video_data*vd=i->hand->spec.data;
	struct MHD_Response*r;
	if(!state.buffer||!vd)goto done;
	ret=MHD_HTTP_INTERNAL_SERVER_ERROR;
	if(!(video=malloc(sizeof(struct video_ctx))))goto done;
	memset(video,0,sizeof(struct video_ctx));
	errno=0,video->pkt=av_packet_alloc();
	if(!video->pkt)EDONE(telog_warn("alloc packet failed"));
	errno=0,video->pic=av_frame_alloc();
	if(!video->pic)EDONE(telog_warn("alloc frame failed"));
	for(size_t c=0;(codec=vd->codecs[c]);c++){
		if(!get_encoder(video,codec))continue;
		found=true;
		break;
	}
	if(!found)EDONE(tlog_warn("no any available encoder for %s",vd->name));
	r=MHD_create_response_from_callback(
		MHD_SIZE_UNKNOWN,1024,stream_video,video,NULL
	);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CONTENT_TYPE,vd->type);
	MHD_add_response_header(r,MHD_HTTP_HEADER_CACHE_CONTROL,"no-cache");
	MHD_queue_response(i->conn,MHD_HTTP_OK,r);
	MHD_destroy_response(r);
	return MHD_YES;
	done:
	clean_video_ctx(video);
	return http_ret_code(i,ret);
}
#endif
#endif
#endif
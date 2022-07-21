/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<SDL.h>
#include<zlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<emscripten.h>
#include<emscripten/websocket.h>
#include"../../include/frame_protocol.h"
#include"web_gui_render.h"
#include"websocket.h"

static struct http_hand_websocket websocket;
static struct{
	SDL_Surface*screen;
	size_t bytes,frames;
	bool compressed:1;
	bool all_paused:1;
	bool video_paused:1;
	bool input_paused:1;
}state;
enum {
	LV_KEY_UP        = 17,  /*0x11*/
	LV_KEY_DOWN      = 18,  /*0x12*/
	LV_KEY_RIGHT     = 19,  /*0x13*/
	LV_KEY_LEFT      = 20,  /*0x14*/
	LV_KEY_ESC       = 27,  /*0x1B*/
	LV_KEY_DEL       = 127, /*0x7F*/
	LV_KEY_BACKSPACE = 8,   /*0x08*/
	LV_KEY_ENTER     = 10,  /*0x0A, '\n'*/
	LV_KEY_NEXT      = 9,   /*0x09, '\t'*/
	LV_KEY_PREV      = 11,  /*0x0B, '*/
	LV_KEY_HOME      = 2,   /*0x02, STX*/
	LV_KEY_END       = 3,   /*0x03, ETX*/
};

static int hand_frame(
	struct http_hand_websocket*h,
	struct ws_data_hand*hand __attribute__((unused)),
	const char*rd,size_t len
){
	uint8_t bsp;
	void*buf=NULL;
	static size_t ds=0;
	static void*zbuf=NULL;
	size_t ss=sizeof(struct frame_data);
	struct frame_data*d=(struct frame_data*)rd;
	if(!state.screen||h!=&websocket)return 0;
	if(state.all_paused||state.video_paused)return 0;
	if(!d||len<=ss)return -1;
	size_t rs=ss+d->size,xs;
	if(memcmp(d->magic,FRAME_MAGIC,sizeof(d->magic))!=0){
		fprintf(stderr,"invalid frame, magic mismatch\n");
		return 0;
	}
	if(d->version!=FRAME_VERSION){
		fprintf(
			stderr,"incompatible frame, version mismatch %d != %d\n",
			d->version,FRAME_VERSION
		);
		return 0;
	}
	if(rs!=len){
		fprintf(
			stderr,"invalid frame, "
			"need %zu, have %zu\n",
			rs,len
		);
		return 0;
	}
	if(
		d->dst_x<=d->src_x||d->dst_y<=d->src_y||
		d->dst_x>state.screen->w||d->dst_y>state.screen->h
	){
		fprintf(
			stderr,"invalid frame, area out of screen "
			"(%u,%u)-(%u,%u) (%dx%d)\n",
			d->src_x,d->src_y,d->dst_x,d->dst_y,
			state.screen->w,state.screen->h
		);
		return 0;
	}
	if(d->type!=TYPE_RAW){
		fprintf(stderr,"unsupported frame type\n");
		return 0;
	}
	bsp=pixel_size(d->pixel);
	if(d->compressed){
		if(d->src_size>ds){
			if(zbuf)free(zbuf);
			if(!(zbuf=malloc(d->src_size))){
				fprintf(stderr,"alloc memory for decompress\n");
				goto out;
			}
			ds=d->src_size;
		}
		uLongf zl=d->src_size;
		int r=uncompress(zbuf,&zl,(Bytef*)d->frame,d->size);
		if(r!=Z_OK){
			fprintf(stderr,"decompress failed: %d\n",r);
			goto out;
		}
		xs=zl,buf=zbuf;
	}else xs=d->size,buf=d->frame;
	size_t gs=(d->dst_x-d->src_x)*(d->dst_y-d->src_y)*bsp;
	if(gs!=xs)fprintf(stderr,"buffer size mismatch %zu != %zu\n",gs,xs);
	if(SDL_MUSTLOCK(screen))SDL_LockSurface(state.screen);
	uint32_t p,n=0;
	for(uint32_t y=d->src_y;y<=d->dst_y;y++){
		p=(y*state.screen->w+d->src_x)*4;
		for(uint32_t x=d->src_x;x<=d->dst_x;x++){
			pixel_copy(d->pixel,state.screen->pixels+p,buf+n);
			p+=4,n+=bsp;
		}
	}
	if(SDL_MUSTLOCK(screen))SDL_UnlockSurface(state.screen);
	SDL_Flip(state.screen);
	state.frames++;
	out:ws_send_cmd(h,"FLUSH");
	return 0;
}

static int hand_size(
	struct http_hand_websocket*h,
	struct ws_data_hand*hand __attribute__((unused)),
	const json_object*jo
){
	int ww=0,hh=0;
	json_object*val;
	if(h!=&websocket)return 0;
	if((val=json_object_object_get(jo,"width")))
		ww=json_object_get_int(val);
	if((val=json_object_object_get(jo,"height")))
		hh=json_object_get_int(val);
	if(ww<0||hh<0)return -1;
	if(!state.screen){
		printf("frame buffer size: %dx%d\n",ww,hh);
		if(!(state.screen=SDL_SetVideoMode(ww,hh,32,SDL_SWSURFACE))){
			fprintf(stderr,"create screen failed\n");
			return -1;
		}
	}else fprintf(stderr,"screen already initialized, skip\n");
	web_gui_refresh();
	ws_send_cmd(h,"COMP:TRUE");
	ws_send_cmd(h,"TYPE:RAW");
	return 0;
}

static struct http_hand_websocket websocket={
	.counter=&state.bytes,
	.hands=WS_HANDS(
		WS_HAND_PROC("FRAME",NULL,hand_frame)
		WS_HAND_PROC("size",hand_size,NULL)
	)
};

static EM_BOOL on_open(
	int type __attribute__((unused)),
	const EmscriptenWebSocketOpenEvent*event,
	void*dta __attribute__((unused))
){
	if(event->socket!=websocket.ws)return EM_FALSE;
	printf("web socket opened\n");
	ws_send_cmd(&websocket,"SIZE");
	return EM_TRUE;
}

static EM_BOOL on_close(
	int type __attribute__((unused)),
	const EmscriptenWebSocketCloseEvent*event,
	void*dta __attribute__((unused))
){
	if(event->socket!=websocket.ws)return EM_FALSE;
	printf("web socket closed\n");
	if(state.screen)SDL_FreeSurface(state.screen);
	if(websocket.ws>=0)websocket.ws=-1;
	state.screen=NULL;
	return EM_TRUE;
}

#define JSON_SET_DECL(_name,_type)\
	static void set_##_name(json_object*jo,const char*key,_type v){\
		json_object*val;\
		if((val=json_object_object_get(jo,key)))json_object_set_##_name(val,v);\
		else if((val=json_object_new_##_name(v)))json_object_object_add(jo,key,val);\
	}

#define JSON_SET(_name,_type,_key)\
	static inline void set_##_key(json_object*jo,_type v){set_##_name(jo,#_key,v);}
JSON_SET_DECL(boolean,bool)
JSON_SET_DECL(int64,int64_t)
JSON_SET_DECL(string,const char*)
JSON_SET(string,const char*,type)
JSON_SET(boolean,bool,pressed)
JSON_SET(int64,int64_t,key)
JSON_SET(int64,int64_t,x)
JSON_SET(int64,int64_t,y)

static uint32_t keycode_to_ascii(uint32_t sdl_key){
	switch(sdl_key){
		case SDLK_ESCAPE:return LV_KEY_ESC;
		case SDLK_BACKSPACE:return LV_KEY_BACKSPACE;
		case SDLK_DELETE:return LV_KEY_DEL;
		case SDLK_KP_ENTER:
		case '\r':return LV_KEY_ENTER;
		case SDLK_DOWN:
		case SDLK_RIGHT:return LV_KEY_RIGHT;
		case SDLK_UP:
		case SDLK_LEFT:return LV_KEY_LEFT;
		case SDLK_KP_MINUS:
		case SDLK_PAGEDOWN:return LV_KEY_DOWN;
		case SDLK_KP_PLUS:
		case SDLK_PAGEUP:return LV_KEY_UP;
		default:return sdl_key;
	}
}

static void parse_event(SDL_Event*e){
	bool changed=false;
	if(!state.screen)return;
	if(state.all_paused||state.input_paused)return;
	json_object*jo=json_object_new_object();
	switch(e->type){
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEMOTION:
			set_pressed(jo,e->button.button==SDL_BUTTON_LEFT);
			set_x(jo,e->motion.x);
			set_y(jo,e->motion.y);
			set_type(jo,"pointer");
			changed=true;
		break;
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:
		case SDL_FINGERMOTION:
			set_pressed(jo,e->type==SDL_FINGERDOWN);
			set_x(jo,(int64_t)((float)state.screen->w*e->tfinger.x));
			set_y(jo,(int64_t)((float)state.screen->h*e->tfinger.y));
			set_type(jo,"pointer");
			changed=true;
		break;
		case SDL_KEYDOWN:
			set_pressed(jo,true);
			set_key(jo,keycode_to_ascii(e->key.keysym.sym));
			set_type(jo,"keypad");
			changed=true;
		break;
		case SDL_KEYUP:
			set_pressed(jo,false);
			set_type(jo,"keypad");
			changed=true;
		break;
		default:break;
	}
	if(changed)ws_send_json_payload(&websocket,"input",jo);
}

static void sdl_event_handler(){
	SDL_Event e;
	while(SDL_PollEvent(&e))
		parse_event(&e);
}

EMSCRIPTEN_KEEPALIVE int web_gui_start(const char*url){
	if((websocket.ws=emscripten_websocket_new(&(EmscriptenWebSocketCreateAttributes){
		.url=url,.protocols=NULL,.createOnMainThread=EM_TRUE
	}))<0){
		fprintf(stderr,"failed to create web socket: %d\n",websocket.ws);
		return -1;
	}
	emscripten_websocket_set_onmessage_callback(websocket.ws,&websocket,ws_on_message);
	emscripten_websocket_set_onclose_callback(websocket.ws,NULL,on_close);
	emscripten_websocket_set_onopen_callback(websocket.ws,NULL,on_open);
	return 0;
}

EMSCRIPTEN_KEEPALIVE void web_gui_disconnect(){
	if(websocket.ws<0)return;
	emscripten_websocket_close(
		websocket.ws,1000,
		"user request close"
	);
}

EMSCRIPTEN_KEEPALIVE void web_gui_refresh(){
	if(websocket.ws<0)return;
	ws_send_cmd(&websocket,"READ");
}

EMSCRIPTEN_KEEPALIVE void web_gui_initialize(){
	static bool initialized=false;
	if(initialized)return;
	SDL_Init(SDL_INIT_VIDEO);
	memset(&state,0,sizeof(state));
	websocket.ws=-1;
	emscripten_set_main_loop(sdl_event_handler,0,0);
	printf("web gui render initialized\n");
	initialized=true;
}

EMSCRIPTEN_KEEPALIVE void web_gui_set_compressed(bool compressed){
	if(compressed!=state.compressed&&websocket.ws>0)
		ws_send_cmd(&websocket,compressed?"COMP:TRUE":"COMP:FALSE");
	state.compressed=compressed;
}

int main(){
	web_gui_initialize();
	return 0;
}

EMSCRIPTEN_KEEPALIVE size_t web_gui_get_bytes(){return state.bytes;}
EMSCRIPTEN_KEEPALIVE size_t web_gui_get_frames(){return state.frames;}
EMSCRIPTEN_KEEPALIVE void web_gui_set_paused(bool paused){state.all_paused=paused;}
EMSCRIPTEN_KEEPALIVE void web_gui_set_video_paused(bool paused){state.video_paused=paused;}
EMSCRIPTEN_KEEPALIVE void web_gui_set_input_paused(bool paused){state.input_paused=paused;}

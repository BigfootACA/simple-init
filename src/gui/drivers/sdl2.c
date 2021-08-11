#ifdef ENABLE_GUI
#ifdef ENABLE_SDL2
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<SDL.h>
#include"version.h"
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
#define TAG "sdl2"
#define SDL2_Z 1
#define SDL2_W 540
#define SDL2_H 960
typedef struct{
	SDL_Window*window;
	SDL_Renderer*renderer;
	SDL_Texture*texture;
	volatile bool sdl_refr_qry;
	uint32_t tft_fb[SDL2_W*SDL2_H];
}monitor_t;
static monitor_t monitor;
static volatile bool sdl_inited=false,sdl_quit_qry=false;
static bool left_button_down=false;
static int16_t last_x=0,last_y=0,enc_diff=0;
static uint32_t last_key;
static lv_indev_state_t mw_state=LV_INDEV_STATE_REL,kbd_state;
static bool mouse_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->point.x=last_x,data->point.y=last_y;
	data->state=left_button_down?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
	return false;
}
static bool mousewheel_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->state=mw_state,data->enc_diff=enc_diff,enc_diff=0;
	return false;
}
static uint32_t keycode_to_ascii(uint32_t sdl_key){
	switch(sdl_key) {
		case SDLK_RIGHT:
		case SDLK_KP_PLUS:return LV_KEY_RIGHT;
		case SDLK_LEFT:
		case SDLK_KP_MINUS:return LV_KEY_LEFT;
		case SDLK_UP:return LV_KEY_UP;
		case SDLK_DOWN:return LV_KEY_DOWN;
		case SDLK_ESCAPE:return LV_KEY_ESC;
		case SDLK_BACKSPACE:return LV_KEY_BACKSPACE;
		case SDLK_DELETE:return LV_KEY_DEL;
		case SDLK_KP_ENTER:
		case '\r':return LV_KEY_ENTER;
		case SDLK_PAGEDOWN:return LV_KEY_NEXT;
		case SDLK_PAGEUP:return LV_KEY_PREV;
		default:return sdl_key;
	}
}
static bool keyboard_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->state=kbd_state,data->key=keycode_to_ascii(last_key);
	return false;
}
static void window_update(){
	SDL_UpdateTexture(monitor.texture,NULL,monitor.tft_fb,SDL2_W*sizeof(uint32_t));
	SDL_RenderClear(monitor.renderer);
	SDL_RenderCopy(monitor.renderer,monitor.texture,NULL,NULL);
	SDL_RenderPresent(monitor.renderer);
}
static void sdl2_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	lv_coord_t hres=disp_drv->hor_res,vres=disp_drv->ver_res;
	if(area->x2<0||area->y2<0||area->x1>hres-1||area->y1>vres-1){
		lv_disp_flush_ready(disp_drv);
		return;
	}
	uint32_t w=lv_area_get_width(area);
	for(int32_t y=area->y1;y<=area->y2&&y<disp_drv->ver_res;y++){
		memcpy(&monitor.tft_fb[y*SDL2_W+area->x1],color_p,w*sizeof(lv_color_t));
		color_p+=w;
	}
	monitor.sdl_refr_qry=true;
	if(lv_disp_flush_is_last(disp_drv)&&monitor.sdl_refr_qry){
		monitor.sdl_refr_qry=false;
		window_update();
	}
	lv_disp_flush_ready(disp_drv);
}
void sdl_event_handler(lv_task_t*t __attribute__((unused))){
	SDL_Event e;
	while(SDL_PollEvent(&e))switch(e.type){
		case SDL_MOUSEBUTTONUP:
			switch(e.button.button){
				case SDL_BUTTON_LEFT:left_button_down=false;break;
				case SDL_BUTTON_MIDDLE:mw_state=LV_INDEV_STATE_REL;break;
			}
		break;
		case SDL_MOUSEBUTTONDOWN:
			switch(e.button.button){
				case SDL_BUTTON_MIDDLE:
					mw_state=LV_INDEV_STATE_PR;
				break;
				case SDL_BUTTON_LEFT:
					left_button_down=true;
					last_x=e.motion.x/SDL2_Z;
					last_y=e.motion.y/SDL2_Z;
				break;
			}
		break;
		case SDL_MOUSEMOTION:
			last_x=e.motion.x/SDL2_Z;
			last_y=e.motion.y/SDL2_Z;
		break;
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:left_button_down=e.type==SDL_FINGERDOWN;//FALLTHROUGH
		case SDL_FINGERMOTION:
			last_x=SDL2_W*e.tfinger.x/SDL2_Z;
			last_y=SDL2_H*e.tfinger.y/SDL2_Z;
		break;
		case SDL_MOUSEWHEEL:enc_diff=-e.wheel.y;break;
		case SDL_KEYDOWN:kbd_state=LV_INDEV_STATE_PR,last_key=e.key.keysym.sym;break;
		case SDL_KEYUP:kbd_state=LV_INDEV_STATE_REL;break;
		case SDL_WINDOWEVENT:
			switch(e.window.event){
				#if SDL_VERSION_ATLEAST(2,0,5)
				case SDL_WINDOWEVENT_TAKE_FOCUS:
				#endif
				case SDL_WINDOWEVENT_EXPOSED:window_update();break;
				default:break;
			}
		break;
		default:break;
	}
	if(sdl_quit_qry){
		SDL_DestroyTexture(monitor.texture);
		SDL_DestroyRenderer(monitor.renderer);
		SDL_DestroyWindow(monitor.window);
		SDL_Quit();
		exit(0);
	}
}
static int quit_filter(void*userdata __attribute__((unused)),SDL_Event*e){
	if(
		e->type==SDL_QUIT||(
			e->type==SDL_WINDOWEVENT&&
			e->window.event==SDL_WINDOWEVENT_CLOSE
		)
	)sdl_quit_qry=true;
	return 1;
}
static int monitor_init(){
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetEventFilter(quit_filter,NULL);
	monitor.window=SDL_CreateWindow(
		PRODUCT,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SDL2_W*SDL2_Z,
		SDL2_H*SDL2_Z,0
	);
	monitor.renderer=SDL_CreateRenderer(
		monitor.window,-1,
		SDL_RENDERER_ACCELERATED
	);
	monitor.texture=SDL_CreateTexture(
		monitor.renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC,
		SDL2_W,SDL2_H
	);
	SDL_SetTextureBlendMode(
		monitor.texture,
		SDL_BLENDMODE_BLEND
	);
	memset(monitor.tft_fb,0x44,SDL2_W*SDL2_H*sizeof(uint32_t));
	monitor.sdl_refr_qry=true,sdl_inited=true;
	lv_task_create(sdl_event_handler,10,LV_TASK_PRIO_HIGH,NULL);

	// Display buffers
	static size_t s=SDL2_H*SDL2_W;
	static lv_color_t*buf=NULL;
	static lv_disp_buf_t disp_buf;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_buf_init(&disp_buf,buf,NULL,s);

	// Display device
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.buffer=&disp_buf;
	disp_drv.flush_cb=sdl2_flush;
	disp_drv.hor_res=SDL2_W;
	disp_drv.ver_res=SDL2_H;
	lv_disp_drv_register(&disp_drv);

	// Mouse input device
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type=LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb=mouse_read;
	lv_indev_drv_register(&indev_drv);

	// Keyboard input device
	lv_indev_drv_t kb_drv;
	lv_indev_drv_init(&kb_drv);
	kb_drv.type=LV_INDEV_TYPE_KEYPAD;
	kb_drv.read_cb=keyboard_read;
	lv_indev_set_group(lv_indev_drv_register(&kb_drv),gui_grp);

	// Mouse wheel input device
	lv_indev_drv_t enc_drv;
	lv_indev_drv_init(&enc_drv);
	enc_drv.type=LV_INDEV_TYPE_ENCODER;
	enc_drv.read_cb=mousewheel_read;
	lv_indev_drv_register(&enc_drv);

	return 0;
}
static void sdl2_get_sizes(uint32_t*width,uint32_t*height){
	if(width)*width=SDL2_W;
	if(height)*height=SDL2_H;
}
static void sdl2_get_dpi(int*dpi){
	if(dpi)*dpi=200/SDL2_Z;
}
static bool sdl2_can_sleep(){
	return false;
}
struct gui_driver guidrv_sdl2={
	.name="sdl2",
	.drv_register=monitor_init,
	.drv_getsize=sdl2_get_sizes,
	.drv_getdpi=sdl2_get_dpi,
	.drv_cansleep=sdl2_can_sleep
};
#endif
#endif

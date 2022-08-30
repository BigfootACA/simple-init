#ifdef ENABLE_GUI
#ifdef ENABLE_SDL2
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<SDL.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"version.h"
#include"gui/tools.h"
#include"gui/guidrv.h"
#include"src/draw/sdl/lv_draw_sdl.h"
#define TAG "sdl2"
typedef struct{
	lv_draw_sdl_drv_param_t param;
	SDL_Window*window;
	SDL_Renderer*renderer;
	SDL_Texture*texture;
	volatile bool sdl_refr_qry;
	uint32_t*tft_fb;
}monitor_t;
static uint32_t ww=540,hh=960;
static monitor_t monitor;
static volatile bool sdl_inited=false,sdl_quit_qry=false;
static bool accelerated;
static bool left_button_down=false;
static int16_t last_x=0,last_y=0,enc_diff=0;
static uint32_t last_key;
static lv_indev_state_t mw_state=LV_INDEV_STATE_REL,kbd_state;
static void mouse_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->point.x=lv_coord_border(last_x,gui_w-1,0);
	data->point.y=lv_coord_border(last_y,gui_h-1,0);
	if(last_x<0||last_y<0||last_x>=gui_w||last_y>=gui_h)left_button_down=false;
	data->state=left_button_down?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
}
static void mousewheel_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->state=mw_state,data->enc_diff=enc_diff,enc_diff=0;
}
static uint32_t keycode_to_ascii(uint32_t sdl_key){
	if(lv_group_get_editing(gui_grp))switch(sdl_key){
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
	}else switch(sdl_key){
		case SDLK_ESCAPE:return LV_KEY_ESC;
		case SDLK_BACKSPACE:return LV_KEY_BACKSPACE;
		case SDLK_DELETE:return LV_KEY_DEL;
		case SDLK_KP_ENTER:
		case '\r':return LV_KEY_ENTER;
		case SDLK_DOWN:
		case SDLK_RIGHT:
		case SDLK_KP_PLUS:
		case SDLK_PAGEDOWN:return LV_KEY_NEXT;
		case SDLK_UP:
		case SDLK_LEFT:
		case SDLK_KP_MINUS:
		case SDLK_PAGEUP:return LV_KEY_PREV;
		default:return sdl_key;
	}
}
static void keyboard_read(lv_indev_drv_t*indev_drv __attribute__((unused)),lv_indev_data_t*data){
	data->state=kbd_state,data->key=keycode_to_ascii(last_key);
}
static void window_update(){
	if(accelerated)SDL_SetRenderTarget(monitor.renderer,NULL);
	else SDL_UpdateTexture(monitor.texture,NULL,monitor.tft_fb,ww*sizeof(uint32_t));
	SDL_RenderClear(monitor.renderer);
	lv_disp_t*d=_lv_refr_get_disp_refreshing();
	if(d->driver->screen_transp){
		SDL_SetRenderDrawColor(monitor.renderer,0xFF,0,0,0xFF);
		SDL_RenderDrawRect(monitor.renderer,&(SDL_Rect){.x=0,.y=0,.w=ww,.h=hh});
	}
	if(accelerated){
		SDL_SetTextureBlendMode(monitor.texture,SDL_BLENDMODE_BLEND);
		SDL_RenderSetClipRect(monitor.renderer,NULL);
	}
	SDL_RenderCopy(monitor.renderer,monitor.texture,NULL,NULL);
	SDL_RenderPresent(monitor.renderer);
	if(accelerated)SDL_SetRenderTarget(monitor.renderer,monitor.texture);
}
static void sdl2_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	lv_coord_t hres=disp_drv->hor_res,vres=disp_drv->ver_res;
	if(area->x2<0||area->y2<0||area->x1>hres-1||area->y1>vres-1){
		lv_disp_flush_ready(disp_drv);
		return;
	}
	if(!accelerated){
		uint32_t w=lv_area_get_width(area);
		for(int32_t y=area->y1;y<=area->y2&&y<disp_drv->ver_res;y++){
			memcpy(&monitor.tft_fb[y*hres+area->x1],color_p,w*sizeof(lv_color_t));
			color_p+=w;
		}
	}
	if(lv_disp_flush_is_last(disp_drv))
		window_update();
	lv_disp_flush_ready(disp_drv);
}
static void sdl_event_handler(lv_timer_t*t __attribute__((unused))){
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
					last_x=e.motion.x,last_y=e.motion.y;
				break;
			}
		break;
		case SDL_MOUSEMOTION:last_x=e.motion.x,last_y=e.motion.y;break;
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:left_button_down=e.type==SDL_FINGERDOWN;//FALLTHROUGH
		case SDL_FINGERMOTION:last_x=ww*e.tfinger.x,last_y=hh*e.tfinger.y;break;
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
	if(sdl_quit_qry)gui_run=false;
}
static void sdl2_exit(){
	if(monitor.texture)SDL_DestroyTexture(monitor.texture);
	if(monitor.renderer)SDL_DestroyRenderer(monitor.renderer);
	if(monitor.window)SDL_DestroyWindow(monitor.window);
	SDL_Quit();
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
static void sdl_apply_mode(){
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
static int monitor_init(){
	if(!getenv("DISPLAY")&&!getenv("WAYLAND_DISPLAY"))return -1;
	memset(&monitor,0,sizeof(monitor));
	sdl_apply_mode();
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetEventFilter(quit_filter,NULL);
	accelerated=confd_get_boolean("gui.sdl2.accelerated",false);
	monitor.window=SDL_CreateWindow(
		PRODUCT,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		ww,hh,SDL_WINDOW_HIDDEN
	);
	monitor.renderer=SDL_CreateRenderer(
		monitor.window,-1,accelerated?
		SDL_RENDERER_ACCELERATED:
		SDL_RENDERER_SOFTWARE
	);
	if(accelerated){
		monitor.texture=lv_draw_sdl_create_screen_texture(
			monitor.renderer,ww,hh
		);
		SDL_SetRenderTarget(monitor.renderer,monitor.texture);
		monitor.param.renderer=monitor.renderer;
	}else{
		monitor.texture=SDL_CreateTexture(
			monitor.renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STATIC,
			ww,hh
		);
		SDL_SetTextureBlendMode(
			monitor.texture,
			SDL_BLENDMODE_BLEND
		);
	}
	if(!(monitor.tft_fb=malloc(ww*hh*sizeof(uint32_t))))
		return terlog_error(-1,"malloc framebuffer failed");
	memset(monitor.tft_fb,0x44,ww*hh*sizeof(uint32_t));
	monitor.sdl_refr_qry=true,sdl_inited=true;
	lv_timer_create(sdl_event_handler,10,NULL);

	// Display buffers
	size_t s=ww*hh;
	void*buf=NULL;
	static lv_disp_draw_buf_t disp_buf;
	if(accelerated)buf=monitor.texture;
	else{
		if(!(buf=malloc(s*sizeof(lv_color_t))))
			return terlog_error(-1,"malloc display buffer failed");
		memset(buf,0,s);
	}
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,s);

	// Display device
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=sdl2_flush;
	disp_drv.hor_res=ww;
	disp_drv.ver_res=hh;
	if(accelerated){
		disp_drv.direct_mode=1;
		disp_drv.antialiasing=1;
		disp_drv.user_data=&monitor.param;
		disp_drv.draw_ctx_init=lv_draw_sdl_init_ctx;
		disp_drv.draw_ctx_deinit=lv_draw_sdl_deinit_ctx;
		disp_drv.draw_ctx_size=sizeof(lv_draw_sdl_ctx_t);
	}else{
		disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
		disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
		disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	}
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}
	lv_disp_drv_register(&disp_drv);
	SDL_ShowWindow(monitor.window);
	SDL_RaiseWindow(monitor.window);
	SDL_StartTextInput();

	return 0;
}

static int kbd_init(){
	// Keyboard input device
	static lv_indev_drv_t kb_drv;
	lv_indev_drv_init(&kb_drv);
	kb_drv.type=LV_INDEV_TYPE_KEYPAD;
	kb_drv.read_cb=keyboard_read;
	lv_indev_set_group(lv_indev_drv_register(&kb_drv),gui_grp);
	return 0;
}

static int mse_init(){
	// Mouse input device
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type=LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb=mouse_read;
	lv_indev_drv_register(&indev_drv);
	return 0;
}

static int whl_init(){
	// Mouse wheel input device
	static lv_indev_drv_t enc_drv;
	lv_indev_drv_init(&enc_drv);
	enc_drv.type=LV_INDEV_TYPE_ENCODER;
	enc_drv.read_cb=mousewheel_read;
	lv_indev_drv_register(&enc_drv);
	return 0;
}

static void sdl2_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}
static void sdl2_get_dpi(int*dpi){
	if(dpi)*dpi=200;
}
static bool sdl2_can_sleep(){
	return false;
}
static int sdl2_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	for(*cnt=0;builtin_modes[*cnt].name[0];(*cnt)++);
	if(*cnt<=0)return 0;
	size_t size=((*cnt)+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memcpy(*modes,builtin_modes,size);
	return 0;
}
#ifdef SDL2_VIRTUAL_BACKLIGHT
static int sdl2_vbl=50;
static int sdl2_get_brightness(){
	return sdl2_vbl;
}
static void sdl2_set_brightness(int value){
	sdl2_vbl=value;
	telog_debug("set virtual backlight to %d%%",value);
}
#endif
struct input_driver indrv_sdl2_kbd={
	.name="sdl2-keyboard",
	.compatible={
		"sdl2",
		NULL
	},
	.drv_register=kbd_init,
};
struct input_driver indrv_sdl2_mse={
	.name="sdl2-mouse",
	.compatible={
		"sdl2",
		NULL
	},
	.drv_register=mse_init,
};
struct input_driver indrv_sdl2_whl={
	.name="sdl2-mouse-wheel",
	.compatible={
		"sdl2",
		NULL
	},
	.drv_register=whl_init,
};
struct gui_driver guidrv_sdl2={
	.name="sdl2",
	.drv_register=monitor_init,
	.drv_getsize=sdl2_get_sizes,
	.drv_getdpi=sdl2_get_dpi,
	.drv_cansleep=sdl2_can_sleep,
	.drv_get_modes=sdl2_get_modes,
	.drv_exit=sdl2_exit,
	#ifdef SDL2_VIRTUAL_BACKLIGHT
	.drv_getbrightness=sdl2_get_brightness,
	.drv_setbrightness=sdl2_set_brightness,
	#endif
};
#endif
#endif

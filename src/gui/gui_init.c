#ifdef ENABLE_GUI
#include<unistd.h>
#include<sys/time.h>
#include<drm_fourcc.h>
#include"lvgl.h"
#include"logger.h"
#include"hardware.h"
#include"gui.h"
#define TAG "gui"
uint32_t w=-1,h=-1;
bool run=true;
#ifdef ENABLE_GTK
static bool use_gtk=false;
#endif

void gui_do_quit(){
	fbdev_exit();
	#ifdef ENABLE_DRM
	drm_exit();
	#endif
}

int driver_init(){

	#ifdef ENABLE_GTK
	tlog_debug("try to scan gtk");
	if(gtkdrv_scan_init_register()>=0){
		w=GTK_W,h=GTK_H;
		use_gtk=true;
		return 0;
	}else tlog_error("failed to scan gtk");
	#endif

	#ifdef ENABLE_DRM
	tlog_debug("try to scan drm");
	if(drm_scan_init_register(DRM_FORMAT_ARGB8888)>=0){
		drm_get_sizes(&w,&h,NULL);
		ts_scan_register();
		return 0;
	}else tlog_error("failed to scan drm");
	#endif

	tlog_debug("try to scan fbdev");
	if(fbdev_scan_init_register()>=0){
		fbdev_get_sizes(&w,&h);
		ts_scan_register();
		return 0;
	}else tlog_error("failed to scan fbdev");

	return -1;
}

int gui_init(draw_func draw){
	lv_obj_t*screen;
	lv_init();
	if(driver_init()<0)return -1;
	tlog_debug("driver init done");
	if(!(screen=lv_scr_act()))return trlog_error(-1,"failed to get screen");
	draw(screen);
	while(run){
		lv_tick_inc(3);
		lv_task_handler();
		usleep(3000);
	}
	gui_do_quit();
	return 0;
}

uint32_t custom_tick_get(void){
	#ifdef ENABLE_GTK
	if(use_gtk)return gtkdrv_tick_get();
	#endif
	static uint64_t start_ms=0;
	if(start_ms==0){
		struct timeval tv_start;
		gettimeofday(&tv_start,NULL);
		start_ms=(tv_start.tv_sec*1000000+tv_start.tv_usec)/1000;
	}
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	return (uint64_t)((tv_now.tv_sec*1000000+tv_now.tv_usec)/1000)-start_ms;
}

#endif

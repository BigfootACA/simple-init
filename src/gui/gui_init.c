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
static struct gui_driver*drv=NULL;

void gui_do_quit(){
	if(!drv)return;
	drv->drv_exit();
}

int driver_init(){
	for(int i=0;gui_drvs[i];i++){
		tlog_debug("try to init gui driver %s",gui_drvs[i]->name);
		if(gui_drvs[i]->drv_register()<0){
			tlog_error("failed to start gui driver %s",gui_drvs[i]->name);
			continue;
		}
		gui_drvs[i]->drv_getsize(&w,&h);
		drv=gui_drvs[i];
		break;
	}
	return drv?0:-1;
}

int gui_init(draw_func draw){
	lv_obj_t*screen;
	lv_init();
	if(driver_init()<0)return -1;
	tlog_debug("driver init done");
	if(!(screen=lv_scr_act()))return trlog_error(-1,"failed to get screen");
	draw(screen);
	while(run){
		lv_tick_inc(15);
		lv_task_handler();
		usleep(15000);
	}
	gui_do_quit();
	return 0;
}

uint32_t custom_tick_get(void){
	if(!drv)return 0;
	if(drv->drv_tickget)return drv->drv_tickget();
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

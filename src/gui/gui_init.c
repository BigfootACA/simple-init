#ifdef ENABLE_GUI
#include<stdlib.h>
#include<unistd.h>
#include<sys/time.h>
#include<semaphore.h>
#include"lvgl.h"
#include"logger.h"
#include"hardware.h"
#include"gui.h"
#define TAG "gui"
int gui_dpi=400;
uint32_t w=-1,h=-1;
lv_font_t*gui_font=NULL;
bool run=true;
static struct gui_driver*drv=NULL;
static bool gui_sleep=false;
static sem_t gui_wait;

void gui_do_quit(){
	if(!drv)return;
	drv->drv_exit();
}

int driver_init(){
	for(int i=0;gui_drvs[i];i++){
		struct gui_driver*d=gui_drvs[i];
		if(!d->drv_getsize)continue;
		tlog_debug("try to init gui driver %s",d->name);
		if(d->drv_register()<0){
			tlog_error("failed to start gui driver %s",d->name);
			continue;
		}
		d->drv_getsize(&w,&h);
		if(d->drv_getdpi)d->drv_getdpi(&gui_dpi);
		drv=gui_drvs[i];
		break;
	}
	return drv?0:-1;
}

void gui_quit_sleep(){
	if(gui_sleep){
		gui_sleep=false;
		lv_tick_inc(LV_DISP_DEF_REFR_PERIOD);
		lv_task_handler();
		lv_disp_trig_activity(NULL);
		sem_post(&gui_wait);
	}
}

int gui_init(draw_func draw){
	lv_obj_t*screen;
	#ifdef ENABLE_FREETYPE2
	lv_freetype_init(64,1,0);
	char*x=getenv("FONT");
	gui_font=x?
		lv_ft_init(x,24,0):
		lv_ft_init_rootfs(_PATH_ETC"/default.ttf",24,0);
	#endif
	if(!gui_font)
		return telog_error("failed to load font");
	lv_init();
	if(driver_init()<0)return -1;
	tlog_debug("driver init done");
	if(!(screen=lv_scr_act()))return trlog_error(-1,"failed to get screen");
	draw(screen);
	sem_init(&gui_wait,0,0);
	while(run){
		if(lv_disp_get_inactive_time(NULL)<10000){
			lv_task_handler();
			if(drv->drv_taskhandler)drv->drv_taskhandler();
		}else{
			tlog_debug("enter sleep");
			gui_sleep=true;
			sem_wait(&gui_wait);
			gui_sleep=false;
			tlog_debug("quit sleep");
		}
		usleep(30000);
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

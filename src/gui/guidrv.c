#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
#define TAG "guidrv"

static struct gui_driver*drv;

int guidrv_getsize(uint32_t*w,uint32_t*h){
	errno=ENOENT;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_getsize)errno=ENOSYS;
	else drv->drv_getsize(w,h);
	return errno==0?0:-1;
}

int guidrv_getdpi(int*dpi){
	errno=ENOENT;
	if(gui_dpi_force>0){
		*dpi=gui_dpi_force;
		return 0;
	}
	*dpi=gui_dpi_def;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_getdpi)errno=ENOSYS;
	else drv->drv_getdpi(dpi);
	return errno==0?0:-1;
}

uint32_t guidrv_get_width(){
	uint32_t w=0;
	guidrv_getsize(&w,NULL);
	return w;
}

uint32_t guidrv_get_height(){
	uint32_t h=0;
	guidrv_getsize(NULL,&h);
	return h;
}

int guidrv_get_dpi(){
	int d=0;
	guidrv_getdpi(&d);
	return d;
}

const char*guidrv_getname(){
	errno=ENOENT;
	if(!drv)return "No Driver";
	errno=ENOSYS;
	if(!drv->name[0])return "Unknown";
	errno=0;
	return (char*)&drv->name;
}

int guidrv_taskhandler(){
	errno=0;
	if(!drv)ERET(ENOENT);
	if(!drv->drv_taskhandler)ERET(ENOSYS);
	drv->drv_taskhandler();
	return 0;
}

uint32_t guidrv_tickget(){
	errno=ENOENT;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_tickget)errno=ENOSYS;
	else return drv->drv_tickget();
	return -1;
}

int guidrv_set_brightness(int percent){
	errno=0;
	if(!drv)ERET(ENOENT);
	else if(!drv->drv_setbrightness)ERET(ENOSYS);
	else drv->drv_setbrightness(percent);
	return errno==0?0:-1;
}

int guidrv_get_brightness(){
	errno=0;
	if(!drv)ERET(ENOENT);
	else if(!drv->drv_getbrightness)ERET(ENOSYS);
	else return drv->drv_getbrightness();
}

bool guidrv_can_sleep(){
	if(!drv)return false;
	if(!drv->drv_cansleep)return true;
	return drv->drv_cansleep();
}

int guidrv_register(){
	errno=0;
	if(!drv)ERET(ENOENT);
	if(!drv->drv_register)ERET(ENOSYS);
	return drv->drv_register();
}

int guidrv_init(uint32_t*w,uint32_t*h,int*dpi){
	errno=0;
	if(drv){
		guidrv_getsize(w,h);
		guidrv_getdpi(dpi);
		return 0;
	}
	for(int i=0;(drv=gui_drvs[i]);i++){
		if(!drv->drv_getsize)continue;
		const char*name=guidrv_getname();
		tlog_debug("try to init gui driver %s",name);
		if(guidrv_register()<0){
			telog_error("failed to start gui driver %s",name);
			continue;
		}
		guidrv_getsize(w,h);
		guidrv_getdpi(dpi);
		if(*w<=0||*h<=0){
			tlog_error("failed to get screen size with gui driver %s",name);
			abort();
		}
		drv=gui_drvs[i];
		return 0;
	}
	tlog_warn("no available gui drivers found");
	drv=NULL;
	return -1;
}

void guidrv_exit(){
	if(drv&&drv->drv_exit)drv->drv_exit();
}

void guidrv_set_driver(struct gui_driver*driver){
	drv=driver;
}

struct gui_driver*guidrv_get_driver(){
	return drv;
}
#endif

#ifndef _GUIDRV_H
#define _GUIDRV_H
#include<stdint.h>
#include<stdbool.h>
struct gui_driver{
	char name[32];
	int(*drv_register)(void);
	void(*drv_getsize)(uint32_t*w,uint32_t*h);
	void(*drv_getdpi)(int*dpi);
	void(*drv_taskhandler)(void);
	void(*drv_exit)(void);
	uint32_t(*drv_tickget)(void);
	void(*drv_setbrightness)(int);
	int(*drv_getbrightness)(void);
	bool(*drv_cansleep)(void);
};
extern struct gui_driver*gui_drvs[];
extern int guidrv_getsize(uint32_t*w,uint32_t*h);
extern int guidrv_getdpi(int*dpi);
extern uint32_t guidrv_get_width();
extern uint32_t guidrv_get_height();
extern int guidrv_get_dpi();
extern const char*guidrv_getname();
extern int guidrv_taskhandler();
extern uint32_t guidrv_tickget();
extern int guidrv_register();
extern int guidrv_set_brightness(int percent);
extern int guidrv_get_brightness();
extern bool guidrv_can_sleep();
extern int guidrv_init(uint32_t*w,uint32_t*h,int*dpi);
extern struct gui_driver*guidrv_get_by_name(const char*name);
extern void guidrv_exit();
extern void guidrv_set_driver(struct gui_driver*driver);
extern struct gui_driver*guidrv_get_driver();
#endif

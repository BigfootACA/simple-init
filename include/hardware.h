#ifndef HARDWARE_H
#define HARDWARE_H
#include<lvgl.h>
#include<stdbool.h>
struct gui_driver{
	char name[32];
	int(*drv_register)(void);
	void(*drv_getsize)(uint32_t*w,uint32_t*h);
	void(*drv_taskhandler)(void);
	void(*drv_exit)(void);
	uint32_t(*drv_tickget)(void);
};
extern struct gui_driver*gui_drvs[];
extern void ts_scan_register(void);
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int set_brightness_percent(char*name,int percent);
extern void vibrate(char*dev,int time);
#endif

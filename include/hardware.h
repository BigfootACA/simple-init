#ifndef HARDWARE_H
#define HARDWARE_H
#include<lvgl.h>
#include<stdbool.h>
extern int set_brightness_percent(char*name,int percent);
extern void vibrate(char*dev,int time);
#endif

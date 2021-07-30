#ifndef HARDWARE_H
#define HARDWARE_H
#include<lvgl.h>
#include<stdbool.h>
extern int led_get_max_brightness(int fd);
extern int led_get_brightness(int fd);
extern int led_set_brightness(int fd,int value);
extern int led_get_brightness_percent(int fd);
extern int led_set_brightness_percent(int fd,int percent);
extern int led_set_brightness_percent_by_name(char*name,int percent);
extern void vibrate(char*dev,int time);
#endif

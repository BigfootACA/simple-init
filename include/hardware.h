#ifndef HARDWARE_H
#define HARDWARE_H
#include<lvgl.h>
#include<stdbool.h>
enum power_supply_status{
	STATUS_UNKNOWN=0,
	STATUS_CHARGING,
	STATUS_DISCHARGING,
	STATUS_NOT_CHARGING,
	STATUS_FULL,
};
enum power_supply_type{
	TYPE_UNKNOWN=0,
	TYPE_BATTERY,
	TYPE_UPS,
	TYPE_MAINS,
	TYPE_USB,
	TYPE_USB_DCP,
	TYPE_USB_CDP,
	TYPE_USB_ACA,
	TYPE_USB_HVDCP,
	TYPE_USB_HVDCP_3,
	TYPE_USB_TYPE_C,
	TYPE_USB_PD,
	TYPE_USB_PD_DRP,
	TYPE_APPLE_BRICK_ID,
	TYPE_WIRELESS,
	TYPE_USB_FLOAT,
	TYPE_BMS,
	TYPE_PARALLEL,
	TYPE_MAIN,
	TYPE_WIPOWER,
	TYPE_TYPEC,
	TYPE_UFP,
	TYPE_DFP,
};

extern int led_find(const char*name);
extern int led_find_class(int sysfs,const char*name);
extern int led_open_sysfs_class();
extern bool led_check_name(const char*name);
extern bool led_is_led(int fd);
extern int led_get_max_brightness(int fd);
extern int led_get_brightness(int fd);
extern int led_set_brightness(int fd,int value);
extern int led_get_brightness_percent(int fd);
extern int led_set_brightness_percent(int fd,int percent);
extern int led_set_brightness_percent_by_name(char*name,int percent);
extern int backlight_open_sysfs_class();
extern int backlight_find(const char*name);
extern int pwr_get_capacity(int fd);
extern bool pwr_is_battery(int fd);
extern int pwr_multi_get_capacity(int*fds);
extern int pwr_scan_device(int fds[],int max,bool battery);
extern int pwr_open_device(const char*name);
extern void vibrate(char*dev,int time);
#endif

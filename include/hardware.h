/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef HARDWARE_H
#define HARDWARE_H
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

// src/hardware/led.c: find LED by name and return fd
extern int led_find(const char*name);

// src/hardware/led.c: find LED by name and return fd (use custom /sys/class/leds fd)
extern int led_find_class(int sysfs,const char*name);

// src/hardware/led.c: open /sys/class/leds and return fd
extern int led_open_sysfs_class(void);

// src/hardware/led.c: check device name is valid
extern bool led_check_name(const char*name);

// src/hardware/led.c: check device is a LED
extern bool led_is_led(int fd);

// src/hardware/led.c: get LED max brightness
extern int led_get_max_brightness(int fd);

// src/hardware/led.c: get LED current brightness
extern int led_get_brightness(int fd);

// src/hardware/led.c: set LED current brightness
extern int led_set_brightness(int fd,int value);

// src/hardware/led.c: get LED current brightness by percent (0-100)
extern int led_get_brightness_percent(int fd);

// src/hardware/led.c: set LED current brightness by percent (0-100)
extern int led_set_brightness_percent(int fd,int percent);

// src/hardware/led.c: find LED and set current brightness by percent (0-100)
extern int led_set_brightness_percent_by_name(char*name,int percent);

// src/hardware/led.c: parse led argument [<CLASS>:]<NAME>
extern int led_parse_arg(const char*arg,char*def);

// src/hardware/led.c: open /sys/class/backlight and return fd
extern int backlight_open_sysfs_class(void);

// src/hardware/led.c: find backlight by name and return fd
extern int backlight_find(const char*name);

// src/hardware/battery.c: get battery capacity
extern int pwr_get_capacity(int fd);

// src/hardware/battery.c: is device a battery
extern bool pwr_is_battery(int fd);

// src/hardware/battery.c: is battery charging
extern bool pwr_is_charging(int fd);

// src/hardware/battery.c: is multi battery charging
extern bool pwr_multi_is_charging(int*fds);

// src/hardware/battery.c: get capacity from multi battery
extern int pwr_multi_get_capacity(int*fds);

// src/hardware/battery.c: scan power supply device
extern int pwr_scan_device(int fds[],int max,bool battery);

// src/hardware/battery.c: open power supply by name
extern int pwr_open_device(const char*name);

// src/hardware/battery.c: close all fd in array
extern void pwr_close_device(int*fds);

// src/hardware/battery.c: get power supply device type
extern enum power_supply_type pwr_get_type(int fd);

// src/hardware/battery.c: convert string to power supply type
extern enum power_supply_type pwr_chars2type(const char*type);

// src/hardware/battery.c: get power supply device status
extern enum power_supply_status pwr_get_status(int fd);

// src/hardware/battery.c: convert string to power supply status
extern enum power_supply_status pwr_chars2status(const char*status);

// src/hardware/battery.c: convert power supply status to string
extern const char*pwr_status2chars(enum power_supply_status status);

// src/hardware/battery.c: convert power supply type to string
extern const char*pwr_type2chars(enum power_supply_type type);

// src/hardware/vibrate.c: vibrate device
extern void vibrate(int time);
#endif

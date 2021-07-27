#ifndef _GUIPM_H
#define _GUIPM_H
#define _GNU_SOURCE
#include<dirent.h>
#include<stdlib.h>
#include<libintl.h>
#include<sys/stat.h>
#include<libfdisk/libfdisk.h>
#include"defines.h"
#include"system.h"
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include"str.h"
#define TAG "guipm"
extern void guipm_draw_title(lv_obj_t*screen);
extern void guipm_draw_disk_sel(lv_obj_t*screen);
#endif
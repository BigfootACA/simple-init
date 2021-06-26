#ifndef HARDWARE_H
#define HARDWARE_H
#include<lvgl.h>
#include<stdbool.h>
#define GTK_W 540
#define GTK_H 960
extern void fbdev_exit(void);
extern int fbdev_init(char*dev);
extern int fbdev_init_register(char*fbdev);
extern int fbdev_scan_init();
extern int fbdev_scan_init_register();
extern void fbdev_flush(lv_disp_drv_t*drv,const lv_area_t*area,lv_color_t*color_p);
extern void fbdev_get_sizes(uint32_t*width,uint32_t*height);
extern void drm_exit(void);
extern int drm_init(const char*card,unsigned int fourcc);
extern int drm_init_register(const char*card,unsigned int fourcc);
extern int drm_scan_init(unsigned int fourcc);
extern int drm_scan_init_register(unsigned int fourcc);
extern void drm_flush(lv_disp_drv_t*drv,const lv_area_t*area,lv_color_t*color_p);
extern void drm_get_sizes(uint32_t*width,uint32_t*height,uint32_t*dpi);
extern int gtkdrv_scan_init_register();
extern void gtkdrv_init(void);
extern uint32_t gtkdrv_tick_get(void);
extern void ts_init(char*dev);
extern int ts_scan_init(void);
extern bool ts_read(struct _lv_indev_drv_t*indev_drv,lv_indev_data_t*data);
extern void ts_register(char*dev);
extern void ts_scan_register(void);
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int set_brightness_percent(char*name,int percent);
extern void vibrate(char*dev,int time);
#endif

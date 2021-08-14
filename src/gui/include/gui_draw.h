#ifndef GUI_DRAW_H
#define GUI_DRAW_H
extern int sysbar_draw(lv_obj_t*scr);
extern void guipm_draw_title(lv_obj_t*screen);
extern void guipm_draw_disk_sel(lv_obj_t*screen);
extern void guipm_draw_partitions(lv_obj_t*screen);
extern void reboot_menu_draw(lv_obj_t*screen);
extern void backlight_menu_draw(lv_obj_t*screen);
extern void logviewer_draw(lv_obj_t*screen);
extern void language_menu_draw(lv_obj_t*screen);
#endif

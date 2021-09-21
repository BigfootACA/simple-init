#ifndef GUI_H
#define GUI_H
#include"lvgl.h"
#include"defines.h"
#define IMG_RES _PATH_USR"/share/pixmaps/simple-init"
typedef void (*draw_func)(lv_obj_t*);
extern int gui_dpi_def,gui_dpi_force;
extern int gui_font_size;
extern int default_backlight;
extern uint32_t gui_w,gui_h;
extern uint32_t gui_sw,gui_sh,gui_sx,gui_sy;
extern lv_font_t*gui_font;
extern lv_font_t*gui_font_small;
extern lv_group_t*gui_grp;
extern lv_obj_t*gui_cursor;
extern bool gui_sleep,gui_run,gui_dark;
extern void input_scan_register(void);
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int gui_pre_init();
extern int gui_init();
extern int gui_main();
extern int gui_draw();
extern void gui_quit_sleep();
extern void gui_do_quit();
extern void gui_set_run_exit(runnable_t*run);
extern void gui_run_and_exit(runnable_t*run);
extern uint32_t custom_tick_get(void);
extern void png_decoder_init();
extern int register_guiapp();
#endif

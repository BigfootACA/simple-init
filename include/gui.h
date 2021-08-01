#ifndef GUI_H
#define GUI_H
#define DIS_X(x) (int)((gui_w)/100*(x))
#define DIS_Y(y) (int)((gui_h)/200*(y))
typedef void (*draw_func)(lv_obj_t*);
extern uint32_t gui_w,gui_h;
extern uint32_t gui_sw,gui_sh,gui_sx,gui_sy;
extern lv_font_t*gui_font;
extern void ts_scan_register(void);
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int gui_init(draw_func draw);
extern void gui_quit_sleep();
extern void gui_do_quit();
extern uint32_t custom_tick_get(void);
#endif

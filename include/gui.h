#ifndef GUI_H
#define GUI_H
#define DIS_X(x) (int)((w)/100*(x))
#define DIS_Y(y) (int)((h)/200*(y))
typedef void (*draw_func)(lv_obj_t*);
extern uint32_t w,h;
extern int gui_init(draw_func draw);
extern void gui_do_quit();
extern uint32_t custom_tick_get(void);
#endif

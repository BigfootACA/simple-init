#ifndef GUI_H
#define GUI_H
#define DIS_X(x) (int)((gui_w)/100*(x))
#define DIS_Y(y) (int)((gui_h)/200*(y))
typedef enum{
	FT_FONT_STYLE_NORMAL = 0,
	FT_FONT_STYLE_ITALIC = 1<<0,
	FT_FONT_STYLE_BOLD   = 1<<1
}lv_ft_style;
struct gui_driver{
	char name[32];
	int(*drv_register)(void);
	void(*drv_getsize)(uint32_t*w,uint32_t*h);
	void(*drv_getdpi)(int*dpi);
	void(*drv_taskhandler)(void);
	void(*drv_exit)(void);
	uint32_t(*drv_tickget)(void);
	void(*drv_setbrightness)(int);
	int(*drv_getbrightness)(void);
};
typedef void (*draw_func)(lv_obj_t*);
extern uint32_t gui_w,gui_h;
extern lv_font_t*gui_font;
extern struct gui_driver*gui_drvs[];
extern void ts_scan_register(void);
extern int init_lvgl_fs(char letter,char*root,bool debug);
extern int gui_init(draw_func draw);
extern void gui_quit_sleep();
extern void gui_do_quit();
extern uint32_t custom_tick_get(void);
extern bool lv_freetype_init(uint16_t max_faces, uint16_t max_sizes, uint32_t max_bytes);
extern void lv_freetype_destroy(void);
extern lv_font_t*lv_ft_init(const char*name,int weight,lv_ft_style style);
extern lv_font_t*lv_ft_init_data(unsigned char*data,long size,int weight,lv_ft_style style);
#ifdef ASSETS_H
extern lv_font_t*lv_ft_init_assets(entry_dir*assets,char*path,int weight,lv_ft_style style);
#endif
extern lv_font_t*lv_ft_init_rootfs(char*path,int weight,lv_ft_style style);
extern void lv_ft_destroy(lv_font_t*font);
extern int guidrv_getsize(uint32_t*w,uint32_t*h);
extern int guidrv_getdpi(int*dpi);
extern uint32_t guidrv_get_width();
extern uint32_t guidrv_get_height();
extern int guidrv_get_dpi();
extern const char*guidrv_getname();
extern int guidrv_taskhandler();
extern uint32_t guidrv_tickget();
extern int guidrv_register();
extern int guidrv_set_brightness(int percent);
extern int guidrv_get_brightness();
extern int guidrv_init(uint32_t*w,uint32_t*h,int*dpi);
extern void guidrv_exit();
extern void guidrv_set_driver(struct gui_driver*driver);
extern struct gui_driver*guidrv_get_driver();
#endif

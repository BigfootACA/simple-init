#ifndef _SYSBAR_H
#define _SYSBAR_H
#include"gui.h"
#include"gui_draw.h"
struct sysbar{
	int size;
	lv_obj_t*screen;
	lv_obj_t*content;
	struct{
		lv_obj_t*bar;
		struct{
			lv_obj_t*time,*level,*battery;
		}content;
	}top;
	lv_obj_t*keyboard;
	struct{
		lv_obj_t*bar;
		lv_style_t btn_style;
		bool style_inited;
		struct{
			lv_obj_t*back,*home,*keyboard,*power;
		}content;
	}bottom;
};
extern struct sysbar sysbar;
extern void sysbar_keyboard_toggle();
extern void sysbar_keyboard_close();
extern void sysbar_keyboard_open();
#endif

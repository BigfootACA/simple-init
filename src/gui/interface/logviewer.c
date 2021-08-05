#include<libintl.h>
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#define TAG "logviewer"

static lv_obj_t*scr,*view;

void logviewer_draw(lv_obj_t*screen){

	scr=lv_obj_create(screen,NULL);
	lv_obj_set_size(scr,gui_sw,gui_sh);
	lv_obj_set_pos(scr,gui_sx,gui_sy);
	lv_theme_apply(scr,LV_THEME_SCR);

	lv_obj_t*txt=lv_label_create(scr,NULL);
	lv_label_set_text(txt,_("Logger Viewer"));
	lv_obj_set_width(txt,gui_sw);
	lv_obj_align(txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);

	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_text_font(&style,LV_STATE_DEFAULT,gui_font_small);

	view=lv_textarea_create(scr,NULL);
	lv_obj_set_size(view,gui_sw-(gui_font_size*2),gui_sh-lv_obj_get_height(txt)-gui_font_size*3);
	lv_obj_align(view,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_add_style(view,LV_TEXTAREA_PART_BG,&style);
	lv_textarea_set_cursor_hidden(view,true);
	lv_textarea_set_one_line(view,false);
	lv_obj_set_click(view,false);
	lv_textarea_set_text(view,"");
	lv_textarea_set_scrollbar_mode(view,LV_SCROLLBAR_MODE_DRAG);

	int fd=open(_PATH_DEV"/logger.log",O_RDONLY);
	if(fd>=0){
		char buff[BUFSIZ]={0};
		while(read(fd,buff,BUFSIZ-1)>0){
			lv_textarea_add_text(view,buff);
			memset(buff,0,BUFSIZ);
		}
		close(fd);
		lv_textarea_set_cursor_pos(view,0);
	}

	guiact_register_activity(&(struct gui_activity){
		.name="logger-viewer",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=true,
		.page=scr
	});
}

#include<libintl.h>
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#define TAG "logviewer"

static lv_obj_t*scr,*view;
static lv_obj_t*btn_top,*btn_bottom,*btn_reload;

static void load_log(){
	lv_textarea_set_text(view,"");
	int fd=open(_PATH_DEV"/logger.log",O_RDONLY);
	if(fd<0)telog_warn("open logger.log failed");
	else{
		char buff[BUFSIZ]={0};
		while(read(fd,buff,BUFSIZ-1)>0){
			lv_textarea_add_text(view,buff);
			memset(buff,0,BUFSIZ);
		}
		lv_textarea_add_text(view,"\n");
		close(fd);
		lv_textarea_set_cursor_pos(view,0);
	}
}

static void go_top_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_top||e!=LV_EVENT_CLICKED)return;
	lv_textarea_set_cursor_pos(view,0);
}

static void reload_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_reload||e!=LV_EVENT_CLICKED)return;
	load_log();
}

static void go_bottom_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_bottom||e!=LV_EVENT_CLICKED)return;
	lv_textarea_set_cursor_pos(view,LV_TEXTAREA_CURSOR_LAST);
}

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

	int bth=gui_font_size+(gui_dpi/8),btw=gui_sw/3-(gui_dpi/8);

	view=lv_textarea_create(scr,NULL);
	lv_obj_set_size(view,gui_sw-(gui_font_size*2),gui_sh-lv_obj_get_height(txt)-gui_font_size*4-bth);
	lv_obj_align(view,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_add_style(view,LV_TEXTAREA_PART_BG,&style);
	lv_textarea_set_cursor_hidden(view,true);
	lv_textarea_set_one_line(view,false);
	lv_obj_set_click(view,false);
	lv_page_set_scrollable_fit2(view,LV_FIT_MAX,LV_FIT_MAX);
	lv_textarea_set_scrollbar_mode(view,LV_SCROLLBAR_MODE_DRAG);
	lv_textarea_ext_t*e=lv_obj_get_ext_attr(view);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_EXPAND);

	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
	lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);

	btn_top=lv_btn_create(scr,NULL);
	lv_obj_set_size(btn_top,btw,bth);
	lv_obj_align(btn_top,view,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_obj_add_style(btn_top,LV_BTN_PART_MAIN,&btn_style);
	lv_obj_add_state(btn_top,LV_STATE_CHECKED);
	lv_obj_set_event_cb(btn_top,go_top_click);
	lv_label_set_text(lv_label_create(btn_top,NULL),_("Go top"));
	lv_group_add_obj(gui_grp,btn_top);

	btn_reload=lv_btn_create(scr,NULL);
	lv_obj_set_size(btn_reload,btw,bth);
	lv_obj_align(btn_reload,view,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_add_style(btn_reload,LV_BTN_PART_MAIN,&btn_style);
	lv_obj_add_state(btn_reload,LV_STATE_CHECKED);
	lv_label_set_text(lv_label_create(btn_reload,NULL),_("Reload"));
	lv_obj_set_event_cb(btn_reload,reload_click);
	lv_group_add_obj(gui_grp,btn_reload);

	btn_bottom=lv_btn_create(scr,NULL);
	lv_obj_set_size(btn_bottom,btw,bth);
	lv_obj_align(btn_bottom,view,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
	lv_obj_add_style(btn_bottom,LV_BTN_PART_MAIN,&btn_style);
	lv_obj_add_state(btn_bottom,LV_STATE_CHECKED);
	lv_label_set_text(lv_label_create(btn_bottom,NULL),_("Go bottom"));
	lv_obj_set_event_cb(btn_bottom,go_bottom_click);
	lv_group_add_obj(gui_grp,btn_bottom);

	load_log();

	guiact_register_activity(&(struct gui_activity){
		.name="logger-viewer",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=true,
		.page=scr
	});
}

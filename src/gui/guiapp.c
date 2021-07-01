#include<libintl.h>
#include"defines.h"
#include"lvgl.h"
#include"gui.h"

static lv_style_t style;
static lv_obj_t*screen;

static void add_button(int x,int y,char*name){
	lv_obj_t*file=lv_btn_create(screen,NULL);
	lv_obj_align(file,NULL,LV_ALIGN_IN_TOP_LEFT,w/2*x+DIS_X(5),h/5*y+DIS_X(5));
	lv_obj_set_size(file,w/2-DIS_X(10),h/5-DIS_X(10));
	lv_obj_add_style(file,LV_OBJ_PART_MAIN,&style);
	lv_obj_t*txt=lv_label_create(file,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_obj_set_width(txt,w/2-DIS_X(15));
	lv_label_set_text(txt,name);
}

static void _draw(lv_obj_t*scr){
	screen=scr;
	// button style
	lv_style_init(&style);
	lv_style_set_text_font(&style,0,&lv_font_cjk_24);
	lv_style_set_radius(&style,LV_STATE_DEFAULT,10);

	// buttons
	add_button(0,0,_("File Manager"));
	add_button(1,0,_("Partition Manager"));
	add_button(0,1,_("USB Control"));
	add_button(1,1,_("Registry Editor"));
	add_button(0,2,_("Image Backgup Recovery"));
	add_button(1,2,_("Enter TWRP"));
	add_button(0,3,_("System Info"));
	add_button(1,3,_("Multi-Boot Manage"));
	add_button(0,4,_("Reboot Menu"));
	add_button(1,4,_("Loggerd Viewer"));
}

int guiapp_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	return gui_init(_draw);
}
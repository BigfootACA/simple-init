#include<stdlib.h>
#include<libintl.h>
#include"logger.h"
#include"defines.h"
#include"lvgl.h"
#include"gui.h"
#include"gui_draw.h"
#include"activity.h"
#include"tools.h"
#define TAG "guiapp"
static lv_obj_t*screen,*realscr;
static int app_num=0;

void clean_buttons(){
	lv_obj_t*o=lv_obj_get_child(screen,NULL);
	if(o)do{lv_obj_del(o);}
	while((o=lv_obj_get_child(screen,o)));
}

static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		lv_obj_del_async(lv_obj_get_parent(obj));
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}

static void click_btn(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	draw_func f=(draw_func)lv_obj_get_user_data(obj);
	if(f)f(realscr);
	else lv_create_ok_msgbox_mask(
		screen,ok_msg_click,
		"This function does not implemented"
	);
}

static void add_button(char*name,char*img,draw_func draw){
	int a=app_num%4,b=app_num/4;
	int w=(gui_w-gui_font_size)/4,h=(gui_h-gui_font_size)/5;

	lv_style_t*style=malloc(sizeof(lv_style_t));
	lv_style_init(style);
	lv_style_set_radius(style,LV_STATE_DEFAULT,10);
	lv_style_set_outline_width(style,LV_STATE_PRESSED,1);
	lv_style_set_outline_width(style,LV_STATE_FOCUSED,1);
	lv_style_set_outline_width(style,LV_STATE_CHECKED,1);

	lv_obj_t*app=lv_objmask_create(screen,NULL);
	lv_obj_set_click(app,true);
	lv_obj_set_pos(app,(w*a)+(gui_font_size/2),(h*b)+(gui_font_size/2));
	lv_obj_set_size(app,w,h);
	lv_obj_add_style(app,LV_OBJ_PART_MAIN,style);
	lv_obj_set_user_data(app,draw);
	lv_obj_set_event_cb(app,click_btn);
	lv_group_add_obj(gui_grp,app);

	lv_style_t*app_style=malloc(sizeof(lv_style_t));
	lv_style_init(app_style);
	lv_style_set_bg_color(app_style,LV_STATE_DEFAULT,LV_COLOR_WHITE);
	lv_style_set_radius(app_style,LV_STATE_DEFAULT,w/10);

	int ix=w-gui_font_size,im=gui_font_size/2;
	lv_obj_t*icon_w=lv_objmask_create(app,NULL);
	lv_obj_add_style(icon_w,LV_OBJMASK_PART_MAIN,app_style);
	lv_obj_set_click(icon_w,false);
	lv_obj_set_size(icon_w,ix,ix);
	lv_obj_set_pos(icon_w,im,im);

	lv_obj_t*icon=lv_img_create(icon_w,NULL);
	char path[BUFSIZ]={0};
	if(!img)img="apps.png";
	snprintf(path,BUFSIZ-1,IMG_RES"/%s",img);
	lv_img_set_src(icon,path);
	lv_img_ext_t*x=lv_obj_get_ext_attr(icon);
	if(x->w>0&&x->h>0)lv_img_set_zoom(icon,(int)(((float)ix/MAX(x->w,x->h))*256));
	lv_img_set_pivot(icon,0,0);

	lv_style_t*txt_style=malloc(sizeof(lv_style_t));
	lv_style_init(txt_style);
	lv_style_set_text_font(txt_style,LV_STATE_DEFAULT,gui_font_small);
	lv_style_set_pad_top(txt_style,LV_STATE_DEFAULT,gui_dpi/100);

	lv_obj_t*txt=lv_label_create(app,NULL);
	lv_obj_add_style(txt,LV_LABEL_PART_MAIN,txt_style);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(txt,w);
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(txt,name);
	lv_obj_align(txt,icon_w,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	app_num++;
}

static void _draw(lv_obj_t*scr){
	realscr=scr;
	screen=lv_obj_create(scr,NULL);
	lv_obj_set_size(screen,gui_sw,gui_sh);
	lv_obj_set_pos(screen,gui_sx,gui_sy);
	lv_theme_apply(screen,LV_THEME_SCR);

	// buttons
	add_button(_("File Manager"),           "filemgr.png",   NULL);
	add_button(_("Partition Manager"),      "guipm.png",     guipm_draw_disk_sel);
	add_button(_("USB Control"),            "usb.png",       NULL);
	add_button(_("Registry Editor"),        "regedit.png",   NULL);
	add_button(_("Image Backgup Recovery"), "backup.png",    NULL);
	add_button(_("Enter TWRP"),             "twrp.png",      NULL);
	add_button(_("System Info"),            "sysinfo.png",   NULL);
	add_button(_("Multi-Boot Manage"),      "bootmgr.png",   NULL);
	add_button(_("Reboot Menu"),            "reboot.png",    reboot_menu_draw);
	add_button(_("Loggerd Viewer"),         "logviewer.png", NULL);

	guiact_register_activity(&(struct gui_activity){
		.name="guiapp",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=false,
		.page=screen
	});
}

int guiapp_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	return gui_init(_draw);
}
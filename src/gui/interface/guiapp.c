#ifdef ENABLE_GUI
#include<stdlib.h>
#ifndef ENABLE_UEFI
#include"service.h"
#endif
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"lvgl.h"
#include"init_internal.h"
#include"str.h"
#include"gui.h"
#include"activity.h"
#include"tools.h"
#define TAG "guiapp"
static lv_obj_t*screen;
static int app_num=0;

static void clean_buttons(){
	lv_obj_t*o=lv_obj_get_child(screen,NULL);
	if(o)do{
		if(!lv_debug_check_obj_type(o,"lv_objmask"))continue;
		lv_obj_del_async(o);
	}while((o=lv_obj_get_child(screen,o)));
	app_num=0;
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
	if(!guiact_is_active_page(screen))return;
	char*f=(char*)lv_obj_get_user_data(obj);
	if(f)guiact_start_activity_by_name(f,NULL);
	else lv_create_ok_msgbox_mask(
		screen,ok_msg_click,
		_("This function does not implemented")
	);
}

static void add_button(struct gui_register*p){
	if(!p->show_app)return;
	int xnum=gui_w/gui_dpi*2,ynum=gui_h/gui_dpi;
	int a=app_num%xnum,b=app_num/xnum;
	int w=(gui_w-gui_font_size)/xnum,h=(gui_h-gui_font_size)/(ynum+1);

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
	lv_obj_set_user_data(app,p);
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
	char path[BUFSIZ]={0},fail[BUFSIZ]={0};
	strcpy(fail,IMG_RES"/apps.png");
	if(p->icon[0]){
		if(contains_of("./",2,p->icon[0]))strncpy(path,p->icon,BUFSIZ-1);
		else snprintf(path,BUFSIZ-1,IMG_RES"/%s",p->icon);
	}
	lv_img_set_src(icon,p->icon[0]?path:fail);
	lv_img_ext_t*x=lv_obj_get_ext_attr(icon);
	if((x->w<=0||x->h<=0)&&p->icon[0]){
		lv_img_set_src(icon,fail);
		x=lv_obj_get_ext_attr(icon);
	}
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
	lv_label_set_text(txt,_(p->title));
	lv_obj_align(txt,icon_w,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_obj_set_height(app,lv_obj_get_y(txt)+lv_obj_get_height(txt)+gui_font_size);

	app_num++;
}

static void redraw_apps(){
	clean_buttons();
	for(int i=0;guiact_register[i];i++)
		add_button(guiact_register[i]);
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	redraw_apps();
}

static int guiapp_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int guiapp_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_obj_t*o=lv_obj_get_child(screen,NULL);
	if(o)do{lv_group_remove_obj(o);}
	while((o=lv_obj_get_child(screen,o)));
	return 0;
}

static int guiapp_draw(struct gui_activity*act){
	screen=act->page;
	static lv_style_t txt_style;
	lv_style_init(&txt_style);
	lv_style_set_text_font(&txt_style,LV_STATE_DEFAULT,gui_font_small);
	lv_style_set_text_color(&txt_style,LV_STATE_DEFAULT,lv_color_make(200,200,200));

	lv_obj_t*author=lv_label_create(act->page,NULL);
	lv_label_set_text(author,"Author: BigfootACA");
	lv_label_set_long_mode(author,LV_LABEL_LONG_BREAK);
	lv_label_set_align(author,LV_LABEL_ALIGN_CENTER);
	lv_obj_add_style(author,LV_LABEL_PART_MAIN,&txt_style);
	lv_obj_set_width(author,gui_sw);
	lv_obj_align(author,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-gui_font_size);
	return 0;
}

static void _draw(lv_obj_t*scr __attribute((unused))){
	guiact_start_activity_by_name("guiapp",NULL);
}

struct gui_register guireg_guiapp={
	.name="guiapp",
	.title="GUI Application",
	.icon="apps.png",
	.show_app=false,
	.get_focus=guiapp_get_focus,
	.lost_focus=guiapp_lost_focus,
	.draw=guiapp_draw,
	.back=false
};

int guiapp_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	#ifndef ENABLE_UEFI
	open_socket_logfd_default();
	open_default_confd_socket(TAG);
	open_socket_initfd(DEFAULT_INITD,false);
	#endif
	return gui_init(_draw);
}

#ifndef ENABLE_UEFI
static int guiap_startup(struct service*svc __attribute__((unused))){
	return guiapp_main(0,NULL);
}

int register_guiapp(){
	struct service*guiapp=svc_create_service("guiapp",WORK_FOREGROUND);
	if(guiapp){
		svc_set_desc(guiapp,"GUI Application Launcher");
		svc_set_start_function(guiapp,guiap_startup);
		guiapp->auto_restart=true;
		svc_add_depend(svc_system,guiapp);
	}
	return 0;
}
#endif
#endif

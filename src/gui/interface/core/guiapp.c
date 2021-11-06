/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#ifndef ENABLE_UEFI
#include"service.h"
#endif
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"language.h"
#include"init_internal.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "guiapp"
struct gui_app{
	lv_obj_t*screen,*tabview,*author;
	list*apps;
	int app_num,app_page,cur_page;
	lv_group_focus_cb_t old_cb;
};
struct app_info{
	int x,y,page,num;
	struct gui_register*reg;
	lv_obj_t*tab,*app;
	struct gui_app*data;
};

static int clean_button(void*b){
	struct app_info*ai=(struct app_info*)b;
	if(!ai)return 0;
	lv_obj_del(ai->app);
	free(ai);
	return 0;
}

static void clean_buttons(struct gui_app*ga){
	list_free_all(ga->apps,clean_button);
	if(ga->tabview)lv_obj_del(ga->tabview);
	ga->apps=NULL,ga->tabview=NULL;
	ga->app_num=0,ga->app_page=-1;
}

static void click_btn(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct app_info*ai=lv_obj_get_user_data(obj);
	struct gui_app*ga=ai->data;
	if(!guiact_is_active_page(ga->screen))return;
	if(guiact_start_activity(ai->reg,NULL)!=0)
		msgbox_alert("This function does not implemented");
}

static void tabview_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	struct gui_app*ga=lv_obj_get_user_data(obj);
	ga->cur_page=lv_tabview_get_tab_act(ga->tabview);
	confd_set_integer("gui.guiapp.page",ga->cur_page);
}

static void add_page(struct gui_app*ga){
	ga->app_page++,ga->app_num=0;
	char name[16]={0};
	snprintf(name,15,"%d",ga->app_page);
	if(!ga->tabview){
		ga->tabview=lv_tabview_create(ga->screen,NULL);
		lv_obj_set_pos(ga->tabview,0,0);
		lv_obj_set_size(ga->tabview,gui_sw,gui_sh-lv_obj_get_height(ga->author)-gui_font_size);
		lv_obj_set_user_data(ga->tabview,ga);
		lv_obj_set_event_cb(ga->tabview,tabview_cb);
		lv_tabview_set_btns_pos(ga->tabview,LV_TABVIEW_TAB_POS_NONE);
	}
	lv_obj_t*t=lv_tabview_add_tab(ga->tabview,name);
	lv_page_set_scrlbar_mode(t,LV_SCROLLBAR_MODE_HIDE);
}

static void add_button(struct gui_app*ga,struct gui_register*p){
	if(!p->show_app)return;
	lv_obj_t*scr=lv_tabview_get_tab(ga->tabview,ga->app_page);
	lv_coord_t pw=lv_page_get_scrl_width(scr);
	lv_coord_t ph=lv_page_get_scrl_height(scr);
	lv_coord_t xnum=pw/gui_dpi*2,ynum=ph/gui_dpi;
	lv_coord_t a=ga->app_num%xnum,b=ga->app_num/xnum;
	lv_coord_t w=(pw-gui_font_size-8)/xnum,h=(ph-gui_font_size-8)/(ynum+1);
	lv_coord_t xx=(w*a)+(gui_font_size/2),yy=(h*b)+(gui_font_size/2);
	if(yy+h>ph){
		add_page(ga);
		add_button(ga,p);
		return;
	}
	struct app_info*ai=malloc(sizeof(struct app_info));
	if(!ai)return;
	memset(ai,0,sizeof(struct app_info));
	ai->data=ga,ai->page=ga->app_page,ai->tab=scr;
	ai->x=a,ai->y=b,ai->num=ga->app_num,ai->reg=p;
	ai->app=lv_objmask_create(scr,NULL);
	lv_obj_set_click(ai->app,true);
	lv_obj_set_drag_parent(ai->app,true);
	lv_obj_set_pos(ai->app,xx,yy);
	lv_obj_set_size(ai->app,w,h);
	lv_color_t c=lv_obj_get_style_text_color(scr,LV_OBJ_PART_MAIN);
	lv_obj_set_style_local_outline_color(ai->app,LV_OBJMASK_PART_MAIN,LV_STATE_PRESSED,c);
	lv_obj_set_style_local_outline_color(ai->app,LV_OBJMASK_PART_MAIN,LV_STATE_FOCUSED,c);
	lv_obj_set_style_local_outline_width(ai->app,LV_OBJMASK_PART_MAIN,LV_STATE_PRESSED,1);
	lv_obj_set_style_local_outline_width(ai->app,LV_OBJMASK_PART_MAIN,LV_STATE_FOCUSED,1);
	lv_obj_set_style_local_radius(ai->app,LV_OBJMASK_PART_MAIN,LV_STATE_DEFAULT,10);
	lv_obj_set_user_data(ai->app,ai);
	lv_obj_set_event_cb(ai->app,click_btn);
	if(guiact_is_active_page(ga->screen))lv_group_add_obj(gui_grp,ai->app);

	int ix=w-gui_font_size,im=gui_font_size/2;
	lv_obj_t*icon_w=lv_objmask_create(ai->app,NULL);
	lv_obj_set_style_local_radius(icon_w,LV_OBJMASK_PART_MAIN,LV_STATE_DEFAULT,w/10);
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

	lv_obj_t*txt=lv_label_create(ai->app,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(txt,w);
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(txt,_(p->title));
	lv_obj_align(txt,icon_w,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_obj_set_style_local_text_font(txt,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,gui_font_small);
	lv_obj_set_style_local_pad_top(txt,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/100);
	lv_obj_set_height(ai->app,lv_obj_get_y(txt)+lv_obj_get_height(txt)+gui_font_size);

	ga->cur_page=confd_get_integer("gui.guiapp.page",0);
	list_obj_add_new(&ga->apps,ai);
	ga->app_num++;
}

static void redraw_apps(struct gui_app*ga){
	clean_buttons(ga);
	add_page(ga);
	for(int i=0;guiact_register[i];i++)
		add_button(ga,guiact_register[i]);
	if(ga->cur_page>=lv_tabview_get_tab_count(ga->tabview))ga->cur_page=0;
	lv_tabview_set_tab_act(ga->tabview,ga->cur_page,LV_ANIM_OFF);
}

static void do_reload(lv_task_t*t){
	redraw_apps(t->user_data);
}

static int guiapp_get_focus(struct gui_activity*d){
	struct gui_app*ga=d->data;
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,ga));
	return 0;
}

static int guiapp_lost_focus(struct gui_activity*d){
	struct gui_app*ga=d->data;
	list*app=list_first(ga->apps);
	if(app)do{
		LIST_DATA_DECLARE(ai,app,struct app_info*);
		lv_group_remove_obj(ai->app);
	}while((app=app->next));
	return 0;
}

static int guiapp_init(struct gui_activity*act){
	struct gui_app*ga=malloc(sizeof(struct gui_app));
	if(!ga)return -ENOMEM;
	memset(ga,0,sizeof(struct gui_app));
	act->data=ga;
	return 0;
}

static int guiapp_exit(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int guiapp_draw(struct gui_activity*act){
	struct gui_app*ga=act->data;
	ga->screen=act->page;
	static lv_style_t txt_style;
	lv_style_init(&txt_style);
	lv_style_set_text_font(&txt_style,LV_STATE_DEFAULT,gui_font_small);
	lv_style_set_text_color(&txt_style,LV_STATE_DEFAULT,lv_color_make(200,200,200));

	ga->author=lv_label_create(act->page,NULL);
	lv_label_set_text(ga->author,"Author: BigfootACA");
	lv_label_set_long_mode(ga->author,LV_LABEL_LONG_BREAK);
	lv_label_set_align(ga->author,LV_LABEL_ALIGN_CENTER);
	lv_obj_add_style(ga->author,LV_LABEL_PART_MAIN,&txt_style);
	lv_obj_set_width(ga->author,gui_sw);
	lv_obj_align(ga->author,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-gui_font_size);

	list_free_all(ga->apps,NULL);
	ga->apps=NULL,ga->tabview=NULL,ga->app_num=0,ga->app_page=-1;
	return 0;
}

struct gui_register guireg_guiapp={
	.name="guiapp",
	.title="GUI Application",
	.icon="apps.png",
	.show_app=false,
	.init=guiapp_init,
	.quiet_exit=guiapp_exit,
	.get_focus=guiapp_get_focus,
	.lost_focus=guiapp_lost_focus,
	.draw=guiapp_draw,
	.back=false
};

int guiapp_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	#ifndef ENABLE_UEFI
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	open_socket_initfd(DEFAULT_INITD,false);
	lang_init_locale();
	#endif
	return gui_init();
}

#ifndef ENABLE_UEFI
static int guiapp_startup(struct service*svc __attribute__((unused))){
	return guiapp_main(0,NULL);
}

int register_guiapp(){
	struct service*guiapp=svc_create_service("guiapp",WORK_FOREGROUND);
	if(guiapp){
		svc_set_desc(guiapp,"GUI Application Launcher");
		svc_set_start_function(guiapp,guiapp_startup);
		guiapp->auto_restart=true;
		if(!confd_get_boolean("runtime.cmdline.gui_disable",false))
			svc_add_depend(svc_system,guiapp);
	}
	return 0;
}
#endif
#endif

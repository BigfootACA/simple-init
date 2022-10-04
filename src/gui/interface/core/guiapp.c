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
#include<sys/prctl.h>
#include"proctitle.h"
#include"service.h"
#endif
#include"str.h"
#include"gui.h"
#include"array.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"language.h"
#include"init_internal.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "guiapp"

struct gui_register guireg_guiapp;
struct gui_app{
	lv_obj_t*screen,*bg,*tabview,*author;
	list*apps;
	int app_num,app_page,cur_page;
	lv_coord_t*grid_row_template;
	lv_coord_t*grid_col_template;
	int row_cnt,col_cnt;
	lv_coord_t app_w,app_h;
	char bg_path[PATH_MAX];
	lv_obj_t*pages[128];
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
	if(ga->grid_col_template)free(ga->grid_col_template);
	if(ga->grid_row_template)free(ga->grid_row_template);
	memset(ga->pages,0,sizeof(ga->pages));
	ga->grid_col_template=NULL;
	ga->grid_row_template=NULL;
	ga->apps=NULL,ga->tabview=NULL;
	ga->app_num=0,ga->app_page=-1;
}

static void tabview_cb(lv_event_t*e){
	struct gui_app*ga=e->user_data;
	lv_obj_t*page=lv_tileview_get_tile_act(ga->tabview);
	ga->cur_page=(intptr_t)lv_obj_get_user_data(page);
	confd_set_integer("gui.guiapp.page",ga->cur_page);
	lv_obj_t*o=lv_group_get_focused(gui_grp);
	if(o){
		struct app_info*cai=lv_obj_get_user_data(o);
		if(cai&&cai->page==ga->cur_page)return;
	}
	list*l=list_first(ga->apps);
	if(l)do{
		LIST_DATA_DECLARE(ai,l,struct app_info*);
		if(ai->page==ga->cur_page){
			lv_group_focus_obj(ai->app);
			break;
		}
	}while((l=l->next));
}

static void click_btn(lv_event_t*e){
	struct app_info*ai=e->user_data;
	struct gui_app*ga=ai->data;
	if(!guiact_is_active_page(ga->screen))return;
	if(guiact_start_activity(ai->reg,NULL)!=0)
		msgbox_alert("This function does not implemented");
}

static void focused_btn(lv_event_t*e){
	struct app_info*ai=e->user_data;
	struct gui_app*ga=ai->data;
	if(!guiact_is_active_page(ga->screen))return;
	if(ai->page!=ga->cur_page){
		lv_obj_set_tile_id(ai->data->tabview,ai->page,0,LV_ANIM_ON);
		tabview_cb(&(lv_event_t){.user_data=ai->data});
	}
}

static void add_page(struct gui_app*ga){
	ga->app_page++,ga->app_num=0;
	if(!ga->tabview){
		ga->tabview=lv_tileview_create(ga->screen);
		lv_obj_set_scrollbar_mode(ga->tabview,LV_SCROLLBAR_MODE_ACTIVE);
		lv_obj_set_style_bg_opa(ga->tabview,LV_OPA_0,0);
		lv_obj_set_style_radius(ga->tabview,0,0);
		lv_obj_set_style_pad_all(ga->tabview,0,0);
		lv_obj_set_style_border_width(ga->tabview,0,0);
		lv_obj_set_pos(ga->tabview,0,0);
		lv_obj_set_size(ga->tabview,lv_pct(100),lv_obj_get_y(ga->author)-gui_font_size);
		lv_obj_add_event_cb(ga->tabview,tabview_cb,LV_EVENT_VALUE_CHANGED,ga);
		lv_obj_update_layout(ga->tabview);

	}
	if((size_t)ga->app_page>=ARRLEN(ga->pages)){
		tlog_error("too many pages");
		abort();
	}
	lv_obj_t*page=lv_tileview_add_tile(
		ga->tabview,ga->app_page,0,
		LV_DIR_LEFT|LV_DIR_RIGHT
	);
	lv_obj_set_user_data(page,(void*)(intptr_t)ga->app_page);
	lv_obj_set_size(
		page,
		lv_obj_get_width(ga->tabview),
		lv_obj_get_height(ga->tabview)
	);
	lv_obj_set_scroll_dir(page,LV_DIR_NONE);
	lv_obj_set_style_bg_opa(page,LV_OPA_0,0);
	lv_obj_set_style_radius(page,0,0);
	lv_obj_set_style_pad_all(page,gui_dpi/50,0);
	lv_obj_set_style_border_width(page,0,0);
	lv_obj_set_grid_dsc_array(page,
		ga->grid_col_template,
		ga->grid_row_template
	);
	lv_obj_update_layout(page);
	ga->pages[ga->app_page]=page;

}

static void add_button(struct gui_app*ga,struct gui_register*p){
	struct app_info*ai;
	if(!p->show_app)return;
	lv_coord_t bx=ga->app_num%ga->col_cnt;
	lv_coord_t by=ga->app_num/ga->col_cnt;
	if(by>=ga->row_cnt){
		add_page(ga);
		add_button(ga,p);
		return;
	}
	if(!(ai=malloc(sizeof(struct app_info))))return;
	memset(ai,0,sizeof(struct app_info));
	lv_coord_t ls=gui_dpi/100;
	lv_obj_t*scr=ga->pages[ga->app_page];
	ai->data=ga,ai->page=ga->app_page,ai->tab=scr;
	ai->x=bx,ai->y=by,ai->num=ga->app_num,ai->reg=p;
	lv_obj_t*warp=lv_obj_create(scr);
	lv_obj_clear_flag(warp,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(warp,0,0);
	lv_obj_set_style_pad_all(warp,ls,0);
	lv_obj_set_style_border_width(warp,0,0);
	lv_obj_set_scroll_dir(warp,LV_DIR_NONE);
	lv_obj_set_style_bg_opa(warp,LV_OPA_0,0);
	lv_obj_set_grid_cell(warp,
		LV_GRID_ALIGN_STRETCH,bx,1,
		LV_GRID_ALIGN_STRETCH,by,1
	);
	ai->app=lv_draw_line_wrapper(warp,NULL,NULL);
	lv_color_t c=lv_obj_get_style_text_color(scr,LV_PART_MAIN);
	lv_obj_set_style_outline_color(ai->app,c,LV_STATE_PRESSED);
	lv_obj_set_style_outline_color(ai->app,c,LV_STATE_FOCUSED);
	lv_obj_set_style_outline_width(ai->app,ls,LV_STATE_PRESSED);
	lv_obj_set_style_outline_width(ai->app,ls,LV_STATE_FOCUSED);
	lv_obj_add_flag(ai->app,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_radius(ai->app,10,0);
	lv_obj_set_style_pad_all(ai->app,0,0);
	lv_obj_add_event_cb(ai->app,click_btn,LV_EVENT_CLICKED,ai);
	lv_obj_add_event_cb(ai->app,focused_btn,LV_EVENT_FOCUSED,ai);
	lv_obj_update_layout(ai->app);
	if(guiact_is_active_page(ga->screen))lv_group_add_obj(gui_grp,ai->app);

	lv_coord_t aw=lv_obj_get_width(ai->app);
	lv_obj_t*icon_w=lv_draw_wrapper(ai->app,NULL,NULL,aw,aw);
	lv_obj_set_style_pad_all(icon_w,ls,0);
	lv_obj_set_size(icon_w,aw,aw);
	lv_obj_set_style_radius(icon_w,gui_dpi/10,0);
	lv_obj_t*icon=lv_img_create(icon_w);
	lv_obj_clear_flag(icon,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_img_src_try(icon,"app",p->name,p->icon);
	lv_obj_center(icon);
	lv_img_fill_image(icon,aw,aw);
	aw-=ls*2;

	lv_obj_t*txt=lv_label_create(ai->app);
	lv_obj_set_width(txt,aw);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(txt,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(txt,_(p->title));
	lv_obj_align_to(txt,icon_w,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_obj_set_style_text_font(txt,gui_font_small,0);
	lv_obj_set_style_pad_top(txt,gui_dpi/100,0);
	lv_obj_set_height(ai->app,LV_SIZE_CONTENT);

	list_obj_add_new(&ga->apps,ai);
	ga->app_num++;
}

static void redraw_apps(struct gui_app*ga){
	list*l;
	clean_buttons(ga);
	if(ga->col_cnt<=0||ga->row_cnt<=0){
		tlog_warn("screen too small, cannot draw apps");
		return;
	}
	size_t cs=sizeof(lv_coord_t)*(ga->col_cnt+1);
	size_t rs=sizeof(lv_coord_t)*(ga->row_cnt+1);
	if(
		!(ga->grid_col_template=malloc(cs))||
		!(ga->grid_row_template=malloc(rs))
	){
		tlog_error("alloc for template failed");
		return;
	}
	memset(ga->grid_col_template,0,cs);
	memset(ga->grid_row_template,0,rs);
	for(int i=0;i<ga->col_cnt;i++)ga->grid_col_template[i]=LV_GRID_FR(1);
	for(int i=0;i<ga->row_cnt;i++)ga->grid_row_template[i]=LV_GRID_FR(1);
	ga->grid_col_template[ga->col_cnt]=LV_GRID_TEMPLATE_LAST;
	ga->grid_row_template[ga->row_cnt]=LV_GRID_TEMPLATE_LAST;
	add_page(ga);
	if((l=guiact_get_registers()))do{
		add_button(ga,LIST_DATA(l,struct gui_register*));
	}while((l=l->next));
	int page=confd_get_integer("gui.guiapp.page",0);
	if(page!=ga->cur_page)lv_obj_set_tile_id(ga->tabview,page,0,LV_ANIM_OFF);
	if(ga->cur_page>=ga->app_page)ga->cur_page=0;
}

static void load_background(struct gui_app*ga,bool changed){
	lv_obj_set_hidden(ga->bg,true);
	if(!confd_get_boolean("gui.show_background",true))return;
	if(changed)lv_img_set_src(ga->bg,ga->bg_path);
	lv_img_t*x=(lv_img_t*)ga->bg;
	if(x->w>0&&x->h>0)lv_obj_set_hidden(ga->bg,false);
}

static void reload_background(struct gui_app*ga){
	bool changed=false;
	char*bg=confd_get_string("gui.background",NULL);
	if(bg&&strcmp(bg,ga->bg_path)!=0){
		memset(ga->bg_path,0,sizeof(ga->bg_path));
		strncpy(ga->bg_path,bg,sizeof(ga->bg_path)-1);
		changed=true;
	}
	load_background(ga,changed);
	if(bg)free(bg);
}

static int do_load(struct gui_activity*d){
	struct gui_app*ga=d->data;
	redraw_apps(ga);
	reload_background(ga);
	return 0;
}

static int guiapp_get_focus(struct gui_activity*d){
	struct gui_app*ga=d->data;
	list*app=list_first(ga->apps);
	if(app)do{
		LIST_DATA_DECLARE(ai,app,struct app_info*);
		lv_group_add_obj(gui_grp,ai->app);
	}while((app=app->next));
	int page=confd_get_integer("gui.guiapp.page",0);
	if(page!=ga->cur_page)lv_obj_set_tile_id(ga->tabview,page,0,LV_ANIM_OFF);
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
	char*path;
	if(
		!ga->bg_path[0]&&
		(path=confd_get_string(
			"gui.background",
			NULL
		))
	){
		strncpy(
			ga->bg_path,
			path,
			sizeof(ga->bg_path)-1
		);
		free(path);
	}
	return 0;
}

static int guiapp_exit(struct gui_activity*act){
	struct gui_app*ga=act->data;
	if(!ga)return 0;
	if(ga->grid_col_template)free(ga->grid_col_template);
	if(ga->grid_row_template)free(ga->grid_row_template);
	ga->grid_col_template=NULL;
	ga->grid_row_template=NULL;
	act->data=NULL;
	free(ga);
	return 0;
}

static int guiapp_resize(struct gui_activity*act){
	struct gui_app*ga=act->data;
	lv_coord_t x_cnt=act->w/gui_dpi*2,y_cnt=act->h/gui_dpi;
	if(x_cnt==ga->col_cnt&&y_cnt==ga->row_cnt)return 0;
	ga->col_cnt=x_cnt,ga->row_cnt=y_cnt;
	lv_obj_align_to(ga->author,NULL,LV_ALIGN_BOTTOM_MID,0,-gui_font_size);
	lv_obj_update_layout(ga->author);
	redraw_apps(ga);
	return 0;
}

static int guiapp_draw(struct gui_activity*act){
	struct gui_app*ga=act->data;
	ga->screen=act->page;

	ga->bg=lv_img_create(act->page);
	lv_obj_set_pos(ga->bg,0,0);
	lv_obj_set_size(ga->bg,lv_pct(100),lv_pct(100));
	lv_img_set_size_mode(ga->bg,LV_IMG_SIZE_MODE_VIRTUAL);
	lv_obj_set_hidden(ga->bg,true);
	load_background(ga,true);

	ga->author=lv_label_create(act->page);
	lv_obj_set_width(ga->author,lv_pct(100));
	lv_label_set_text(ga->author,"Author: BigfootACA");
	lv_label_set_long_mode(ga->author,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(ga->author,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_style_text_font(ga->author,gui_font_small,0);
	lv_obj_set_style_text_color(ga->author,lv_color_make(200,200,200),0);

	list_free_all(ga->apps,NULL);
	ga->apps=NULL,ga->tabview=NULL,ga->app_num=0,ga->app_page=-1;
	return 0;
}

struct gui_register guireg_guiapp={
	.name="guiapp",
	.title="GUI Application",
	.show_app=false,
	.init=guiapp_init,
	.quiet_exit=guiapp_exit,
	.get_focus=guiapp_get_focus,
	.lost_focus=guiapp_lost_focus,
	.resize=guiapp_resize,
	.draw=guiapp_draw,
	.data_load=do_load,
	.back=false
};

int guiapp_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	#ifndef ENABLE_UEFI
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	open_socket_initfd(DEFAULT_INITD,false);
	lang_init_locale();
	prctl(PR_SET_NAME,"GUI Launcher");
	setproctitle("guiapp");
	pid_t p=confd_get_integer("runtime.pid.bootmenu",0);
	if(p>0)while(is_link(_PATH_PROC"/%d/exe",p))sleep(1);
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

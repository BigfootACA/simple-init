/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifndef ENABLE_UEFI
#include<signal.h>
#include"gui.h"
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"service.h"
#include"language.h"
#include"init_internal.h"
#include"gui/tools.h"
#define TAG "bootmenu"

struct bootmenu;
struct bootmenu_item;

struct bootmenu{
	lv_coord_t bm,bw,bh,bi,si;
	lv_label_long_mode_t lm;
	lv_obj_t*scr,*last_btn;
	lv_obj_t*bg,*page,*btn,*title,*time;
	lv_task_t*auto_boot;
	list*items;
	int timeout;
	char*def;
	char bg_path[PATH_MAX];
	struct bootmenu_item*selected;
};
struct bootmenu_item{
	struct bootmenu*bm;
	boot_config cfg;
	lv_obj_t*btn,*img,*title;
};

static int bootmenu_item_free(void*d){
	struct bootmenu_item*bi=d;
	if(bi){
		lv_obj_del(bi->btn);
		free(bi);
	}
	return 0;
}

static int bootmenu_clean(struct bootmenu*bm){
	lv_obj_set_enabled(bm->btn,false);
	list_free_all(bm->items,bootmenu_item_free);
	bm->last_btn=NULL,bm->selected=NULL;
	return 0;
}

static int bootmenu_send(){
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,"default-boot");
	init_send(&msg,&response);
	return 0;
}

static void bootmenu_boot(lv_task_t*tsk){
	struct bootmenu*bm;
	struct bootmenu_item*bi;
	if(!(bm=tsk->user_data)||!(bi=bm->selected))return;
	confd_set_string("boot.current",bi->cfg.ident);
	confd_set_save("boot.current",false);
	gui_run_and_exit(bootmenu_send);
}

static void bootmenu_remove_auto_boot(struct bootmenu*bm){
	if(!bm->auto_boot)return;
	lv_task_del(bm->auto_boot);
	lv_obj_set_hidden(bm->time,true);
	bm->auto_boot=NULL;
}

static void bootmenu_click(lv_obj_t*obj,lv_event_t e){
	struct bootmenu*bm=lv_obj_get_user_data(obj);
	if(!bm||!bm->selected||bm->btn!=obj)return;
	bootmenu_remove_auto_boot(bm);
	if(e!=LV_EVENT_CLICKED)return;
	lv_task_create(bootmenu_boot,10,LV_TASK_PRIO_HIGHEST,bm);
}

static void bootmenu_item_click(lv_obj_t*obj,lv_event_t e){
	struct bootmenu_item*bi=lv_obj_get_user_data(obj);
	if(!bi||(bi->img!=obj&&bi->btn!=obj))return;
	bootmenu_remove_auto_boot(bi->bm);
	if(e!=LV_EVENT_CLICKED)return;
	list*o=list_first(bi->bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmenu_item*);
		if(item&&item->btn){
			lv_obj_set_checked(item->btn,false);
			lv_obj_clear_state(item->img,LV_STATE_FOCUSED);
		}
	}while((o=o->next));
	lv_obj_set_enabled(bi->bm->btn,true);
	lv_obj_set_checked(bi->btn,true);
	bi->bm->selected=bi;
}

static void bootmenu_auto_boot_cb(lv_task_t*tsk);
static void bootmenu_auto_boot(struct bootmenu*bm){
	if(bm->timeout<=0){
		bootmenu_remove_auto_boot(bm);
		bootmenu_click(bm->btn,LV_EVENT_CLICKED);
	}else if(!bm->auto_boot){
		bm->auto_boot=lv_task_create(
			bootmenu_auto_boot_cb,
			1000,LV_TASK_PRIO_MID,bm
		);
	}else{
		lv_obj_set_hidden(bm->time,false);
		char*tout=confd_get_string("boot.timeout_text",NULL);
		lv_label_set_text_fmt(bm->time,tout?tout:_("Auto boot in %d seconds"),bm->timeout);
		lv_obj_align(bm->time,bm->page,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size/2);
		if(tout)free(tout);
		bm->timeout--;
		lv_disp_trig_activity(NULL);
	}
}

static void bootmenu_auto_boot_cb(lv_task_t*tsk){
	bootmenu_auto_boot(tsk->user_data);
}

static void bootmenu_add(struct bootmenu*bm,char*c){
	struct bootmenu_item*bi=malloc(sizeof(struct bootmenu_item));
	if(!bi)return;
	memset(bi,0,sizeof(struct bootmenu_item));
	boot_config*cfg=boot_get_config(c);
	if(!cfg)goto fail;
	memcpy(&bi->cfg,cfg,sizeof(boot_config));
	free(cfg);
	if(!bi->cfg.show||!bi->cfg.enabled)goto fail;
	bi->bm=bm;
	bi->btn=lv_btn_create(bm->page,NULL);
	lv_obj_set_size(bi->btn,bm->bw,bm->bh);
	lv_obj_set_user_data(bi->btn,bi);
	lv_obj_set_event_cb(bi->btn,bootmenu_item_click);
	lv_style_set_btn_item(bi->btn);
	lv_obj_align(
		bi->btn,bm->last_btn,bm->last_btn?
		LV_ALIGN_OUT_BOTTOM_LEFT:
		LV_ALIGN_IN_TOP_MID,
		0,bm->bm/8+(bm->last_btn?gui_dpi/20:0)
	);
	bm->last_btn=bi->btn;
	if(list_obj_add_new(&bm->items,bi)!=0){
		telog_error("cannot add item list");
		abort();
	}

	// line for button text
	lv_obj_t*line=lv_line_create(bi->btn,NULL);
	lv_obj_set_width(line,bm->bw);

	// boot item image
	bi->img=lv_img_create(line,NULL);
	lv_obj_set_size(bi->img,bm->si,bm->si);
	lv_obj_align(bi->img,bi->btn,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
	lv_obj_set_drag_parent(bi->img,true);
	lv_obj_set_click(bi->img,true);
	lv_obj_set_user_data(bi->img,bi);
	lv_obj_set_event_cb(bi->img,bootmenu_item_click);
	lv_obj_set_style_local_outline_width(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/100);
	lv_obj_set_style_local_outline_color(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_obj_set_style_local_radius(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/50);
	if(bi->cfg.icon[0]=='/'||bi->cfg.icon[1]==':')lv_img_set_src(bi->img,bi->cfg.icon);
	else{
		char ipath[PATH_MAX];
		memset(ipath,0,PATH_MAX);
		snprintf(ipath,PATH_MAX-1,IMG_RES"/%s",bi->cfg.icon);
		lv_img_set_src(bi->img,ipath);
	}
	lv_img_ext_t*ext=lv_obj_get_ext_attr(bi->img);
	if((ext->w<=0||ext->h<=0))lv_img_set_src(bi->img,IMG_RES"/apps.svg");
	lv_img_fill_image(bi->img,bm->si,bm->si);
	lv_group_add_obj(gui_grp,bi->img);

	bi->title=lv_label_create(line,NULL);
	lv_label_set_long_mode(bi->title,bm->lm);
	lv_label_set_text(bi->title,_(bi->cfg.desc));
	lv_obj_set_width(bi->title,bm->bw-bm->si-(gui_font_size*2));
	lv_obj_set_user_data(bi->title,bi);
	lv_obj_align(
		bi->title,NULL,
		LV_ALIGN_IN_LEFT_MID,
		gui_font_size+bm->si,0
	);

	if(strcmp(bm->def,bi->cfg.ident)==0){
		lv_obj_set_enabled(bm->btn,true);
		lv_obj_set_checked(bi->btn,true);
		bi->bm->selected=bi;
	}
	lv_group_add_obj(gui_grp,bi->img);
	return;
	fail:
	free(bi);
}

static int bootmenu_load(struct bootmenu*bm){
	bootmenu_clean(bm);
	bm->bm=gui_font_size;
	bm->bw=lv_page_get_scrl_width(bm->page)-bm->bm;
	bm->bh=gui_font_size*3,bm->si=bm->bh-gui_font_size;
	bm->lm=confd_get_boolean("gui.text_scroll",true)?LV_LABEL_LONG_SROLL_CIRC:LV_LABEL_LONG_DOT;
	lv_group_remove_obj(bm->btn);
	char**cs=confd_ls("boot.configs");
	if(cs){
		bm->timeout=confd_get_integer("boot.timeout",10);
		bm->def=confd_get_string("boot.current",NULL);
		if(!bm->def)bm->def=confd_get_string("boot.default",NULL);
		for(size_t i=0;cs[i];i++)bootmenu_add(bm,cs[i]);
		if(cs[0])free(cs[0]);
		if(cs)free(cs);
		if(bm->def)free(bm->def);
		bm->def=NULL;
		if(bm->selected&&bm->timeout>=0)bootmenu_auto_boot(bm);
	}
	lv_group_add_obj(gui_grp,bm->btn);
	return 0;
}

static int bootmenu_draw(){
	lv_coord_t pw=MIN((lv_coord_t)gui_w-(gui_dpi/4),gui_dpi*4);
	lv_coord_t ph=MIN((lv_coord_t)gui_h/4*3,gui_dpi*4);
	struct bootmenu*bm=malloc(sizeof(struct bootmenu));
	if(!bm)abort();
	memset(bm,0,sizeof(struct bootmenu));

	char*def_bg=IMG_RES"/bg.jpg",*bg,*key="gui.background";
	if(confd_get_type(key)!=TYPE_STRING)confd_set_string(key,def_bg);
	else if((bg=confd_get_string(key,NULL))){
		strncpy(bm->bg_path,bg,sizeof(bm->bg_path)-1);
		free(bg);
	}
	if(!bm->bg_path[0])strncpy(bm->bg_path,def_bg,sizeof(bm->bg_path)-1);

	bm->scr=lv_obj_create(lv_scr_act(),NULL);
	lv_obj_set_pos(bm->scr,0,0);
	lv_obj_set_size(bm->scr,gui_w,gui_h);
	lv_theme_apply(bm->scr,LV_THEME_SCR);

	bm->bg=lv_img_create(bm->scr,NULL);
	lv_obj_set_size(bm->bg,gui_sw,gui_sh);
	lv_obj_set_pos(bm->bg,0,0);
	lv_obj_set_hidden(bm->bg,true);
	if(confd_get_boolean("gui.show_background",true)){
		lv_img_set_src(bm->bg,bm->bg_path);
		lv_img_fill_image(bm->bg,gui_sw,gui_sh);
		lv_img_ext_t*x=lv_obj_get_ext_attr(bm->bg);
		if(x->w>0&&x->h>0)lv_obj_set_hidden(bm->bg,false);
	}

	char*title=confd_get_string("boot.title",NULL);
	bm->title=lv_label_create(bm->scr,NULL);
	lv_label_set_text(bm->title,title?title:_("Boot Menu"));
	lv_obj_set_style_local_bg_color(bm->title,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_COLOR_GRAY);
	lv_obj_set_style_local_bg_opa(bm->title,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_OPA_50);
	lv_obj_set_style_local_pad_ver(bm->title,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/4);
	lv_obj_set_style_local_pad_hor(bm->title,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/2);
	lv_obj_set_style_local_radius(bm->title,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/4);
	lv_obj_align(bm->title,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);
	if(title)free(title);

	bm->page=lv_page_create(bm->scr,NULL);
	lv_obj_set_size(bm->page,pw,ph);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_bg_color(bm->page,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_COLOR_GRAY);
	lv_obj_set_style_local_bg_opa(bm->page,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_OPA_50);
	lv_obj_align(bm->page,NULL,LV_ALIGN_CENTER,0,-(gui_dpi/5));

	bm->time=lv_label_create(bm->scr,NULL);
	lv_obj_set_style_local_text_font(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_small);
	lv_obj_set_style_local_bg_color(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_COLOR_GRAY);
	lv_obj_set_style_local_bg_opa(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,LV_OPA_50);
	lv_obj_set_style_local_pad_ver(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/4);
	lv_obj_set_style_local_pad_hor(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/2);
	lv_obj_set_style_local_radius(bm->time,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size/4);
	lv_obj_align(bm->time,bm->page,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size/2);
	lv_obj_set_hidden(bm->time,true);

	bm->btn=lv_btn_create(bm->scr,NULL);
	lv_label_set_text(lv_label_create(bm->btn,NULL),_("Boot"));
	lv_obj_set_size(bm->btn,gui_w/3,gui_font_size/2*5);
	lv_obj_align(bm->btn,0,LV_ALIGN_IN_BOTTOM_MID,0,-(gui_dpi/5));
	lv_obj_set_checked(bm->btn,true);
	lv_obj_set_enabled(bm->btn,false);
	lv_obj_set_state(bm->btn,LV_STATE_CHECKED);
	lv_obj_set_user_data(bm->btn,bm);
	lv_obj_set_event_cb(bm->btn,bootmenu_click);
	lv_obj_set_style_local_radius(bm->btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size/2);
	bootmenu_load(bm);
	return 0;
}

static int start_default(){
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,"default-boot");
	init_send(&msg,&response);
	return 0;
}

int bootmenu_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	open_socket_initfd(DEFAULT_INITD,false);
	lang_init_locale();
	if(confd_get_boolean("runtime.cmdline.gui_disable",false))return start_default();
	if(
		gui_pre_init()==0&&
		gui_screen_init()==0&&
		bootmenu_draw()==0&&
		gui_main()==0
	)return 0;
	tlog_error("gui initialize failed");
	return start_default();
}

static int bootmenu_startup(struct service*svc __attribute__((unused))){
	return bootmenu_main(0,NULL);
}

int register_bootmenu(){
	struct service*bootmenu=svc_create_service("bootmenu",WORK_FOREGROUND);
	if(bootmenu){
		svc_set_desc(bootmenu,"Boot Menu");
		svc_set_start_function(bootmenu,bootmenu_startup);
		svc_add_depend(svc_default,bootmenu);
	}
	return 0;
}
#endif
#endif

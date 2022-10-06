/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef ENABLE_UEFI
#include<sys/prctl.h>
#endif
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"service.h"
#include"proctitle.h"
#include"language.h"
#include"init_internal.h"
#define TAG "bootmenu"
#ifdef ENABLE_GUI
#include"gui.h"
#include"gui/tools.h"

struct bootmenu;
struct bootmenu_item;

struct bootmenu{
	bool anim,anim_done;
	lv_anim_t anim_exec;
	lv_coord_t si,bw;
	lv_label_long_mode_t lm;
	lv_obj_t*scr;
	lv_obj_t*bg,*page,*btn,*title,*time;
	lv_timer_t*auto_boot;
	list*items;
	bool do_auto_boot;
	int timeout;
	char*def;
	char bg_path[PATH_MAX];
	char folder[64];
	struct bootmenu_item*selected;
};
struct bootmenu_item{
	struct bootmenu*bm;
	bool parent;
	boot_config cfg;
	lv_obj_t*btn;
	lv_obj_t*w_img,*img;
	lv_obj_t*title,*chk;
};

static bool no_autoboot=false;

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
	bm->selected=NULL,bm->items=NULL;
	return 0;
}

#ifdef ENABLE_UEFI
static void enter_simple_init(void*d __attribute__((unused))){
	gui_screen_init();
	gui_draw();
}

static char target_boot_name[PATH_MAX];
int bootmenu_main(int argc __attribute((unused)),char**argv __attribute((unused)));
extern int bootmenu_draw();
static int after_exit(void*d __attribute__((unused))){
	boot_name(target_boot_name);
	memset(target_boot_name,0,sizeof(target_boot_name));
	gui_set_run_exit(NULL);
	no_autoboot=true;
	gui_run=true;
	#ifdef ENABLE_LUA
	if(!gui_global_lua)
		gui_global_lua=xlua_init();
	#endif
	return
		gui_screen_init()==0&&
		bootmenu_draw()==0&&
		gui_main()==0;
}
#else
static int bootmenu_send(){
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,"default-boot");
	init_send(&msg,&response);
	return 0;
}
#endif

static int bootmenu_load(struct bootmenu*bm);
static bool bootmenu_chdir(struct bootmenu_item*bi){
	if(bi->cfg.mode!=BOOT_FOLDER)return false;
	memset(bi->bm->folder,0,sizeof(bi->bm->folder));
	strncpy(bi->bm->folder,bi->cfg.ident,sizeof(bi->bm->folder)-1);
	tlog_debug("enter folder %s",bi->cfg.ident);
	bootmenu_load(bi->bm);
	return true;
}

static void bootmenu_boot(void*data){
	struct bootmenu*bm=data;
	struct bootmenu_item*bi;
	if(!bm||!(bi=bm->selected))return;
	if(bootmenu_chdir(bi))return;
	tlog_debug("run config %s",bi->cfg.ident);
	confd_set_string("boot.current",bi->cfg.ident);
	confd_set_save("boot.current",false);
	#ifdef ENABLE_UEFI
	if(bi->cfg.mode==BOOT_SIMPLE_INIT){
		lv_async_call(enter_simple_init,NULL);
		return;
	}
	memset(
		target_boot_name,0,
		sizeof(target_boot_name)
	);
	strncpy(
		target_boot_name,
		bi->cfg.ident,
		sizeof(target_boot_name)-1
	);
	gui_run_and_exit(after_exit);
	#else
	gui_run_and_exit(bootmenu_send);
	#endif
}

static void bootmenu_remove_auto_boot(struct bootmenu*bm){
	if(!bm->auto_boot)return;
	lv_timer_del(bm->auto_boot);
	lv_obj_set_hidden(bm->time,true);
	bm->auto_boot=NULL;
}

static void bootmenu_remove_auto_boot_cb(lv_event_t*e){
	bootmenu_remove_auto_boot(e->user_data);
}

static void bootmenu_click(lv_event_t*e){
	lv_async_call(bootmenu_boot,e->user_data);
}

static void bootmenu_item_released(lv_event_t*e){
	struct bootmenu_item*bi=e->user_data;
	if(!bi->parent)lv_group_focus_obj(bi->bm->btn);
}

static void bootmenu_item_focused(lv_event_t*e){
	struct bootmenu_item*bi=e->user_data;
	lv_obj_scroll_to_view(bi->btn,LV_ANIM_ON);
}

static void bootmenu_item_click(lv_event_t*e){
	struct bootmenu_item*bi=e->user_data;
	if(bi->parent){
		memset(
			bi->bm->folder,0,
			sizeof(bi->bm->folder)
		);
		boot_config*cfg=boot_get_config(bi->bm->folder);
		if(cfg){
			strncpy(
				bi->bm->folder,
				cfg->parent,
				sizeof(bi->bm->folder)-1
			);
			free(cfg);
		}
		bootmenu_load(bi->bm);
		return;
	}
	if(bootmenu_chdir(bi))return;
	list*o=list_first(bi->bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmenu_item*);
		if(item&&item->btn){
			lv_obj_set_checked(item->btn,false);
			lv_obj_clear_state(item->img,LV_STATE_FOCUSED);
			lv_obj_add_flag(item->chk,LV_OBJ_FLAG_HIDDEN);
		}
	}while((o=o->next));
	lv_obj_set_enabled(bi->bm->btn,true);
	lv_obj_set_checked(bi->btn,true);
	lv_obj_clear_flag(bi->chk,LV_OBJ_FLAG_HIDDEN);
	bi->bm->selected=bi;
}

static void bootmenu_auto_boot_cb(lv_timer_t*tsk);
static void bootmenu_auto_boot(struct bootmenu*bm){
	if(bm->timeout<=0){
		bootmenu_remove_auto_boot(bm);
		bootmenu_click(&(lv_event_t){.user_data=bm});
	}else if(!bm->auto_boot){
		bm->auto_boot=lv_timer_create(
			bootmenu_auto_boot_cb,1000,bm
		);
	}else{
		if(bm->anim&&!bm->anim_done)return;
		lv_obj_set_hidden(bm->time,false);
		char*tout=confd_get_string("boot.timeout_text",NULL);
		lv_label_set_text_fmt(
			bm->time,
			tout?tout:_("Auto boot in %d seconds"),
			bm->timeout
		);
		if(tout)free(tout);
		bm->timeout--;
		lv_disp_trig_activity(NULL);
	}
}

static void bootmenu_auto_boot_cb(lv_timer_t*tsk){
	bootmenu_auto_boot(tsk->user_data);
}

static void bootmenu_add(struct bootmenu*bm,char*c){
	struct bootmenu_item*bi;
	static lv_coord_t grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_col[]={
		0,LV_GRID_FR(1),0,
		LV_GRID_TEMPLATE_LAST
	};
	if(!(bi=malloc(sizeof(struct bootmenu_item))))return;
	memset(bi,0,sizeof(struct bootmenu_item));
	if(grid_col[0]!=bm->si)grid_col[0]=bm->si;
	if(grid_col[2]!=bm->si)grid_col[2]=bm->si;
	if(c){
		boot_config*cfg=boot_get_config(c);
		if(!cfg)goto fail;
		memcpy(&bi->cfg,cfg,sizeof(boot_config));
		free(cfg);
		if(!bi->cfg.show||!bi->cfg.enabled)goto fail;
		if(strncmp(
			bi->cfg.parent,
			bm->folder,
			sizeof(bm->folder)
		)!=0)goto fail;
	}else{
		if(!bm->folder[0])goto fail;
		bi->parent=true;
	}
	bi->bm=bm;
	bi->btn=lv_obj_create(bm->page);
	lv_obj_set_size(bi->btn,bm->bw,LV_SIZE_CONTENT);
	lv_obj_set_scroll_dir(bi->btn,LV_DIR_NONE);
	lv_obj_set_grid_dsc_array(bi->btn,grid_col,grid_row);
	lv_obj_add_event_cb(bi->btn,bootmenu_item_click,LV_EVENT_CLICKED,bi);
	lv_obj_add_event_cb(bi->btn,bootmenu_item_released,LV_EVENT_RELEASED,bi);
	lv_obj_add_event_cb(bi->btn,bootmenu_item_focused,LV_EVENT_FOCUSED,bi);
	lv_obj_set_style_outline_width(bi->btn,gui_dpi/100,LV_STATE_FOCUSED);
	lv_obj_set_style_outline_color(bi->btn,lv_theme_get_color_primary(bi->btn),LV_STATE_FOCUSED);
	lv_group_add_obj(gui_grp,bi->btn);
	if(list_obj_add_new(&bm->items,bi)!=0){
		telog_error("cannot add item list");
		abort();
	}

	// boot item image
	bi->w_img=lv_draw_wrapper(bi->btn,NULL,NULL,bm->si,bm->si);
	lv_obj_set_grid_cell(
		bi->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	bi->img=lv_img_create(bi->w_img);
	lv_obj_clear_flag(bi->img,LV_OBJ_FLAG_CLICKABLE);
	lv_img_set_size_mode(bi->img,LV_IMG_SIZE_MODE_REAL);
	if(c)lv_img_src_try(bi->img,"bootitem",bi->cfg.ident,bi->cfg.icon);
	else lv_img_src_try(bi->img,"bootitem",NULL,"@bootitem-back");
	lv_img_fill_image(bi->img,bm->si,bm->si);
	lv_obj_center(bi->img);

	bi->title=lv_label_create(bi->btn);
	lv_obj_clear_flag(bi->title,LV_OBJ_FLAG_CLICKABLE);
	lv_label_set_long_mode(bi->title,bm->lm);
	lv_label_set_text(bi->title,_(c?bi->cfg.desc:"Go back"));
	lv_obj_set_align(bi->title,LV_ALIGN_LEFT_MID);
	lv_obj_set_grid_cell(
		bi->title,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	bi->chk=lv_img_create(bi->btn);
	lv_obj_add_flag(bi->chk,LV_OBJ_FLAG_HIDDEN);
	lv_img_set_src(bi->chk,LV_SYMBOL_OK);
	lv_img_set_size_mode(bi->chk,LV_IMG_SIZE_MODE_REAL);
	lv_obj_set_grid_cell(
		bi->chk,
		LV_GRID_ALIGN_END,2,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	if(c)switch(bi->cfg.mode){
		case BOOT_NONE:
		case BOOT_CHARGER:
		case BOOT_KEXEC:
		case BOOT_REBOOT:
		case BOOT_POWEROFF:
		case BOOT_HALT:
		case BOOT_FOLDER:break;
		default:
			if(no_autoboot)break;
			if(!bm->do_auto_boot)break;
			if(strcmp(bm->def,bi->cfg.ident)!=0)break;
			lv_obj_set_enabled(bm->btn,true);
			lv_obj_clear_flag(bi->chk,LV_OBJ_FLAG_HIDDEN);
			lv_obj_scroll_to_view(bi->btn,LV_ANIM_OFF);
			lv_group_focus_obj(bi->btn);
			bi->bm->selected=bi;
		break;
	}
	lv_obj_add_event_cb(
		bi->btn,
		bootmenu_remove_auto_boot_cb,
		LV_EVENT_FOCUSED,bm
	);
	return;
	fail:
	free(bi);
}

static int bootmenu_load(struct bootmenu*bm){
	if(bm->items)bm->do_auto_boot=false;
	lv_group_remove_obj(bm->btn);
	bootmenu_clean(bm);
	bm->si=gui_font_size*2;
	bm->lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;
	char**cs=confd_ls("boot.configs");
	if(cs){
		bm->timeout=confd_get_integer("boot.timeout",10);
		bm->def=confd_get_string("boot.current",NULL);
		if(!bm->def)bm->def=confd_get_string("boot.default",NULL);
		if(bm->folder[0])bootmenu_add(bm,NULL);
		for(size_t i=0;cs[i];i++)bootmenu_add(bm,cs[i]);
		if(cs[0])free(cs[0]);
		if(cs)free(cs);
		if(bm->def)free(bm->def);
		bm->def=NULL;
		if(bm->selected&&bm->timeout>=0)
			bootmenu_auto_boot(bm);
	}
	lv_group_add_obj(gui_grp,bm->btn);
	return 0;
}

static void anim_ready_cb(lv_anim_t*anim){
	struct bootmenu*bm=lv_anim_get_user_data(anim);
	bm->anim_done=true;
}

static void anim_opa(void*obj,int32_t val){
	lv_obj_set_style_opa(obj,val,0);
}

static void anim_init(struct bootmenu*bm){
	bm->anim=true,bm->anim_done=false;
	lv_obj_set_style_opa(bm->scr,LV_OPA_TRANSP,0);
	lv_anim_init(&bm->anim_exec);
	lv_anim_set_var(&bm->anim_exec,bm->scr);
	lv_anim_set_user_data(&bm->anim_exec,bm);
	lv_anim_set_values(&bm->anim_exec,LV_OPA_TRANSP,LV_OPA_COVER);
	lv_anim_set_ready_cb(&bm->anim_exec,anim_ready_cb);
	lv_anim_set_exec_cb(&bm->anim_exec,(lv_anim_exec_xcb_t)anim_opa);
	lv_anim_set_time(&bm->anim_exec,500);
	lv_anim_start(&bm->anim_exec);
}

int bootmenu_draw(){
	struct bootmenu*bm=malloc(sizeof(struct bootmenu));
	if(!bm)abort();
	memset(bm,0,sizeof(struct bootmenu));

	char*path;
	if(
		!bm->bg_path[0]&&
		(path=confd_get_string("gui.background",NULL))
	){
		strncpy(
			bm->bg_path,path,
			sizeof(bm->bg_path)-1
		);
		free(path);
	}
	bm->do_auto_boot=true;

	bm->scr=lv_draw_wrapper(lv_scr_act(),NULL,NULL,gui_w,gui_h);
	lv_obj_set_style_pad_all(bm->scr,gui_font_size/2,0);
	lv_obj_set_flex_flow(bm->scr,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(
		bm->scr,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER
	);

	bm->bg=lv_img_create(bm->scr);
	lv_obj_set_size(bm->bg,gui_sw,gui_sh);
	lv_obj_set_pos(bm->bg,0,0);
	lv_obj_set_hidden(bm->bg,true);
	if(confd_get_boolean("gui.show_background",true)){
		lv_img_set_src(bm->bg,bm->bg_path);
		lv_img_fill_image(bm->bg,gui_sw,gui_sh);
		lv_img_t*x=(lv_img_t*)bm->bg;
		if(x->w>0&&x->h>0)lv_obj_set_hidden(bm->bg,false);
	}

	char*title=confd_get_string("boot.title",NULL);
	bm->title=lv_label_create(bm->scr);
	lv_label_set_text(bm->title,title?title:_("Boot Menu"));
	lv_obj_set_style_bg_opa(bm->title,LV_OPA_0,0);
	lv_obj_set_style_pad_ver(bm->title,gui_font_size/4,0);
	lv_obj_set_style_pad_hor(bm->title,gui_font_size/2,0);
	lv_obj_set_style_radius(bm->title,gui_font_size/4,0);
	lv_obj_set_style_text_align(bm->title,LV_TEXT_ALIGN_CENTER,0);
	if(title)free(title);

	bm->page=lv_obj_create(bm->scr);
	lv_obj_set_scroll_dir(bm->page,LV_DIR_VER);
	lv_obj_set_style_border_width(bm->page,0,0);
	lv_obj_set_style_bg_opa(bm->page,LV_OPA_0,LV_STATE_DEFAULT);
	lv_obj_add_event_cb(bm->page,bootmenu_remove_auto_boot_cb,LV_EVENT_PRESSING,bm);
	lv_obj_set_width(bm->page,lv_pct(100));
	lv_obj_set_flex_flow(bm->page,LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_grow(bm->page,1);

	bm->time=lv_label_create(bm->scr);
	lv_obj_set_style_text_font(bm->time,gui_font_small,LV_STATE_DEFAULT);
	lv_obj_set_hidden(bm->time,true);

	bm->btn=lv_draw_button(bm->scr,_("Boot"),false,bootmenu_click,bm);
	lv_obj_set_style_max_width(bm->btn,gui_dpi*2,0);
	lv_obj_set_width(bm->btn,lv_pct(100));

	lv_obj_update_layout(bm->page);
	bm->bw=(
		lv_obj_get_width(bm->page)-
		lv_obj_get_style_pad_right(bm->page,0)-
		lv_obj_get_style_pad_column(bm->page,0)-
		lv_obj_get_style_pad_left(bm->page,0)
	)/lv_coord_border(confd_get_integer(
		"gui.bootmenu.column",gui_w>gui_h?2:1
	),255,1);
	bootmenu_load(bm);

	if(confd_get_boolean("gui.anim",true))anim_init(bm);
	return 0;
}

#endif

extern int guiapp_main(int argc,char**argv);
static int start_default(){
	#ifdef ENABLE_UEFI
	return guiapp_main(1,(char*[]){"guiapp",NULL});
	#else
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,"default-boot");
	init_send(&msg,&response);
	return 0;
	#endif
}

int bootmenu_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	#ifdef ENABLE_UEFI
	boot_init_configs();
	#else
	open_socket_logfd_default();
	open_default_confd_socket(false,TAG);
	open_socket_initfd(DEFAULT_INITD,false);
	lang_init_locale();
	prctl(PR_SET_NAME,"GUI Boot Menu");
	setproctitle("bootmenu");
	if(confd_get_boolean("runtime.cmdline.gui_disable",false))return start_default();
	#endif
	#ifdef ENABLE_GUI
	if(
		gui_pre_init()==0&&
		gui_screen_init()==0&&
		bootmenu_draw()==0&&
		gui_main()==0
	)return 0;
	tlog_error("gui initialize failed");
	#else
	tlog_notice("skip gui boot menu");
	#endif
	return start_default();
}

#ifndef ENABLE_UEFI
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

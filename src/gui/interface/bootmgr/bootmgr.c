/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootmgr_item_free(void*d){
	struct bootmgr_item*bi=d;
	if(bi){
		lv_obj_del(bi->btn);
		free(bi);
	}
	return 0;
}

static int bootmgr_clean(struct bootmgr*bm){
	lv_obj_set_hidden(bm->btn_open,true);
	lv_obj_set_hidden(bm->btn_default,false);
	lv_obj_set_enabled(bm->btn_open,false);
	lv_obj_set_enabled(bm->btn_default,false);
	lv_obj_set_enabled(bm->btn_edit,false);
	lv_obj_set_enabled(bm->btn_delete,false);
	list_free_all(bm->items,bootmgr_item_free);
	bm->last_btn=NULL,bm->selected=NULL,bm->items=NULL;
	return 0;
}

static int bootmgr_load(struct bootmgr*bm);
static bool bootmgr_chdir(struct bootmgr_item*bi){
	if(bi->cfg.mode!=BOOT_FOLDER)return false;
	memset(bi->bm->folder,0,sizeof(bi->bm->folder));
	strncpy(bi->bm->folder,bi->cfg.ident,sizeof(bi->bm->folder)-1);
	tlog_debug("enter folder %s",bi->cfg.ident);
	bootmgr_load(bi->bm);
	return true;
}

static void bootmgr_item_click(lv_obj_t*obj,lv_event_t e){
	struct bootmgr_item*bi=lv_obj_get_user_data(obj);
	if(!bi||(bi->img!=obj&&bi->btn!=obj))return;
	if(e!=LV_EVENT_CLICKED)return;
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
		bootmgr_load(bi->bm);
		return;
	}
	list*o=list_first(bi->bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmgr_item*);
		if(item&&item->btn){
			lv_obj_set_checked(item->btn,false);
			lv_obj_clear_state(item->img,LV_STATE_FOCUSED);
		}
	}while((o=o->next));
	char*def=confd_get_string("boot.default",NULL);
	bool d=!def||strcasecmp(def,bi->cfg.ident)!=0;
	lv_obj_set_checked(bi->btn,true);
	lv_obj_set_hidden(bi->bm->btn_open,bi->cfg.mode!=BOOT_FOLDER);
	lv_obj_set_hidden(bi->bm->btn_default,bi->cfg.mode==BOOT_FOLDER);
	lv_obj_set_enabled(bi->bm->btn_open,d);
	lv_obj_set_enabled(bi->bm->btn_default,d);
	lv_obj_set_enabled(bi->bm->btn_edit,true);
	lv_obj_set_enabled(bi->bm->btn_delete,true);
	if(def)free(def);
	bi->bm->selected=bi;
}

static void bootmgr_add(struct bootmgr*bm,char*c){
	struct bootmgr_item*bi=malloc(sizeof(struct bootmgr_item));
	if(!bi)return;
	memset(bi,0,sizeof(struct bootmgr_item));
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
	bi->btn=lv_btn_create(bm->page,NULL);
	lv_obj_set_size(bi->btn,bm->bw,bm->bh);
	lv_obj_set_user_data(bi->btn,bi);
	lv_obj_set_event_cb(bi->btn,bootmgr_item_click);
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
	lv_obj_set_event_cb(bi->img,bootmgr_item_click);
	lv_obj_set_style_local_outline_width(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/100);
	lv_obj_set_style_local_outline_color(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_obj_set_style_local_radius(bi->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/50);
	if(!c)lv_img_set_src(bi->img,IMG_RES"/back.svg");
	else if(bi->cfg.icon[0]=='/'||bi->cfg.icon[1]==':')lv_img_set_src(bi->img,bi->cfg.icon);
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
	lv_label_set_text(bi->title,_(c?bi->cfg.desc:"Go back"));
	lv_obj_set_width(bi->title,bm->bw-bm->si-(gui_font_size*2));
	lv_obj_set_user_data(bi->title,bi);
	lv_obj_align(
		bi->title,NULL,
		LV_ALIGN_IN_LEFT_MID,
		gui_font_size+bm->si,0
	);

	lv_group_add_obj(gui_grp,bi->img);
	return;
	fail:
	free(bi);
}

static int bootmgr_load(struct bootmgr*bm){
	bootmgr_clean(bm);
	bm->bm=gui_font_size;
	bm->bw=lv_page_get_scrl_width(bm->page)-bm->bm;
	bm->bh=gui_font_size*3,bm->si=bm->bh-gui_font_size;
	bm->lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT;
	char**cs=confd_ls(bootmgr_base);
	if(cs){
		if(bm->folder[0])bootmgr_add(bm,NULL);
		for(size_t i=0;cs[i];i++)bootmgr_add(bm,cs[i]);
		if(cs[0])free(cs[0]);
		if(cs)free(cs);
	}
	return 0;
}

static int do_load(struct gui_activity*a){
	return bootmgr_load(a->data);
}

static bool default_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	struct bootmgr*bm=user_data;
	if(!bm||!bm->selected)return true;
	if(id==0){
		confd_set_string("boot.default",bm->selected->cfg.ident);
		lv_obj_set_enabled(bm->btn_default,false);
	}
	return false;
}

static bool delete_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	struct bootmgr*bm=user_data;
	if(!bm||!bm->selected)return true;
	if(id==0){
		confd_delete(bm->selected->cfg.base);
		bootmgr_load(bm);
	}
	return false;
}

static void btn_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct bootmgr*bm=lv_obj_get_user_data(obj);
	if(!bm)return;
	struct bootmgr_item*sel=bm->selected;
	if(obj==bm->btn_default){
		if(!sel)return;
		msgbox_set_user_data(msgbox_create_yesno(
			default_cb,
			"Are you sure you want to set boot item '%s' (%s) as default?",
			sel->cfg.desc[0]?sel->cfg.desc:sel->cfg.ident,sel->cfg.ident
		),bm);
	}else if(obj==bm->btn_open){
		if(!sel)return;
		bootmgr_chdir(sel);
	}else if(obj==bm->btn_edit){
		if(!sel)return;
		guiact_start_activity(&guireg_bootitem,sel->cfg.ident);
	}else if(obj==bm->btn_delete){
		if(!sel)return;
		msgbox_set_user_data(msgbox_create_yesno(
			delete_cb,
			"Are you sure you want to delete the boot item '%s' (%s)?",
			sel->cfg.desc[0]?sel->cfg.desc:sel->cfg.ident,sel->cfg.ident
		),bm);
	}else if(obj==bm->btn_create){
		guiact_start_activity(&guireg_bootitem,NULL);
	}else if(obj==bm->btn_reload){
		bootmgr_load(bm);
	}else if(obj==bm->btn_setting){
	}
}

static int bootmgr_resize(struct gui_activity*a){
	struct bootmgr*bm=a->data;
	if(!bm)return 0;
	lv_obj_set_width(bm->page,gui_w-(gui_dpi/4));
	lv_obj_align(bm->title,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);
	lv_obj_align(
		bm->page,NULL,
		LV_ALIGN_IN_TOP_MID,
		0,lv_obj_get_y(bm->title)+
		lv_obj_get_height(bm->title)+
		gui_font_size
	);

	lv_coord_t btm=gui_dpi/10;
	lv_coord_t btw=a->w/3-btm;
	lv_coord_t bth=gui_font_size+gui_dpi/10;

	// set default button
	lv_obj_set_size(bm->btn_default,btw,bth);
	lv_obj_align(bm->btn_default,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-(bth+btm*2));

	// open folder button
	lv_obj_set_size(bm->btn_open,btw,bth);
	lv_obj_align(bm->btn_open,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-(bth+btm*2));

	// edit button
	lv_obj_set_size(bm->btn_edit,btw,bth);
	lv_obj_align(bm->btn_edit,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-(bth+btm*2));

	// delete button
	lv_obj_set_size(bm->btn_delete,btw,bth);
	lv_obj_align(bm->btn_delete,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-(bth+btm*2));

	// create button
	lv_obj_set_size(bm->btn_create,btw,bth);
	lv_obj_align(bm->btn_create,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);

	// reload button
	lv_obj_set_size(bm->btn_reload,btw,bth);
	lv_obj_align(bm->btn_reload,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-btm);

	// settings button
	lv_obj_set_size(bm->btn_setting,btw,bth);
	lv_obj_align(bm->btn_setting,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-btm);

	lv_obj_set_height(
		bm->page,a->h-
		lv_obj_get_y(bm->page)-
		(bth*2)-(btm*2)-
		gui_font_size
	);
	return 0;
}

static int init(struct gui_activity*act){
	struct bootmgr*bm=malloc(sizeof(struct bootmgr));
	if(!bm)return -ENOMEM;
	memset(bm,0,sizeof(struct bootmgr));
	act->data=bm;
	return 0;
}

static int bootmgr_draw(struct gui_activity*act){
	struct bootmgr*bm=act->data;
	if(!bm)return -1;

	bm->title=lv_label_create(act->page,NULL);
	lv_label_set_text(bm->title,_("Boot Manager"));

	bm->page=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(bm->page,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);

	// set default button
	bm->btn_default=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_default,bm);
	lv_obj_set_event_cb(bm->btn_default,btn_click);
	lv_style_set_action_button(bm->btn_default,false);
	lv_label_set_text(lv_label_create(bm->btn_default,NULL),_("Default"));

	// open folder button
	bm->btn_open=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_open,bm);
	lv_obj_set_event_cb(bm->btn_open,btn_click);
	lv_obj_set_hidden(bm->btn_open,true);
	lv_style_set_action_button(bm->btn_open,false);
	lv_label_set_text(lv_label_create(bm->btn_open,NULL),_("Open"));

	// edit button
	bm->btn_edit=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_edit,bm);
	lv_obj_set_event_cb(bm->btn_edit,btn_click);
	lv_style_set_action_button(bm->btn_edit,false);
	lv_label_set_text(lv_label_create(bm->btn_edit,NULL),_("Edit"));

	// delete button
	bm->btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_delete,bm);
	lv_obj_set_event_cb(bm->btn_delete,btn_click);
	lv_style_set_action_button(bm->btn_delete,false);
	lv_label_set_text(lv_label_create(bm->btn_delete,NULL),_("Delete"));

	// create button
	bm->btn_create=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_create,bm);
	lv_obj_set_event_cb(bm->btn_create,btn_click);
	lv_style_set_action_button(bm->btn_create,true);
	lv_label_set_text(lv_label_create(bm->btn_create,NULL),_("Create"));

	// reload button
	bm->btn_reload=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_reload,bm);
	lv_obj_set_event_cb(bm->btn_reload,btn_click);
	lv_style_set_action_button(bm->btn_reload,true);
	lv_label_set_text(lv_label_create(bm->btn_reload,NULL),_("Reload"));

	// settings button
	bm->btn_setting=lv_btn_create(act->page,NULL);
	lv_obj_set_user_data(bm->btn_setting,bm);
	lv_obj_set_event_cb(bm->btn_setting,btn_click);
	lv_style_set_action_button(bm->btn_setting,true);
	lv_label_set_text(lv_label_create(bm->btn_setting,NULL),_("Settings"));

	return 0;
}

static int bootmgr_get_focus(struct gui_activity*act){
	struct bootmgr*bm=act->data;
	if(!bm)return 0;
	list*o=list_first(bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmgr_item*);
		if(item&&item->img)lv_group_add_obj(gui_grp,item->img);
	}while((o=o->next));
	lv_group_add_obj(gui_grp,bm->btn_open);
	lv_group_add_obj(gui_grp,bm->btn_default);
	lv_group_add_obj(gui_grp,bm->btn_edit);
	lv_group_add_obj(gui_grp,bm->btn_delete);
	lv_group_add_obj(gui_grp,bm->btn_create);
	lv_group_add_obj(gui_grp,bm->btn_reload);
	lv_group_add_obj(gui_grp,bm->btn_setting);
	return 0;
}

static int bootmgr_lost_focus(struct gui_activity*act){
	struct bootmgr*bm=act->data;
	if(!bm)return 0;
	list*o=list_first(bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmgr_item*);
		if(item&&item->img)lv_group_remove_obj(item->img);
	}while((o=o->next));
	lv_group_remove_obj(bm->btn_open);
	lv_group_remove_obj(bm->btn_default);
	lv_group_remove_obj(bm->btn_edit);
	lv_group_remove_obj(bm->btn_delete);
	lv_group_remove_obj(bm->btn_create);
	lv_group_remove_obj(bm->btn_reload);
	lv_group_remove_obj(bm->btn_setting);
	return 0;
}

static int do_clean(struct gui_activity*a){
	struct bootmgr*bm=a->data;
	if(!bm)return 0;
	bootmgr_clean(bm);
	free(bm);
	a->data=NULL;
	return 0;
}

static int do_back(struct gui_activity*a){
	boot_config*cfg;
	struct bootmgr*bm=a->data;
	if(!bm||!bm->folder[0])return 0;
	if(!(cfg=boot_get_config(bm->folder)))return 0;
	memset(bm->folder,0,sizeof(bm->folder));
	if(cfg->parent[0])strncpy(
		bm->folder,
		cfg->parent,
		sizeof(bm->folder)-1
	);
	free(cfg);
	bootmgr_load(bm);
	return 1;
}

struct gui_register guireg_bootmgr={
	.name="bootmgr",
	.title="Boot Manager",
	.icon="bootmgr.svg",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.init=init,
	.draw=bootmgr_draw,
	.resize=bootmgr_resize,
	.lost_focus=bootmgr_lost_focus,
	.get_focus=bootmgr_get_focus,
	.data_load=do_load,
	.back=true,
};
#endif

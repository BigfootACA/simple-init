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
	lv_obj_add_flag(bm->btn_open,LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(bm->btn_default,LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_enabled(bm->btn_open,false);
	lv_obj_set_enabled(bm->btn_default,false);
	lv_obj_set_enabled(bm->btn_edit,false);
	lv_obj_set_enabled(bm->btn_delete,false);
	list_free_all(bm->items,bootmgr_item_free);
	bm->selected=NULL,bm->items=NULL;
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

static void bootmgr_item_click(lv_event_t*e){
	struct bootmgr_item*bi=e->user_data;
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
	if(bi->bm->selected){
		if(bi==bi->bm->selected)return;
		else lv_obj_set_checked(bi->bm->selected->btn,false);
	}
	char*def=confd_get_string("boot.default",NULL);
	bool d=!def||strcasecmp(def,bi->cfg.ident)!=0;
	lv_obj_set_checked(bi->btn,true);
	if(bi->cfg.mode==BOOT_FOLDER){
		lv_obj_clear_flag(bi->bm->btn_open,LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(bi->bm->btn_default,LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_add_flag(bi->bm->btn_open,LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(bi->bm->btn_default,LV_OBJ_FLAG_HIDDEN);
	}
	lv_obj_set_enabled(bi->bm->btn_open,d);
	lv_obj_set_enabled(bi->bm->btn_default,d);
	lv_obj_set_enabled(bi->bm->btn_edit,true);
	lv_obj_set_enabled(bi->bm->btn_delete,true);
	if(def)free(def);
	bi->bm->selected=bi;
}

static void bootmgr_add(struct bootmgr*bm,char*c){
	struct bootmgr_item*bi;
	static lv_coord_t grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_col[]={
		0,LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	if(!(bi=malloc(sizeof(struct bootmgr_item))))return;
	memset(bi,0,sizeof(struct bootmgr_item));
	if(grid_col[0]!=bm->si)grid_col[0]=bm->si;
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
	bi->btn=lv_btn_create(bm->page);
	lv_style_set_btn_item(bi->btn);
	lv_obj_set_size(bi->btn,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_grid_dsc_array(bi->btn,grid_col,grid_row);
	lv_obj_add_event_cb(bi->btn,bootmgr_item_click,LV_EVENT_CLICKED,bi);
	lv_group_add_obj(gui_grp,bi->btn);
	if(list_obj_add_new(&bm->items,bi)!=0){
		telog_error("cannot add item list");
		abort();
	}

	// boot item image
	bi->w_img=lv_obj_create(bi->btn);
	lv_obj_set_size(bi->w_img,bm->si,bm->si);
	lv_obj_clear_flag(bi->w_img,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(bi->w_img,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_border_width(bi->w_img,0,0);
	lv_obj_set_style_bg_opa(bi->w_img,LV_OPA_0,0);
	lv_obj_set_grid_cell(
		bi->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	bi->img=lv_img_create(bi->w_img);
	lv_obj_clear_flag(bi->img,LV_OBJ_FLAG_CLICKABLE);
	lv_img_t*ext=(lv_img_t*)bi->img;
	lv_img_set_size_mode(bi->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_set_src(bi->img,c?bi->cfg.icon:"back.svg");
	if(ext->w<=0||ext->h<=0)lv_img_set_src(bi->img,"apps.svg");
	lv_img_fill_image(bi->img,bm->si,bm->si);
	lv_obj_center(bi->img);

	bi->title=lv_label_create(bi->btn);
	lv_obj_clear_flag(bi->title,LV_OBJ_FLAG_CLICKABLE);
	lv_label_set_long_mode(bi->title,bm->lm);
	lv_label_set_text(bi->title,_(c?bi->cfg.desc:"Go back"));
	lv_obj_set_align(bi->title,LV_ALIGN_LEFT_MID);
	lv_obj_set_grid_cell(
		bi->title,
		LV_GRID_ALIGN_START,1,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	return;
	fail:
	free(bi);
}

static int bootmgr_load(struct bootmgr*bm){
	bootmgr_clean(bm);
	bm->si=gui_font_size*2;
	bm->lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
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

static void btn_click(lv_event_t*e){
	struct bootmgr*bm=e->user_data;
	if(!bm)return;
	struct bootmgr_item*sel=bm->selected;
	if(e->target==bm->btn_default){
		if(!sel)return;
		msgbox_set_user_data(msgbox_create_yesno(
			default_cb,
			"Are you sure you want to set boot item '%s' (%s) as default?",
			sel->cfg.desc[0]?sel->cfg.desc:sel->cfg.ident,sel->cfg.ident
		),bm);
	}else if(e->target==bm->btn_open){
		if(!sel)return;
		bootmgr_chdir(sel);
	}else if(e->target==bm->btn_edit){
		if(!sel)return;
		guiact_start_activity(&guireg_bootitem,sel->cfg.ident);
	}else if(e->target==bm->btn_delete){
		if(!sel)return;
		msgbox_set_user_data(msgbox_create_yesno(
			delete_cb,
			"Are you sure you want to delete the boot item '%s' (%s)?",
			sel->cfg.desc[0]?sel->cfg.desc:sel->cfg.ident,sel->cfg.ident
		),bm);
	}else if(e->target==bm->btn_create){
		guiact_start_activity(&guireg_bootitem,NULL);
	}else if(e->target==bm->btn_reload){
		bootmgr_load(bm);
	}else if(e->target==bm->btn_setting){
	}
}

static int init(struct gui_activity*act){
	struct bootmgr*bm=malloc(sizeof(struct bootmgr));
	if(!bm)return -ENOMEM;
	memset(bm,0,sizeof(struct bootmgr));
	act->data=bm;
	return 0;
}

static int bootmgr_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	struct bootmgr*bm=act->data;
	if(!bm)return -1;
	struct btn{
		lv_obj_t**tgt;
		bool enabled;
		const char*title;
		lv_coord_t x,y;
	}buttons[]={
		{&bm->btn_default, false, "Default",  0,2},
		{&bm->btn_open,    false, "Open",     0,2},
		{&bm->btn_edit,    false, "Edit",     1,2},
		{&bm->btn_delete,  false, "Delete",   2,2},
		{&bm->btn_create,  true,  "Create",   0,3},
		{&bm->btn_reload,  true,  "Reload",   1,3},
		{&bm->btn_setting, true,  "Settings", 2,3},
		{NULL,false,NULL,0,0}
	};
	lv_obj_set_grid_dsc_array(act->page,grid_col,grid_row);

	bm->title=lv_label_create(act->page);
	lv_obj_set_width(bm->title,lv_pct(100));
	lv_obj_set_style_pad_all(bm->title,gui_font_size,0);
	lv_obj_set_style_text_align(bm->title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(bm->title,_("Boot Manager"));
	lv_obj_set_grid_cell(bm->title,LV_GRID_ALIGN_CENTER,0,3,LV_GRID_ALIGN_CENTER,0,1);

	bm->page=lv_obj_create(act->page);
	lv_obj_set_width(bm->page,lv_pct(100));
	lv_obj_set_flex_flow(bm->page,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_grid_cell(bm->page,LV_GRID_ALIGN_STRETCH,0,3,LV_GRID_ALIGN_STRETCH,1,1);

	for(size_t i=0;buttons[i].tgt;i++){
		*buttons[i].tgt=lv_btn_create(act->page);
		lv_obj_set_enabled(*buttons[i].tgt,buttons[i].enabled);
		lv_obj_add_event_cb(*buttons[i].tgt,btn_click,LV_EVENT_CLICKED,bm);
		lv_obj_t*lbl=lv_label_create(*buttons[i].tgt);
		lv_label_set_text(lbl,_(buttons[i].title));
		lv_obj_set_grid_cell(
			*buttons[i].tgt,
			LV_GRID_ALIGN_STRETCH,buttons[i].x,1,
			LV_GRID_ALIGN_STRETCH,buttons[i].y,1
		);
		lv_obj_center(lbl);
	}
	lv_obj_add_flag(bm->btn_open,LV_OBJ_FLAG_HIDDEN);

	return 0;
}

static int bootmgr_get_focus(struct gui_activity*act){
	struct bootmgr*bm=act->data;
	if(!bm)return 0;
	list*o=list_first(bm->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct bootmgr_item*);
		if(item&&item->btn)lv_group_add_obj(gui_grp,item->btn);
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
		if(item&&item->btn)lv_group_remove_obj(item->btn);
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
	.lost_focus=bootmgr_lost_focus,
	.get_focus=bootmgr_get_focus,
	.data_load=do_load,
	.back=true,
};
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<stdlib.h>
#include<string.h>
#include"regexp.h"
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"gui/tools.h"
#include"gui/fileopen.h"
#include"gui/activity.h"

struct fileopen_app;
struct fileopen{
	char path[PATH_MAX];
	lv_obj_t*box;
	lv_obj_t*label,*view;
	lv_obj_t*show_all;
	lv_obj_t*ok,*cancel;
	list*apps;
	void*user_data;
	struct gui_activity*act;
	struct fileopen_app*selected;
};

struct fileopen_app{
	struct fileopen*fo;
	struct gui_register*reg;
	lv_obj_t*btn,*w_img,*img,*lbl;
};

static int clean_app(void*d){
	struct fileopen_app*i=d;
	if(i){
		if(i->btn)lv_obj_del(i->btn);
		free(i);
	}
	return 0;
}

static void clean_apps(struct fileopen*fo){
	list_free_all(fo->apps,clean_app);
	fo->apps=NULL,fo->selected=NULL;
	lv_obj_set_enabled(fo->ok,false);
}

static void app_click(lv_event_t*e){
	struct fileopen_app*fa=e->user_data;
	if(!fa||!fa->fo||!fa->reg)return;
	if(fa->fo->selected)lv_obj_set_checked(fa->fo->selected->btn,false);
	lv_obj_set_checked(fa->btn,true);
	fa->fo->selected=fa;
	lv_obj_set_enabled(fa->fo->ok,true);
	lv_group_focus_obj(fa->fo->ok);
}

static void add_app(struct fileopen*fo,struct gui_register*reg){
	if(!reg||!reg->open_file)return;
	if(reg->open_regex&&!lv_obj_is_checked(fo->show_all)){
		Reprog*prog;
		bool match=false;
		for(size_t i=0;reg->open_regex[i];i++){
			if(!(prog=regexp_comp(reg->open_regex[i],REG_ICASE,NULL)))continue;
			if(regexp_exec(prog,fo->path,NULL,0)==0)match=true;
			regexp_free(prog);
		}
		if(!match)return;
	}
	static lv_coord_t grid_col[]={
		0,
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	struct fileopen_app*app;
	if(!(app=malloc(sizeof(struct fileopen_app))))return;
	memset(app,0,sizeof(struct fileopen_app));
	app->reg=reg,app->fo=fo;
	if(list_obj_add_new(&fo->apps,app)!=0)return;
	if(grid_col[0]==0)grid_col[0]=gui_font_size*1.5;

	// app item button
	app->btn=lv_btn_create(fo->view);
	lv_obj_set_width(app->btn,lv_pct(100));
	lv_obj_set_content_height(app->btn,grid_col[0]);
	lv_style_set_btn_item(app->btn);
	lv_obj_add_event_cb(app->btn,app_click,LV_EVENT_CLICKED,app);
	lv_obj_set_grid_dsc_array(app->btn,grid_col,grid_row);
	lv_group_add_obj(gui_grp,app->btn);

	// conf image
	app->w_img=lv_obj_create(app->btn);
	lv_obj_set_size(app->w_img,grid_col[0],grid_col[0]);
	lv_obj_clear_flag(app->w_img,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(app->w_img,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_border_width(app->w_img,0,0);
	lv_obj_set_style_bg_opa(app->w_img,LV_OPA_0,0);
	lv_obj_set_grid_cell(
		app->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	app->img=lv_img_create(app->w_img);
	lv_img_src_try(app->img,"app",reg->name,reg->icon);
	lv_img_set_size_mode(app->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_fill_image(app->img,grid_col[0],grid_col[0]);
	lv_obj_center(app->img);

	// app name
	app->lbl=lv_label_create(app->btn);
	lv_label_set_text(app->lbl,_(reg->title));
	lv_label_set_long_mode(app->lbl,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_grid_cell(
		app->lbl,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_CENTER,0,1
	);
}

static void redraw_apps(struct fileopen*fo){
	clean_apps(fo);
	list*l;
	if((l=guiact_get_registers()))do{
		add_app(fo,LIST_DATA(l,struct gui_register*));
	}while((l=l->next));
}

static void fileopen_click(lv_event_t*e){
	struct fileopen*fo=e->user_data;
	if(!fo)return;
	if(e->target==fo->cancel)guiact_do_back();
	else if(e->target==fo->ok&&fo->selected){
		char*d=strdup(fo->path);
		if(!d)return;
		guiact_start_activity(fo->selected->reg,d);
		guiact_do_back();
	}
}

static void show_all_click(lv_event_t*e){
	redraw_apps(e->user_data);
}

static int fileopen_draw(struct gui_activity*act){
	struct fileopen*fo=act->args;
	fo->act=act;
	fo->box=lv_draw_dialog_box(act->page,&fo->label,"Open file with");
	fo->view=lv_obj_create(fo->box);
	fo->show_all=lv_draw_checkbox(fo->box,"Show all apps",false,show_all_click,fo);
	lv_draw_btns_ok_cancel(fo->box,&fo->ok,&fo->cancel,fileopen_click,fo);
	lv_obj_set_flex_flow(fo->view,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(fo->view,1);
	lv_obj_set_width(fo->view,lv_pct(100));
	lv_obj_set_height(fo->box,lv_pct(80));
	return 0;
}

static int fileopen_clean(struct gui_activity*d){
	struct fileopen*box=d->args;
	if(!box)return 0;
	list_free_all_def(box->apps);
	free(box);
	d->args=NULL;
	return 0;
}

static int fileopen_get_focus(struct gui_activity*d){
	struct fileopen*fo=d->args;
	if(!fo)return 0;
	redraw_apps(fo);
	lv_group_add_obj(gui_grp,fo->show_all);
	lv_group_add_obj(gui_grp,fo->ok);
	lv_group_add_obj(gui_grp,fo->cancel);
	return 0;
}

static int fileopen_lost_focus(struct gui_activity*d){
	struct fileopen*box=d->args;
	if(!box)return 0;
	list*o=list_first(box->apps);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct fileopen_app*);
		if(item->img)lv_group_remove_obj(item->img);
	}while((o=o->next));
	lv_group_remove_obj(box->show_all);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

struct gui_register guireg_fileopen={
	.name="fileopen",
	.title="File Open",
	.show_app=false,
	.draw=fileopen_draw,
	.quiet_exit=fileopen_clean,
	.get_focus=fileopen_get_focus,
	.lost_focus=fileopen_lost_focus,
	.back=true,
	.mask=true,
};

void fileopen_open(const char*path){
	struct fileopen*fo;
	if(!(fo=malloc(sizeof(struct fileopen))))return;
	memset(fo,0,sizeof(struct fileopen));
	strcpy(fo->path,path);
	guiact_start_activity(&guireg_fileopen,fo);
}

void fileopen_open_url(url*uri){
	struct fileopen*fo;
	if(!(fo=malloc(sizeof(struct fileopen))))return;
	memset(fo,0,sizeof(struct fileopen));
	url_generate(fo->path,sizeof(fo->path),uri);
	guiact_start_activity(&guireg_fileopen,fo);
}

void fileopen_open_fsh(fsh*f){
	struct fileopen*fo;
	if(!(fo=malloc(sizeof(struct fileopen))))return;
	memset(fo,0,sizeof(struct fileopen));
	fs_get_path(f,fo->path,sizeof(fo->path));
	guiact_start_activity(&guireg_fileopen,fo);
}

#endif

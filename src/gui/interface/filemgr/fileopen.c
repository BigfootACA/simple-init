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
#include"str.h"
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"pathnames.h"
#include"gui/tools.h"
#include"gui/fileopen.h"
#include"gui/activity.h"

struct fileopen_app;
struct fileopen{
	char path[PATH_MAX];
	lv_obj_t*mask,*box;
	lv_obj_t*label,*view;
	lv_obj_t*ok,*cancel;
	lv_obj_t*last_btn;
	list*apps;
	void*user_data;
	struct gui_activity*act;
	struct fileopen_app*selected;
};

struct fileopen_app{
	struct fileopen*fo;
	struct gui_register*reg;
	lv_obj_t*btn,*img,*lbl;
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
	fo->last_btn=NULL;
	lv_obj_set_enabled(fo->ok,false);
}

static void img_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct fileopen_app*fa=lv_obj_get_user_data(obj);
	if(!fa||!fa->fo||!fa->reg||fa->img!=obj)return;
	if(fa->fo->selected)lv_obj_set_checked(fa->fo->selected->btn,false);
	lv_obj_set_checked(fa->btn,true);
	fa->fo->selected=fa;
	lv_obj_set_enabled(fa->fo->ok,true);
}

static void add_app(struct fileopen*fo,struct gui_register*reg){
	if(!reg||!reg->open_file)return;
	struct fileopen_app*app=malloc(sizeof(struct fileopen_app));
	if(!app)return;
	memset(app,0,sizeof(struct fileopen_app));
	app->reg=reg,app->fo=fo;
	if(list_obj_add_new(&fo->apps,app)!=0)return;

	// app item button
	app->btn=lv_btn_create(fo->view,NULL);
	lv_obj_set_size(app->btn,lv_page_get_scrl_width(fo->view),gui_dpi/3);
	lv_style_set_btn_item(app->btn);
	lv_obj_set_click(app->btn,false);
	lv_obj_align(
		app->btn,fo->last_btn,fo->last_btn?
			 LV_ALIGN_OUT_BOTTOM_LEFT:
			 LV_ALIGN_IN_TOP_MID,
		0,gui_font_size/8+(fo->last_btn?gui_dpi/20:0)
	);
	fo->last_btn=app->btn;

	// line for button text
	lv_obj_t*line=lv_line_create(app->btn,NULL);
	lv_obj_set_width(line,lv_obj_get_width(app->btn));

	// conf image
	lv_coord_t si=lv_obj_get_height(app->btn)-gui_font_size;
	app->img=lv_img_create(line,NULL);
	lv_obj_set_size(app->img,si,si);
	lv_obj_align(app->img,app->btn,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
	lv_obj_set_click(app->img,true);
	lv_obj_set_user_data(app->img,app);
	lv_obj_set_event_cb(app->img,img_click);
	lv_obj_set_style_local_outline_width(app->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/100);
	lv_obj_set_style_local_outline_color(app->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_obj_set_style_local_radius(app->img,LV_IMG_PART_MAIN,LV_STATE_FOCUSED,gui_dpi/50);
	lv_img_ext_t*ext=lv_obj_get_ext_attr(app->img);
	char path[BUFSIZ]={0},fail[BUFSIZ]={0};
	strcpy(fail,IMG_RES"/apps.svg");
	if(reg->icon[0]){
		if(contains_of("./",2,reg->icon[0]))strncpy(path,reg->icon,BUFSIZ-1);
		else snprintf(path,BUFSIZ-1,IMG_RES"/%s",reg->icon);
	}
	lv_img_set_src(app->img,reg->icon[0]?path:fail);
	if(ext->w>0&&ext->h>0)lv_img_set_zoom(app->img,(int)(((float)si/MAX(ext->w,ext->h))*256));
	lv_img_set_pivot(app->img,0,0);
	lv_group_add_obj(gui_grp,app->img);

	// app name
	app->lbl=lv_label_create(line,NULL);
	lv_label_set_text(app->lbl,_(reg->title));
	lv_obj_align(
		app->lbl,NULL,
		LV_ALIGN_IN_LEFT_MID,
		gui_font_size+si,0
	);
	lv_label_set_long_mode(app->lbl,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_width(app->lbl,lv_obj_get_width(app->btn)-si-gui_font_size*2);
}

static void redraw_apps(struct fileopen*fo){
	clean_apps(fo);
	for(int i=0;guiact_register[i];i++)
		add_app(fo,guiact_register[i]);
}

static void fileopen_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct fileopen*fo=lv_obj_get_user_data(obj);
	if(!fo)return;
	if(obj==fo->cancel)guiact_do_back();
	else if(obj==fo->ok&&fo->selected){
		char*d=strdup(fo->path);
		if(!d)return;
		guiact_start_activity(fo->selected->reg,d);
		guiact_do_back();
	}
}

static int fileopen_draw(struct gui_activity*act){
	lv_coord_t box_h=gui_dpi/8;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3;
	lv_coord_t max_h=gui_dpi*6,cur_h=gui_sh/5*2;
	struct fileopen*fo=act->args;
	fo->act=act;

	fo->mask=lv_create_opa_mask(act->page);
	fo->box=lv_obj_create(fo->mask,NULL);
	lv_obj_set_style_local_border_width(fo->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(fo->box,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_border_width(fo->box,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_width(fo->box,MIN(max_w,cur_w));

	lv_coord_t lh=gui_sh/8;
	fo->label=lv_label_create(fo->box,NULL);
	lv_label_set_long_mode(fo->label,LV_LABEL_LONG_BREAK);
	lv_label_set_align(fo->label,LV_LABEL_ALIGN_CENTER);
	lv_obj_set_width(fo->label,lv_obj_get_width(fo->box));
	lv_label_set_text(fo->label,_("Open file with"));
	lv_obj_set_pos(fo->label,0,box_h);
	if(lv_obj_get_height(fo->label)>lh){
		lv_label_set_long_mode(fo->label,LV_LABEL_LONG_DOT);
		lv_obj_set_height(fo->label,lh);
	}
	box_h+=lv_obj_get_height(fo->label);

	box_h+=gui_dpi/12;
	fo->view=lv_page_create(fo->box,NULL);
	lv_color_t c=lv_obj_get_style_border_color(fo->view,LV_PAGE_PART_BG);
	lv_obj_set_style_local_border_color(fo->view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,c);
	lv_obj_set_style_local_border_color(fo->view,LV_PAGE_PART_BG,LV_STATE_PRESSED,c);
	lv_obj_set_style_local_border_color(fo->view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,c);
	lv_obj_set_size(fo->view,lv_obj_get_width(fo->box)-gui_font_size,MIN(max_h,cur_h));
	lv_obj_set_pos(fo->view,gui_font_size/2,box_h);
	box_h+=lv_obj_get_height(fo->view);

	lv_coord_t
		bm=gui_font_size/2,
		bw=lv_obj_get_width(fo->box)/2,
		bh=gui_font_size+(gui_dpi/8);

	box_h+=bm;
	fo->ok=lv_btn_create(fo->box,NULL);
	lv_obj_set_enabled(fo->ok,false);
	lv_label_set_text(lv_label_create(fo->ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_style_local_margin_bottom(fo->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fo->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fo->ok,bw-bm,bh);
	lv_obj_set_user_data(fo->ok,fo);
	lv_obj_set_event_cb(fo->ok,fileopen_click);
	lv_obj_set_pos(fo->ok,bm/2,box_h);

	fo->cancel=lv_btn_create(fo->box,NULL);
	lv_label_set_text(lv_label_create(fo->cancel,NULL),LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_margin_bottom(fo->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fo->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fo->cancel,bw-bm,bh);
	lv_obj_set_user_data(fo->cancel,fo);
	lv_obj_set_event_cb(fo->cancel,fileopen_click);
	lv_obj_set_pos(fo->cancel,bm/2+bw,box_h);
	box_h+=bh;

	box_h+=bm;
	lv_obj_set_height(fo->box,box_h);
	lv_obj_align(fo->box,NULL,LV_ALIGN_CENTER,0,0);

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

static void fileopen_cb(lv_task_t*t){
	guiact_start_activity(&guireg_fileopen,t->user_data);
}

void fileopen_open(const char*path){
	struct fileopen*fo=malloc(sizeof(struct fileopen));
	if(!fo)return;
	memset(fo,0,sizeof(struct fileopen));
	strcpy(fo->path,path);
	lv_task_once(lv_task_create(fileopen_cb,0,LV_TASK_PRIO_LOWEST,fo));
}

#endif

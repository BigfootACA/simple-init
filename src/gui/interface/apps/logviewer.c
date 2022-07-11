/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"list.h"
#include"logger.h"
#include"defines.h"
#include<sys/stat.h>
#include"gui/sysbar.h"
#include"gui/activity.h"
#define TAG "logviewer"

struct log_viewer{
	lv_obj_t*txt,*view,*content,*btns;
	lv_obj_t*btn_ctrl,*btn_reload;
};

static void load_log_task(lv_timer_t*t __attribute__((unused))){
	struct log_viewer*v=t->user_data;
	#ifdef ENABLE_UEFI
	list*l=list_first(logbuffer);
	if(l){
		lv_textarea_set_text(v->content,"");
		do{
			bool changed=false;
			LIST_DATA_DECLARE(b,l,struct log_buff*);
			if(!b)continue;
			if(b->tag&&b->tag[0]){
				lv_textarea_add_text(v->content,b->tag);
				changed=true;
			}
			if(b->content&&b->content[0]){
				if(changed)lv_textarea_add_text(v->content,": ");
				lv_textarea_add_text(v->content,b->content);
				changed=true;
			}
			if(changed)lv_textarea_add_text(v->content,"\n");
		}while((l=l->next));
	}else lv_label_set_text(v->content,_("No buffer found"));
	#else
	char*buff=NULL;
	struct stat st;
	int fd=open(_PATH_DEV"/logger.log",O_RDONLY);
	if(fd<0){
		telog_warn("open logger.log failed");
		goto fail;
	}
	if(fstat(fd,&st)!=0)goto fail;
	if(!(buff=malloc(st.st_size+2)))goto fail;
	ssize_t r=read(fd,buff,st.st_size);
	if(r<=0)goto fail;
	buff[r++]='\n',buff[r]=0;
	lv_label_set_text(v->content,buff);
	free(buff);
	close(fd);
	return;
	fail:
	lv_label_set_text_fmt(
		v->content,
		_("Load log failed: %s"),
		_(strerror(errno))
	);
	if(buff)free(buff);
	close(fd);
	#endif
}

static void load_log(struct log_viewer*v){
	lv_label_set_text(v->content,_("Loading..."));
	lv_timer_set_repeat_count(lv_timer_create(load_log_task,20,v),1);
}

static void reload_click(lv_event_t*e){
	load_log(lv_event_get_user_data(e));
}

static void ctrl_click(lv_event_t*e __attribute__((unused))){
	ctrl_pad_show();
}

static int logviewer_get_focus(struct gui_activity*d){
	struct log_viewer*v=d->data;
	if(!v)return 0;
	lv_group_add_obj(gui_grp,v->btn_ctrl);
	lv_group_add_obj(gui_grp,v->btn_reload);
	ctrl_pad_set_target(v->view);
	return 0;
}

static int logviewer_lost_focus(struct gui_activity*d){
	struct log_viewer*v=d->data;
	if(!v)return 0;
	lv_group_remove_obj(v->btn_ctrl);
	lv_group_remove_obj(v->btn_reload);
	ctrl_pad_set_target(NULL);
	return 0;
}

static int do_clean(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int logviewer_init(struct gui_activity*act){
	act->data=malloc(sizeof(struct log_viewer));
	if(!act->data)return -1;
	memset(act->data,0,sizeof(struct log_viewer));
	return 0;
}

static int logviewer_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	struct log_viewer*v=act->data;
	if(!v)return -1;
	lv_obj_set_style_pad_all(act->page,gui_font_size/2,0);
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	v->txt=lv_label_create(act->page);
	lv_label_set_text(v->txt,_("Loggerd Viewer"));
	lv_obj_set_size(v->txt,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(v->txt,gui_font_size/2,0);
	lv_obj_set_style_text_align(v->txt,LV_TEXT_ALIGN_CENTER,0);

	v->view=lv_obj_create(act->page);
	lv_obj_set_width(v->view,lv_pct(100));
	lv_obj_set_flex_grow(v->view,1);
	v->content=lv_label_create(v->view);
	lv_obj_set_style_text_font(v->content,gui_font_small,0);
	lv_obj_set_size(v->content,LV_SIZE_CONTENT,LV_SIZE_CONTENT);

	v->btns=lv_obj_create(act->page);
	lv_obj_set_style_radius(v->btns,0,0);
	lv_obj_set_style_pad_all(v->btns,gui_dpi/50,0);
	lv_obj_set_scroll_dir(v->btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(v->btns,0,0);
	lv_obj_set_style_bg_opa(v->btns,LV_OPA_0,0);
	lv_obj_clear_flag(v->btns,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(v->btns,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_grid_dsc_array(v->btns,grid_col,grid_row);

	v->btn_ctrl=lv_btn_create(v->btns);
	lv_obj_add_event_cb(
		v->btn_ctrl,ctrl_click,
		LV_EVENT_CLICKED,NULL
	);
	lv_obj_set_style_radius(v->btn_ctrl,gui_font_size/2,0);
	lv_obj_t*txt_ctrl=lv_label_create(v->btn_ctrl);
	lv_label_set_text(txt_ctrl,_("Control"));
	lv_obj_center(txt_ctrl);
	lv_obj_set_grid_cell(
		v->btn_ctrl,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);

	v->btn_reload=lv_btn_create(v->btns);
	lv_obj_add_event_cb(
		v->btn_reload,reload_click,
		LV_EVENT_CLICKED,v
	);
	lv_obj_set_style_radius(v->btn_reload,gui_font_size/2,0);
	lv_obj_t*txt_reload=lv_label_create(v->btn_reload);
	lv_label_set_text(txt_reload,_("Reload"));
	lv_obj_center(txt_reload);
	lv_obj_set_grid_cell(
		v->btn_reload,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_STRETCH,0,1
	);
	return 0;
}

static int do_load(struct gui_activity*act){
	load_log(act->data);
	return 0;
}

struct gui_register guireg_logviewer={
	.name="logger-viewer",
	.title="Loggerd Viewer",
	.icon="logviewer.svg",
	.show_app=true,
	.quiet_exit=do_clean,
	.init=logviewer_init,
	.draw=logviewer_draw,
	.lost_focus=logviewer_lost_focus,
	.get_focus=logviewer_get_focus,
	.data_load=do_load,
	.back=true,
};
#endif

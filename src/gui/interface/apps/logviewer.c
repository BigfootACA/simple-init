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
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#define TAG "logviewer"

static lv_obj_t*view;
static lv_obj_t*btn_ctrl,*btn_reload;

static void load_log_task(lv_task_t*t __attribute__((unused))){
	if(!view)return;
	#ifdef ENABLE_UEFI
	list*l=list_first(logbuffer);
	if(l){
		lv_textarea_set_text(view,"");
		do{
			bool changed=false;
			LIST_DATA_DECLARE(b,l,struct log_buff*);
			if(!b)continue;
			if(b->tag&&b->tag[0]){
				lv_textarea_add_text(view,b->tag);
				changed=true;
			}
			if(b->content&&b->content[0]){
				if(changed)lv_textarea_add_text(view,": ");
				lv_textarea_add_text(view,b->content);
				changed=true;
			}
			if(changed)lv_textarea_add_text(view,"\n");
		}while((l=l->next));
	}else lv_textarea_set_text(view,_("No buffer found"));
	#else
	char buff[BUFSIZ];
	memset(buff,0,BUFSIZ);
	int fd=open(_PATH_DEV"/logger.log",O_RDONLY);
	if(fd<0){
		telog_warn("open logger.log failed");
		snprintf(buff,BUFSIZ-1,_("Load log failed: %m"));
		lv_textarea_set_text(view,buff);
	}else{
		lv_textarea_set_text(view,"");
		while(read(fd,buff,BUFSIZ-1)>0){
			lv_textarea_add_text(view,buff);
			memset(buff,0,BUFSIZ);
			lv_task_handler();
		}
		lv_textarea_add_text(view,"\n");
		close(fd);
		lv_textarea_set_cursor_pos(view,0);
	}
	#endif
}

static void load_log(){
	lv_textarea_set_text(view,_("Loading..."));
	lv_task_t*t=lv_task_create(load_log_task,20,LV_TASK_PRIO_LOWEST,NULL);
	lv_task_once(t);
}

static void reload_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_reload||e!=LV_EVENT_CLICKED)return;
	load_log();
}

static void ctrl_click(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_ctrl||e!=LV_EVENT_CLICKED)return;
	ctrl_pad_show();
}

static int logviewer_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,btn_ctrl);
	lv_group_add_obj(gui_grp,btn_reload);
	ctrl_pad_set_target(view);
	return 0;
}

static int logviewer_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(btn_ctrl);
	lv_group_remove_obj(btn_reload);
	ctrl_pad_set_target(NULL);
	return 0;
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	view=NULL;
	return 0;
}

static int logviewer_draw(struct gui_activity*act){

	lv_obj_t*txt=lv_label_create(act->page,NULL);
	lv_label_set_text(txt,_("Loggerd Viewer"));
	lv_obj_set_width(txt,gui_sw);
	lv_obj_align(txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);

	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_text_font(&style,LV_STATE_DEFAULT,gui_font_small);

	int bth=gui_font_size+(gui_dpi/8),btw=gui_sw/2-(gui_dpi/6);

	view=lv_textarea_create(act->page,NULL);
	lv_obj_set_size(view,gui_sw-(gui_font_size*2),gui_sh-lv_obj_get_height(txt)-gui_font_size*4-bth);
	lv_obj_align(view,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_add_style(view,LV_TEXTAREA_PART_BG,&style);
	lv_textarea_set_cursor_hidden(view,true);
	lv_textarea_set_one_line(view,false);
	lv_obj_set_click(view,false);
	lv_page_set_scrollable_fit2(view,LV_FIT_MAX,LV_FIT_MAX);
	lv_textarea_set_scrollbar_mode(view,LV_SCROLLBAR_MODE_DRAG);
	lv_textarea_ext_t*e=lv_obj_get_ext_attr(view);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_EXPAND);

	btn_ctrl=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_ctrl,btw,bth);
	lv_obj_set_event_cb(btn_ctrl,ctrl_click);
	lv_obj_align(btn_ctrl,view,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_style_set_action_button(btn_ctrl,true);
	lv_label_set_text(lv_label_create(btn_ctrl,NULL),_("Control"));
	lv_group_add_obj(gui_grp,btn_ctrl);

	btn_reload=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_reload,btw,bth);
	lv_obj_set_event_cb(btn_reload,reload_click);
	lv_obj_align(btn_reload,view,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
	lv_style_set_action_button(btn_reload,true);
	lv_label_set_text(lv_label_create(btn_reload,NULL),_("Reload"));
	lv_group_add_obj(gui_grp,btn_reload);
	return 0;
}

static int do_load(struct gui_activity*act __attribute__((unused))){
	load_log();
	return 0;
}

struct gui_register guireg_logviewer={
	.name="logger-viewer",
	.title="Loggerd Viewer",
	.icon="logviewer.svg",
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=logviewer_draw,
	.lost_focus=logviewer_lost_focus,
	.get_focus=logviewer_get_focus,
	.data_load=do_load,
	.back=true,
};
#endif

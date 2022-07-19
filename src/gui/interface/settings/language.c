/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"logger.h"
#include"defines.h"
#include"language.h"
#include"init_internal.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "language"

struct language_menu{
	struct gui_activity*act;
	lv_obj_t*box,*txt,*sel;
	lv_obj_t*arr_left,*btn_ok,*arr_right;
};

static void ok_action(lv_event_t*e){
	struct language_menu*m=e->user_data;
	uint16_t i=lv_dropdown_get_selected(m->sel);
	if(!languages[i].lang)return;
	const char*lang=lang_concat(&languages[i],true,true);
	tlog_debug("set language to %s",lang);
	lang_set(lang);
	m->act->data_changed=true;
	#ifndef ENABLE_UEFI
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strcpy(msg.data.data,lang);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		msgbox_alert("init control command failed: %s",strerror(ex));
		return;
	}
	#endif
	guiact_do_back();
}

static void arrow_action(lv_event_t*e){
	struct language_menu*m=e->user_data;
	uint16_t cnt=lv_dropdown_get_option_cnt(m->sel);
	int16_t cur=lv_dropdown_get_selected(m->sel);
	if(e->target==m->arr_left)cur--;
	else if(e->target==m->arr_right)cur++;
	else return;
	if(cur<=0)cur=0;
	if(cur>=cnt-1)cur=cnt-1;
	lv_dropdown_set_selected(m->sel,(uint16_t)cur);
}

static void init_languages(struct language_menu*m){
	lv_dropdown_clear_options(m->sel);
	char*lang=lang_get_locale(NULL);
	uint16_t s=0;
	for(size_t i=0;languages[i].name;i++){
		struct language*l=&languages[i];
		lv_dropdown_add_option(m->sel,l->name,i);
		if(lang_compare(l,lang))s=i;
	}
	lv_dropdown_set_selected(m->sel,s);
	if(lang)free(lang);
}

static int language_get_focus(struct gui_activity*d){
	struct language_menu*m=d->data;
	if(!m)return 0;
	lv_group_add_obj(gui_grp,m->sel);
	lv_group_add_obj(gui_grp,m->arr_left);
	lv_group_add_obj(gui_grp,m->btn_ok);
	lv_group_add_obj(gui_grp,m->arr_right);
	lv_group_focus_obj(m->sel);
	return 0;
}

static int language_lost_focus(struct gui_activity*d){
	struct language_menu*m=d->data;
	if(!m)return 0;
	lv_group_remove_obj(m->sel);
	lv_group_remove_obj(m->arr_left);
	lv_group_remove_obj(m->btn_ok);
	lv_group_remove_obj(m->arr_right);
	return 0;
}

static int language_init(struct gui_activity*act){
	struct language_menu*m=malloc(sizeof(struct language_menu));
	if(!m)ERET(ENOMEM);
	memset(m,0,sizeof(struct language_menu));
	act->data=m,m->act=act;
	return 0;
}

static int language_clean(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int language_menu_draw(struct gui_activity*act){
	struct language_menu*m=act->data;
	if(!m)return 0;

	m->box=lv_obj_create(act->page);
	lv_obj_set_style_pad_all(m->box,gui_font_size,0);
	lv_obj_set_scroll_dir(m->box,LV_DIR_NONE);
	lv_obj_set_style_min_width(m->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(m->box,gui_dpi,0);
	lv_obj_set_style_max_width(m->box,lv_pct(80),0);
	lv_obj_set_style_max_height(m->box,lv_pct(80),0);
	lv_obj_set_height(m->box,LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(m->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(m->box,gui_font_size/2,0);
	lv_obj_center(m->box);

	m->txt=lv_label_create(m->box);
	lv_label_set_long_mode(m->txt,LV_LABEL_LONG_CLIP);
	lv_obj_set_style_text_align(m->txt,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(m->txt,_("Select language"));
	lv_obj_set_width(m->txt,lv_pct(100));

	m->sel=lv_dropdown_create(m->box);
	lv_obj_add_event_cb(m->sel,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_width(m->sel,lv_pct(100));
	init_languages(m);

	lv_obj_t*btns=lv_obj_create(m->box);
	lv_obj_set_size(btns,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_clear_flag(btns,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_radius(btns,0,0);
	lv_obj_set_scroll_dir(btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(btns,0,0);
	lv_obj_set_style_bg_opa(btns,LV_OPA_0,0);
	lv_obj_set_style_pad_all(btns,gui_dpi/50,0);
	lv_obj_set_flex_flow(btns,LV_FLEX_FLOW_ROW);

	m->arr_left=lv_btn_create(btns);
	lv_obj_add_event_cb(m->arr_left,arrow_action,LV_EVENT_CLICKED,m);
	lv_obj_set_enabled(m->arr_left,true);
	lv_obj_set_flex_grow(m->arr_left,1);
	lv_obj_t*txt_arr_left=lv_label_create(m->arr_left);
	lv_label_set_text(txt_arr_left,LV_SYMBOL_LEFT);
	lv_obj_center(txt_arr_left);

	m->btn_ok=lv_btn_create(btns);
	lv_obj_add_event_cb(m->btn_ok,ok_action,LV_EVENT_CLICKED,m);
	lv_obj_set_enabled(m->btn_ok,true);
	lv_obj_set_flex_grow(m->btn_ok,3);
	lv_obj_t*txt_ok=lv_label_create(m->btn_ok);
	lv_label_set_text(txt_ok,_("OK"));
	lv_obj_center(txt_ok);

	m->arr_right=lv_btn_create(btns);
	lv_obj_add_event_cb(m->arr_right,arrow_action,LV_EVENT_CLICKED,m);
	lv_obj_set_enabled(m->arr_right,true);
	lv_obj_set_flex_grow(m->arr_right,1);
	lv_obj_t*txt_arr_right=lv_label_create(m->arr_right);
	lv_label_set_text(txt_arr_right,LV_SYMBOL_RIGHT);
	lv_obj_center(txt_arr_right);

	return 0;
}

struct gui_register guireg_language={
	.name="language-menu",
	.title="Language",
	.icon="language.svg",
	.show_app=true,
	.init=language_init,
	.quiet_exit=language_clean,
	.get_focus=language_get_focus,
	.lost_focus=language_lost_focus,
	.draw=language_menu_draw,
	.back=true,
	.mask=true,
};
#endif

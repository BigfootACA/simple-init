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
	lv_obj_t*box,*txt,*sel;
	lv_obj_t*arr_left,*btn_ok,*arr_right;
};

static void ok_action(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE)return;
	struct language_menu*m=lv_obj_get_user_data(obj);
	if(!m||obj!=m->btn_ok||e!=LV_EVENT_CLICKED)return;
	struct gui_activity*act=lv_obj_get_user_data(obj);
	uint16_t i=lv_dropdown_get_selected(m->sel);
	if(!languages[i].lang)return;
	const char*lang=lang_concat(&languages[i],true,true);
	tlog_debug("set language to %s",lang);
	lang_set(lang);
	act->data_changed=true;
	#ifndef ENABLE_UEFI
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strcpy(msg.data.data,lang);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		guiact_do_back();
		msgbox_alert("init control command failed: %s",strerror(ex));
		return;
	}
	#endif
	guiact_do_back();
}

static void arrow_action(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE)return;
	struct language_menu*m=lv_obj_get_user_data(obj);
	if(!m||e!=LV_EVENT_CLICKED)return;
	uint16_t cnt=lv_dropdown_get_option_cnt(m->sel);
	int16_t cur=lv_dropdown_get_selected(m->sel);
	if(obj==m->arr_left)cur--;
	else if(obj==m->arr_right)cur++;
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

static int language_resize(struct gui_activity*d){
	int bts=gui_font_size+(gui_dpi/8);
	struct language_menu*m=d->data;
	if(!m)return 0;
	lv_obj_set_width(m->box,d->w/6*5);
	lv_obj_set_width(m->txt,lv_page_get_scrl_width(m->box));
	lv_obj_set_width(m->sel,lv_page_get_scrl_width(m->box)-gui_dpi/10);
	lv_obj_align(m->sel,m->txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	lv_obj_set_size(m->btn_ok,lv_page_get_scrl_width(m->box)-gui_dpi/10-bts*3,bts);
	lv_obj_align(m->btn_ok,m->sel,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	lv_obj_set_size(m->arr_left,bts,bts);
	lv_obj_align(m->arr_left,m->btn_ok,LV_ALIGN_OUT_LEFT_MID,-bts/2,0);
	lv_obj_set_size(m->arr_right,bts,bts);
	lv_obj_align(m->arr_right,m->btn_ok,LV_ALIGN_OUT_RIGHT_MID,bts/2,0);
	lv_obj_set_height(m->box,lv_obj_get_y(m->btn_ok)+lv_obj_get_height(m->btn_ok)+(gui_font_size*2)+gui_dpi/20);
	lv_obj_align(m->box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int language_init(struct gui_activity*act){
	struct language_menu*m=malloc(sizeof(struct language_menu));
	if(!m)ERET(ENOMEM);
	memset(m,0,sizeof(struct language_menu));
	act->data=m;
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

	m->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(m->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_click(m->box,false);

	m->txt=lv_label_create(m->box,NULL);
	lv_label_set_long_mode(m->txt,LV_LABEL_LONG_BREAK);
	lv_label_set_align(m->txt,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(m->txt,_("Select language"));

	m->sel=lv_dropdown_create(m->box,NULL);
	lv_obj_set_event_cb(m->sel,lv_default_dropdown_cb);
	init_languages(m);

	m->btn_ok=lv_btn_create(m->box,NULL);
	lv_style_set_action_button(m->btn_ok,true);
	lv_obj_set_event_cb(m->btn_ok,ok_action);
	lv_obj_set_user_data(m->btn_ok,act);
	lv_label_set_text(lv_label_create(m->btn_ok,NULL),_("OK"));

	m->arr_left=lv_btn_create(m->box,NULL);
	lv_obj_set_event_cb(m->arr_left,arrow_action);
	lv_style_set_action_button(m->arr_left,true);
	lv_label_set_text(lv_label_create(m->arr_left,NULL),LV_SYMBOL_LEFT);

	m->arr_right=lv_btn_create(m->box,NULL);
	lv_obj_set_event_cb(m->arr_right,arrow_action);
	lv_style_set_action_button(m->arr_right,true);
	lv_label_set_text(lv_label_create(m->arr_right,NULL),LV_SYMBOL_RIGHT);

	return 0;
}

struct gui_register guireg_language={
	.name="language-menu",
	.title="Language",
	.icon="language.svg",
	.show_app=true,
	.init=language_init,
	.quiet_exit=language_clean,
	.resize=language_resize,
	.get_focus=language_get_focus,
	.lost_focus=language_lost_focus,
	.draw=language_menu_draw,
	.back=true,
	.mask=true,
};
#endif

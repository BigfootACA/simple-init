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
	lv_obj_t*btn_ok,*btn_cancel;
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
	lv_group_add_obj(gui_grp,m->btn_ok);
	lv_group_add_obj(gui_grp,m->btn_cancel);
	lv_group_focus_obj(m->sel);
	return 0;
}

static int language_lost_focus(struct gui_activity*d){
	struct language_menu*m=d->data;
	if(!m)return 0;
	lv_group_remove_obj(m->sel);
	lv_group_remove_obj(m->btn_ok);
	lv_group_remove_obj(m->btn_cancel);
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
	m->box=lv_draw_dialog_box(act->page,&m->txt,"Select language");
	m->sel=lv_dropdown_create(m->box);
	lv_obj_add_event_cb(m->sel,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_width(m->sel,lv_pct(100));
	init_languages(m);
	lv_draw_btns_ok_cancel(m->box,&m->btn_ok,&m->btn_cancel,ok_action,m);
	return 0;
}

struct gui_register guireg_language={
	.name="language-menu",
	.title="Language",
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

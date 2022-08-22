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
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/inputbox.h"
#include"gui/activity.h"

struct inputbox{
	char text[BUFSIZ];
	char holder[BUFSIZ];
	char content[BUFSIZ];
	const char*accepts;
	inputbox_callback callback;
	lv_obj_t*box,*btns;
	lv_obj_t*label,*input;
	lv_obj_t*ok,*cancel;
	lv_event_cb_t input_cb;
	uint32_t max;
	lv_text_align_t align;
	bool one_line,pwd,sel;
	void*user_data;
	struct gui_activity*act;
};

static void input_click(lv_event_t*e){
	struct inputbox*box=e->user_data;
	if(guiact_get_last()->args!=box)return;
	sysbar_keyboard_close();
	sysbar_focus_input(NULL);
	const char*cont=lv_textarea_get_text(box->input);
	if(box->callback&&box->callback(
		e->target==box->ok,
		cont,box->user_data
	))return;
	box->act->args=NULL;
	free(box);
	guiact_do_back();
}

static void text_cb(lv_event_t*e){
	if(e->code==LV_EVENT_DELETE)return;
	struct inputbox*box=e->user_data;
	if(guiact_get_last()->args!=box)return;
	if(box->input_cb)box->input_cb(e);
}

static int inputbox_draw(struct gui_activity*act){
	struct inputbox*box=act->args;
	box->act=act;
	box->box=lv_draw_dialog_box(act->page,NULL,NULL);

	box->label=lv_label_create(box->box);
	lv_obj_set_style_text_align(box->label,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(box->label,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(box->label,lv_pct(100));
	lv_label_set_text(box->label,box->text);

	box->input=lv_textarea_create(box->box);
	lv_textarea_set_text(box->input,box->content);
	lv_textarea_set_password_mode(box->input,box->pwd);
	lv_textarea_set_text_selection(box->input,box->sel);
	lv_textarea_set_one_line(box->input,box->one_line);
	if(box->align!=LV_TEXT_ALIGN_AUTO)lv_obj_set_style_text_align(box->input,box->align,0);
	if(box->max>0)lv_textarea_set_max_length(box->input,box->max);
	if(box->holder[0])lv_textarea_set_placeholder_text(box->input,box->holder);
	if(box->accepts)lv_textarea_set_accepted_chars(box->input,box->accepts);
	lv_obj_set_size(box->input,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(box->input,lv_pct(60),0);
	lv_obj_add_event_cb(box->input,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_add_event_cb(box->input,text_cb,LV_EVENT_ALL,box);

	box->btns=lv_draw_btns_ok_cancel(box->box,&box->ok,&box->cancel,input_click,box);
	return 0;
}

static int inputbox_clean(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	sysbar_focus_input(NULL);
	free(box);
	d->args=NULL;
	return 0;
}

static int inputbox_get_focus(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	lv_group_add_obj(gui_grp,box->input);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int inputbox_lost_focus(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	lv_group_remove_obj(box->input);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

struct gui_register guireg_inputbox={
	.name="inputbox",
	.title="Input Box",
	.show_app=false,
	.draw=inputbox_draw,
	.quiet_exit=inputbox_clean,
	.get_focus=inputbox_get_focus,
	.lost_focus=inputbox_lost_focus,
	.back=true,
	.mask=true,
	.allow_exclusive=true,
};

struct inputbox*inputbox_create(inputbox_callback callback,const char*title,...){
	struct inputbox*input=malloc(sizeof(struct inputbox));
	if(!input)return NULL;
	memset(input,0,sizeof(struct inputbox));
	if(title){
		va_list va;
		va_start(va,title);
		vsnprintf(input->text,sizeof(input->text)-1,_(title),va);
		va_end(va);
	}
	input->callback=callback;
	input->one_line=true;
	input->align=LV_TEXT_ALIGN_AUTO;
	guiact_start_activity(&guireg_inputbox,input);
	return input;
}

void inputbox_set_one_line(struct inputbox*input,bool one_line){
	if(!input)return;
	input->one_line=one_line;
}

void inputbox_set_text_select(struct inputbox*input,bool sel){
	if(!input)return;
	input->sel=sel;
}

void inputbox_set_pwd_mode(struct inputbox*input,bool pwd){
	if(!input)return;
	input->pwd=pwd;
}

void inputbox_set_input_align(struct inputbox*input,lv_text_align_t align){
	if(!input)return;
	input->align=align;
}

void inputbox_set_max_length(struct inputbox*input,uint32_t max){
	if(!input)return;
	input->max=max;
}

void inputbox_set_input_event_cb(struct inputbox*input,lv_event_cb_t cb){
	if(!input)return;
	input->input_cb=cb;
}

void inputbox_set_callback(struct inputbox*input,inputbox_callback cb){
	if(!input)return;
	input->callback=cb;
}

void inputbox_set_accept(struct inputbox*input,const char*accept){
	if(!input||!accept)return;
	input->accepts=accept;
}

void inputbox_set_title(struct inputbox*input,const char*title,...){
	if(!input)return;
	memset(input->text,0,sizeof(input->text));
	if(!title)return;
	va_list va;
	va_start(va,title);
	vsnprintf(input->text,sizeof(input->text)-1,_(title),va);
	va_end(va);
}

void inputbox_set_content(struct inputbox*input,const char*content,...){
	if(!input)return;
	memset(input->content,0,sizeof(input->content));
	if(!content)return;
	va_list va;
	va_start(va,content);
	vsnprintf(input->content,sizeof(input->content)-1,content,va);
	va_end(va);
}

void inputbox_set_holder(struct inputbox*input,const char*holder,...){
	if(!input)return;
	memset(input->holder,0,sizeof(input->holder));
	if(!holder)return;
	va_list va;
	va_start(va,holder);
	vsnprintf(input->holder,sizeof(input->holder)-1,_(holder),va);
	va_end(va);
}

void inputbox_set_user_data(struct inputbox*input,void*user_data){
	if(!input)return;
	input->user_data=user_data;
}

#endif

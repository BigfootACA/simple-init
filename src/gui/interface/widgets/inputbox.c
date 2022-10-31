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
#include"gui/sysbar.h"
#include"gui/inputbox.h"
#include"gui/activity.h"

struct inputbox{
	char text[BUFSIZ];
	char holder[BUFSIZ];
	char content[BUFSIZ];
	const char*accepts;
	inputbox_callback callback;
	lv_obj_t*box,*page;
	lv_obj_t*label,*input;
	lv_obj_t*ok,*cancel;
	lv_event_cb_t input_cb;
	uint32_t max;
	lv_label_align_t align;
	bool one_line,pwd,sel;
	void*user_data;
	struct gui_activity*act;
};

static void input_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct inputbox*box;
	if(!(box=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=box)return;
	sysbar_keyboard_close();
	sysbar_focus_input(NULL);
	const char*cont=lv_textarea_get_text(box->input);
	if(box->callback&&box->callback(obj==box->ok,cont,box->user_data))return;
	lv_obj_set_user_data(box->ok,NULL);
	lv_obj_set_user_data(box->cancel,NULL);
	box->act->args=NULL;
	free(box);
	guiact_do_back();
}

static void text_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj)return;
	struct inputbox*box;
	if(!(box=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=box)return;
	if(e==LV_EVENT_CLICKED){
		sysbar_focus_input(obj);
		sysbar_keyboard_open();
	}
	if(box->input_cb)box->input_cb(obj,e);
}

static int inputbox_resize(struct gui_activity*act){
	lv_coord_t box_h=0;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3,xw=MIN(max_w,cur_w);
	lv_coord_t max_h=gui_dpi*6,cur_h=gui_sh/3*2,xh=MIN(max_h,cur_h);
	struct inputbox*box=act->args;
	static bool style_init=false;
	static lv_style_t style;
	if(!style_init)lv_style_init(&style);
	lv_obj_set_style_local_border_width(box->box,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_width(box->page,xw);
	xw=lv_page_get_scrl_width(box->page);
	lv_obj_set_width(box->box,xw);

	lv_coord_t
		btn_m=gui_font_size/2,
		btn_w=xw/2,
		btn_h=gui_font_size+(gui_dpi/8);
	box_h+=btn_m;

	lv_obj_set_width(box->label,xw);
	box_h+=lv_obj_get_height(box->label);

	lv_style_set_margin_bottom(&style,LV_STATE_DEFAULT,btn_m);
	lv_style_set_radius(&style,LV_STATE_DEFAULT,gui_dpi/15);

	lv_obj_set_style_local_margin_bottom(box->input,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_pos(box->input,btn_m/2,box_h);
	lv_obj_set_width(box->input,xw-btn_m);
	box_h+=lv_obj_get_height(box->input)+btn_m;

	lv_obj_add_style(box->ok,LV_BTN_PART_MAIN,&style);
	lv_obj_set_size(box->ok,btn_w-btn_m,btn_h);
	lv_obj_set_pos(box->ok,btn_m/2,box_h);

	lv_obj_add_style(box->cancel,LV_BTN_PART_MAIN,&style);
	lv_obj_set_size(box->cancel,btn_w-btn_m,btn_h);
	lv_obj_set_pos(box->cancel,btn_m/2+btn_w,box_h);

	box_h+=btn_m+btn_h+gui_font_size/2;
	lv_obj_set_height(box->box,box_h);
	box_h+=lv_obj_get_style_pad_top(box->page,LV_PAGE_PART_BG);
	box_h+=lv_obj_get_style_pad_bottom(box->page,LV_PAGE_PART_BG);
	lv_obj_set_height(box->page,MIN(box_h,xh));
	lv_obj_align(box->page,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int inputbox_draw(struct gui_activity*act){
	struct inputbox*box=act->args;
	box->act=act;

	box->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(box->page,false);

	box->box=lv_obj_create(box->page,NULL);
	lv_obj_set_click(box->box,false);
	lv_obj_set_style_local_border_width(box->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	box->label=lv_label_create(box->box,NULL);
	lv_label_set_align(box->label,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(box->label,LV_LABEL_LONG_BREAK);
	lv_label_set_text(box->label,box->text);

	box->input=lv_textarea_create(box->box,NULL);
	lv_textarea_set_text(box->input,box->content);
	lv_textarea_set_cursor_hidden(box->input,true);
	lv_textarea_set_pwd_mode(box->input,box->pwd);
	lv_textarea_set_text_sel(box->input,box->sel);
	lv_textarea_set_one_line(box->input,box->one_line);
	if(box->align!=LV_LABEL_ALIGN_AUTO)lv_textarea_set_text_align(box->input,box->align);
	if(box->max>0)lv_textarea_set_max_length(box->input,box->max);
	if(box->holder[0])lv_textarea_set_placeholder_text(box->input,box->holder);
	if(box->accepts)lv_textarea_set_accepted_chars(box->input,box->accepts);
	lv_obj_set_user_data(box->input,box);
	lv_obj_set_event_cb(box->input,text_cb);

	box->ok=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_user_data(box->ok,box);
	lv_obj_set_event_cb(box->ok,input_click);

	box->cancel=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->cancel,NULL),LV_SYMBOL_CLOSE);
	lv_obj_set_user_data(box->cancel,box);
	lv_obj_set_event_cb(box->cancel,input_click);

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
	.resize=inputbox_resize,
	.quiet_exit=inputbox_clean,
	.get_focus=inputbox_get_focus,
	.lost_focus=inputbox_lost_focus,
	.back=true,
	.mask=true,
	.allow_exclusive=true,
};

static void inputbox_cb(lv_task_t*t){
	guiact_start_activity(&guireg_inputbox,t->user_data);
}

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
	input->align=LV_LABEL_ALIGN_AUTO;
	lv_task_once(lv_task_create(inputbox_cb,0,LV_TASK_PRIO_LOWEST,input));
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

void inputbox_set_input_align(struct inputbox*input,lv_label_align_t align){
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

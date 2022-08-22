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
#include"gui/msgbox.h"
#include"gui/activity.h"

struct msgbox{
	char text[BUFSIZ];
	const char**buttons;
	msgbox_callback callback;
	uint16_t btn_cnt;
	lv_obj_t*box;
	lv_obj_t*label;
	lv_obj_t*btns;
	lv_obj_t*btn[64];
	void*btn_data_p;
	void*user_data;
	struct gui_activity*act;
};

struct msgbox_btn{
	uint16_t id;
	const char*text;
	struct msgbox*box;
};

static const char*null_btn[]={""};
static const char*ok_btn[]={LV_SYMBOL_OK,""};
static const char*yesno_btn[]={LV_SYMBOL_OK,LV_SYMBOL_CLOSE,""};

static void msg_click(lv_event_t*e){
	struct msgbox_btn*btn=e->user_data;
	struct msgbox*box=btn->box;
	if(guiact_get_last()->args!=box)return;
	if(box->callback&&box->callback(btn->id,btn->text,box->user_data))return;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_obj_set_user_data(box->btn[i],NULL);
	guiact_do_back();
}

static int msgbox_clean(struct gui_activity*act){
	struct msgbox*box=act->args;
	if(!box)return 0;
	free(box->btn_data_p);
	free(box);
	act->args=NULL;
	return 0;
}

static int msgbox_draw(struct gui_activity*act){
	struct msgbox*box=act->args;
	if(!box->buttons)box->buttons=null_btn;
	for(box->btn_cnt=0;*box->buttons[box->btn_cnt];box->btn_cnt++);
	if(box->btn_cnt>64)return 3;
	box->act=act;
	box->box=lv_draw_dialog_box(act->page,NULL,NULL);

	box->label=lv_label_create(box->box);
	lv_label_set_text(box->label,box->text);
	lv_label_set_long_mode(box->label,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(box->label,LV_TEXT_ALIGN_CENTER,0);
	lv_obj_set_width(box->label,lv_pct(100));

	box->btns=lv_draw_line_wrapper(box->box,NULL,NULL);
	lv_obj_set_flex_flow(box->btns,LV_FLEX_FLOW_ROW_WRAP);

	if(box->btn_cnt>0){
		struct msgbox_btn*d=malloc(sizeof(struct msgbox_btn)*box->btn_cnt);
		if(!d){
			free(box);
			act->args=NULL;
			return 4;
		}
		box->btn_data_p=d;
		for(uint16_t i=0;i<box->btn_cnt;i++){
			d[i].box=box,d[i].id=i,d[i].text=box->buttons[i];
			box->btn[i]=lv_draw_button(box->btns,_(box->buttons[i]),true,msg_click,&d[i]);
			if(box->btn_cnt<4)lv_obj_set_flex_grow(box->btn[i],1);
			else lv_obj_set_width(box->btn[i],lv_pct(100));
		}
	}
	return 0;
}

static int msgbox_get_focus(struct gui_activity*d){
	struct msgbox*box=d->args;
	if(!box)return 0;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_group_add_obj(gui_grp,box->btn[i]);
	return 0;
}

static int msgbox_lost_focus(struct gui_activity*d){
	struct msgbox*box=d->args;
	if(!box)return 0;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_group_remove_obj(box->btn[i]);
	return 0;
}

struct gui_register guireg_msgbox={
	.name="msgbox",
	.title="Message Box",
	.show_app=false,
	.quiet_exit=msgbox_clean,
	.draw=msgbox_draw,
	.get_focus=msgbox_get_focus,
	.lost_focus=msgbox_lost_focus,
	.back=true,
	.mask=true,
	.allow_exclusive=true,
};

static struct msgbox*_msgbox_create(
	msgbox_callback callback,
	const char**buttons,
	const char*content,
	va_list va
){
	struct msgbox*msg=malloc(sizeof(struct msgbox));
	if(!msg)return NULL;
	memset(msg,0,sizeof(struct msgbox));
	vsnprintf(msg->text,sizeof(msg->text)-1,_(content),va);
	msg->callback=callback;
	msg->buttons=buttons;
	guiact_start_activity(&guireg_msgbox,msg);
	return msg;
}

struct msgbox*msgbox_create_yesno(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,yesno_btn,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_create_ok(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,ok_btn,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_create_custom(msgbox_callback callback,const char**btns,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,btns,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_alert(const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(NULL,ok_btn,content,va);
	va_end(va);
	return msg;
}

void msgbox_set_user_data(struct msgbox*box,void*user_data){
	if(!box)return;
	box->user_data=user_data;
}
#endif

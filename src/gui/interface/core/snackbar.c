/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
#include"gui.h"
#include"gui/snackbar.h"

static struct{
	lv_color_t color;
	lv_coord_t off_btm;
	lv_coord_t height;
	uint32_t show_time;
	lv_anim_t show_anim;
	lv_anim_t hide_anim;
	lv_style_t style;
	lv_task_t*hide_tsk;
	lv_font_t*font;
	lv_obj_t*bar;
	lv_obj_t*content;
	lv_obj_t*last_focus;
	runnable_t*on_click;
	void*data_click;
	runnable_t*on_dismiss;
	void*data_dismiss;
}sb;

static void snackbar_hide_task(lv_task_t*tsk){
	if(tsk==sb.hide_tsk)sb.hide_tsk=NULL;
	snackbar_hide();
}

void snackbar_show(void){
	lv_obj_t*par=lv_obj_get_parent(sb.bar);
	lv_obj_set_hidden(sb.bar,false);
	lv_obj_set_size(sb.bar,lv_obj_get_width(par),sb.height);
	if(sb.hide_tsk){
		lv_task_del(sb.hide_tsk);
		sb.hide_tsk=NULL;
	}else{
		lv_anim_set_values(
			&sb.show_anim,
			lv_obj_get_height(par)+sb.height,
			lv_obj_get_height(par)-sb.off_btm-sb.height
		);
		lv_anim_start(&sb.show_anim);
	}
	sb.hide_tsk=lv_task_create(
		snackbar_hide_task,
		sb.show_time,
		LV_TASK_PRIO_LOWEST,
		NULL
	);
	lv_task_once(sb.hide_tsk);
}

void snackbar_hide(void){
	lv_obj_t*par=lv_obj_get_parent(sb.bar);
	if(sb.hide_tsk){
		lv_task_del(sb.hide_tsk);
		sb.hide_tsk=NULL;
	}
	lv_obj_set_size(sb.bar,lv_obj_get_width(par),sb.height);
	lv_anim_set_values(
		&sb.hide_anim,
		lv_obj_get_height(par)-sb.off_btm-sb.height,
		lv_obj_get_height(par)+sb.height
	);
	lv_anim_start(&sb.hide_anim);
}

void snackbar_set_on_click(runnable_t cb,void*data){
	sb.on_click=cb,sb.data_click=data;
}

void snackbar_set_on_dismiss(runnable_t cb,void*data){
	sb.on_dismiss=cb,sb.data_dismiss=data;
}

void snackbar_set_show_time(uint32_t ms){
	sb.show_time=ms;
}

void snackbar_set_anim_time(uint32_t time){
	lv_anim_set_time(&sb.show_anim,time);
	lv_anim_set_time(&sb.hide_anim,time);
}

void snackbar_set_text(const char*text){
	lv_coord_t tx=gui_font_size,tm=tx/2;
	lv_coord_t tw=lv_obj_get_width(sb.bar)-tx;
	lv_coord_t th=lv_obj_get_height(sb.bar)-tx;
	lv_obj_set_size(sb.content,tw,sb.font->line_height);
	lv_label_set_text(sb.content,text?:"");
	lv_obj_align(
		sb.content,NULL,
		LV_ALIGN_IN_LEFT_MID,
		tm,0
	);
	if(lv_obj_get_y(sb.content)<tm)
		lv_obj_set_y(sb.content,tm);
	if(lv_obj_get_height(sb.content)>th)
		lv_obj_set_height(sb.content,th);
}

static void _snackbar_set_text_fmt(const char*fmt,va_list va){
	char text[BUFSIZ];
	memset(text,0,sizeof(text));
	if(fmt)vsnprintf(text,sizeof(text)-1,fmt,va);
	snackbar_set_text(text);
}

void snackbar_set_text_fmt(const char*fmt,...){
	va_list va;
	va_start(va,fmt);
	_snackbar_set_text_fmt(fmt,va);
	va_end(va);
}

void snackbar_show_text_fmt(const char*fmt,...){
	va_list va;
	va_start(va,fmt);
	snackbar_show();
	_snackbar_set_text_fmt(fmt,va);
	va_end(va);
}

void snackbar_show_text(const char*text){
	snackbar_show();
	snackbar_set_text(text);
}

void snackbar_set_color(lv_color_t bg_color,lv_color_t txt_color){
	lv_obj_remove_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	lv_style_set_bg_color(&sb.style,LV_STATE_DEFAULT,bg_color);
	lv_style_set_text_color(&sb.style,LV_STATE_DEFAULT,txt_color);
	lv_style_set_shadow_color(&sb.style,LV_STATE_DEFAULT,bg_color);
	lv_obj_add_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
}

void snackbar_set_font(lv_font_t*font){
	if(!font)return;
	lv_obj_remove_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	lv_style_set_text_font(&sb.style,LV_STATE_DEFAULT,font);
	lv_obj_add_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	sb.font=font;
}

void snackbar_set_height(lv_coord_t height){
	lv_obj_remove_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	lv_style_set_shadow_width(&sb.style,LV_STATE_DEFAULT,height);
	lv_style_set_shadow_ofs_y(&sb.style,LV_STATE_DEFAULT,-height/10);
	lv_obj_add_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	sb.height=height;
}

static void snackbar_focus_cb(lv_anim_t*anim __attribute__((unused))){
	lv_obj_t*foc=lv_group_get_focused(gui_grp);
	if(foc&&foc!=sb.bar)sb.last_focus=foc;
	lv_group_add_obj(gui_grp,sb.bar);
	lv_group_focus_obj(sb.bar);
}

static void snackbar_defocus_cb(lv_anim_t*anim __attribute__((unused))){
	if(sb.last_focus&&sb.last_focus!=sb.bar)
		lv_group_focus_obj(sb.last_focus);
	sb.last_focus=NULL;
	lv_group_remove_obj(sb.bar);
	lv_obj_set_hidden(sb.bar,true);
	snackbar_set_text(NULL);
	if(sb.on_dismiss)sb.on_dismiss(sb.data_dismiss);
	sb.on_dismiss=NULL,sb.data_dismiss=NULL;
}

static void snackbar_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=sb.bar||e!=LV_EVENT_CLICKED)return;
	if(
		!sb.on_click||
		sb.on_click(sb.data_click)==0
	)snackbar_hide();
	sb.on_click=NULL,sb.data_click=NULL;
}

void snackbar_draw(lv_obj_t*scr,lv_coord_t off,lv_coord_t height){
	sb.off_btm=off;
	sb.bar=lv_obj_create(scr,NULL);
	lv_theme_apply(sb.bar,LV_THEME_SCR);
	lv_obj_set_event_cb(sb.bar,snackbar_cb);
	lv_obj_set_click(sb.bar,true);
	lv_obj_set_hidden(sb.bar,true);
	lv_style_init(&sb.style);
	lv_obj_add_style(sb.bar,LV_OBJ_PART_MAIN,&sb.style);
	snackbar_set_color(lv_color_darken(LV_COLOR_GRAY,LV_OPA_30),LV_COLOR_WHITE);
	snackbar_set_font(gui_font_small);
	snackbar_set_height(height);
	snackbar_set_show_time(10000);
	sb.content=lv_label_create(sb.bar,NULL);
	lv_label_set_long_mode(sb.content,LV_LABEL_LONG_BREAK);
	snackbar_set_text(NULL);
	lv_anim_init(&sb.show_anim);
	lv_anim_init(&sb.hide_anim);
	lv_anim_set_var(&sb.show_anim,sb.bar);
	lv_anim_set_var(&sb.hide_anim,sb.bar);
	lv_anim_set_ready_cb(&sb.show_anim,snackbar_focus_cb);
	lv_anim_set_ready_cb(&sb.hide_anim,snackbar_defocus_cb);
	lv_anim_set_exec_cb(&sb.show_anim,(lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_exec_cb(&sb.hide_anim,(lv_anim_exec_xcb_t)lv_obj_set_y);
	snackbar_set_anim_time(100);
}

#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<time.h>
#include<string.h>
#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
#include"gui.h"
#include"hardware.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/snackbar.h"
#include"gui/activity.h"
#include"gui/clipboard.h"
struct sysbar sysbar;

static struct{
	bool enabled;
	char*key,*img,*text;
}btns[]={
	{ true, "copy",       LV_SYMBOL_COPY,  "Copy selected" },
	{ true, "copy-all",   LV_SYMBOL_COPY,  "Copy all"      },
	{ true, "cut",        LV_SYMBOL_CUT,   "Cut selected"  },
	{ true, "cut-all",    LV_SYMBOL_CUT,   "Cut all"       },
	{ true, "paste",      LV_SYMBOL_PASTE, "Paste"         },
	{ true, "delete",     LV_SYMBOL_TRASH, "Delete"        },
	{ true, "delete-all", LV_SYMBOL_TRASH, "Delete all"    },
	{ true, "select-all", "\uf065",        "Select all"    },
	{ true, "deselect",   "\uf066",        "Deselect"      },
	{ true, "ctrl-pad",   "\uf0b2",        "Control Pad"   },
	{ true, "close",      LV_SYMBOL_CLOSE, "Close"         },
	{ false,NULL,NULL,NULL}
};

static void sysbar_thread(struct sysbar*b){
	#ifndef ENABLE_UEFI
	int bats[64];
	int bat,lvl;
	char*sym=NULL;
	#endif
	char timestr[64];
	time_t t=time(NULL);
	struct tm*tt=localtime(&t);
	memset(timestr,0,sizeof(timestr));
	if(tt&&strftime(timestr,sizeof(timestr)-1,"%H:%M",tt)>0)
		lv_label_set_text(b->top.content.time,timestr);
	#ifndef ENABLE_UEFI
	memset(bats,0,sizeof(bats));
	bat=pwr_scan_device(bats,sizeof(bats)-1,true);
	if(bat>=0&&(lvl=pwr_multi_get_capacity(bats))>=0){
		if(lvl<10)sym=LV_SYMBOL_BATTERY_EMPTY;
		else if(lvl<25)sym=LV_SYMBOL_BATTERY_1;
		else if(lvl<45)sym=LV_SYMBOL_BATTERY_2;
		else if(lvl<80)sym=LV_SYMBOL_BATTERY_3;
		else sym=LV_SYMBOL_BATTERY_FULL;
		lv_label_set_text_fmt(b->top.content.level,"%d%%",lvl);
		lv_label_set_text(b->top.content.battery,sym);
		lv_obj_clear_flag(b->top.content.level,LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(b->top.content.battery,LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_label_set_text_fmt(b->top.content.level,"---");
		lv_obj_add_flag(b->top.content.level,LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(b->top.content.battery,LV_OBJ_FLAG_HIDDEN);
	}
	pwr_close_device(bats);
	#endif
}

static void sysbar_thread_cb(lv_timer_t*a){
	if(!sysbar.content){
		lv_timer_del(a);
		return;
	}
	sysbar_thread((struct sysbar*)a->user_data);
}

static void set_bar_style(lv_obj_t*obj,uint8_t part){
	lv_obj_set_style_bg_color(
		obj,
		lv_color_darken(
			lv_obj_get_style_bg_color(obj,part),
			LV_OPA_20
		),part
	);
}

void sysbar_edit_menu_show(void){
	if(lv_obj_is_visible(sysbar.edit_menu)||!sysbar.focus_input)return;
	lv_obj_set_hidden(sysbar.edit_menu,false);
	lv_group_add_obj(gui_grp,sysbar.edit_menu);
	lv_group_focus_obj(sysbar.edit_menu);
	if(sysbar.keyboard)lv_group_remove_obj(sysbar.keyboard);
	lv_group_set_editing(gui_grp,true);
	lv_textarea_t*ext=(lv_textarea_t*)sysbar.focus_input;
	bool sel=ext->sel_start-ext->sel_end>0;
	bool clip=clipboard_get_type()==CLIP_TEXT;
	bool ena=lv_textarea_get_text_selection(sysbar.focus_input);
	lv_obj_set_enabled(sysbar.edit_btns[0],sel&&ena);//copy
	lv_obj_set_enabled(sysbar.edit_btns[2],sel&&ena);//cut
	lv_obj_set_enabled(sysbar.edit_btns[4],clip);//paste
	lv_obj_set_enabled(sysbar.edit_btns[7],ena);//select-all
	lv_obj_set_enabled(sysbar.edit_btns[8],sel&&ena);//deselect
}

void sysbar_edit_menu_hide(void){
	if(!lv_obj_is_visible(sysbar.edit_menu))return;
	lv_obj_set_hidden(sysbar.edit_menu,true);
	if(sysbar.keyboard){
		lv_group_add_obj(gui_grp,sysbar.keyboard);
		lv_group_focus_obj(sysbar.keyboard);
		lv_group_set_editing(gui_grp,true);
	}
	lv_group_remove_obj(sysbar.edit_menu);
}

static void edit_menu_cb(lv_event_t*e){
	if(sysbar.focus_input){
		lv_obj_t*ta=sysbar.focus_input;
		char*c=e->user_data;
		const char*cont=lv_textarea_get_text(ta);
		lv_textarea_t*ext=(lv_textarea_t*)ta;
		uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
		bool copy=strcmp(c,"copy")==0;
		bool cut=strcmp(c,"cut")==0;
		bool copy_all=strcmp(c,"copy-all")==0;
		bool cut_all=strcmp(c,"cut-all")==0;
		if(copy||cut||copy_all||cut_all){
			if(copy_all||cut_all)ss=0,sl=strlen(cont);
			if(sl>0){
				clipboard_set(CLIP_TEXT,cont+ss,sl);
				if(cut){
					lv_textarea_remove_text(ta,ss,sl);
					lv_textarea_set_cursor_pos(ta,ss);
				}else if(cut_all)lv_textarea_set_text(ta,"");
			}
		}else if(strcmp(c,"delete")==0){
			if(sl>0){
				lv_textarea_remove_text(ta,ss,sl);
				lv_textarea_set_cursor_pos(ta,ss);
			}else lv_textarea_del_char(ta);
		}else if(strcmp(c,"delete-all")==0){
			lv_textarea_set_text(ta,"");
		}else if(strcmp(c,"select-all")==0){
			ext->sel_start=0,ext->sel_end=strlen(cont);
			lv_label_set_text_sel_start(ext->label,ext->sel_start);
			lv_label_set_text_sel_end(ext->label,ext->sel_end);
			lv_textarea_set_cursor_pos(ta,ext->sel_end);
		}else if(strcmp(c,"deselect")==0){
			lv_textarea_clear_selection(ta);
			ext->sel_start=lv_textarea_get_cursor_pos(ta);
			ext->sel_end=ext->sel_start;
		}else if(strcmp(c,"paste")==0&&clipboard_get_type()==CLIP_TEXT){
			if(sl>0){
				lv_textarea_remove_text(ta,ss,sl);
				lv_textarea_set_cursor_pos(ta,ss);
			}
			lv_textarea_add_text(ta,clipboard_get_content());
		}else if(strcmp(c,"ctrl-pad")==0){
			sysbar_keyboard_close();
			ctrl_pad_show();
		}
	}
	sysbar_edit_menu_hide();
}

static void keyboard_toggle(lv_event_t*e){
	if(e->target==sysbar.bottom.content.keyboard&&e->code!=LV_EVENT_CLICKED)return;
	if(e->target==sysbar.keyboard&&e->code==LV_EVENT_KEY&&sysbar.focus_input){
		uint32_t key=lv_indev_get_key(lv_indev_get_act());
		switch(key){
			case LV_KEY_UP:
			case LV_KEY_DOWN:
			case LV_KEY_RIGHT:
			case LV_KEY_LEFT:
			case LV_KEY_ESC:
			case LV_KEY_ENTER:
			case LV_KEY_NEXT:
			case LV_KEY_PREV:
			case LV_KEY_HOME:
			case LV_KEY_END:break;
			case LV_KEY_DEL:lv_textarea_del_char_forward(sysbar.focus_input);break;
			case LV_KEY_BACKSPACE:lv_textarea_del_char(sysbar.focus_input);break;
			default:lv_textarea_add_char(sysbar.focus_input,key);
		}
	}
	if(e->target==sysbar.keyboard){
		if(e->code==LV_EVENT_VALUE_CHANGED){
			uint16_t sel=lv_btnmatrix_get_selected_btn(e->target);
			if(strcmp(lv_btnmatrix_get_btn_text(e->target,sel),LV_SYMBOL_EDIT)==0)
				sysbar_edit_menu_show();
		}
		return;
	}
	int w=gui_w,h=gui_h/3;
	if(sysbar.keyboard){
		gui_sh+=h;
		lv_obj_del(sysbar.keyboard);
		sysbar.keyboard=NULL;
		if(sysbar.focus_input){
			lv_event_send(sysbar.focus_input,LV_EVENT_DEFOCUSED,NULL);
		}
		lv_group_set_editing(gui_grp,false);
		sysbar_show_bar();
		sysbar_edit_menu_hide();
		struct gui_activity*act=guiact_get_last();
		lv_obj_set_height(sysbar.content,gui_sh);
		if(act){
			lv_obj_set_height(act->page,gui_sh);
			act->w=gui_sw,act->h=gui_sh;
			if(act->reg->resize)act->reg->resize(act);
		}
		return;
	}
	sysbar_show_bar();
	sysbar_edit_menu_hide();
	gui_sh-=h;
	sysbar.keyboard=lv_keyboard_create(sysbar.screen);
	lv_obj_move_foreground(sysbar.edit_menu);
	lv_obj_set_size(sysbar.keyboard,w,h);
	lv_obj_align_to(sysbar.keyboard,sysbar.bottom.bar,LV_ALIGN_OUT_TOP_MID,0,0);
	lv_obj_add_event_cb(sysbar.keyboard,keyboard_toggle,LV_EVENT_ALL,NULL);
	if(sysbar.focus_input){
		lv_group_focus_obj(sysbar.focus_input);
		lv_obj_scroll_to_view(sysbar.focus_input,LV_ANIM_OFF);
		lv_keyboard_set_textarea(sysbar.keyboard,sysbar.focus_input);
		lv_event_send(sysbar.focus_input,LV_EVENT_FOCUSED,NULL);
	}
	lv_group_add_obj(gui_grp,sysbar.keyboard);
	lv_group_focus_obj(sysbar.keyboard);
	lv_group_set_editing(gui_grp,true);
	if(sysbar.hide)lv_timer_del(sysbar.hide);
	sysbar.hide=NULL;
	struct gui_activity*act=guiact_get_last();
	lv_obj_set_height(sysbar.content,gui_sh);
	if(act){
		lv_obj_set_height(act->page,gui_sh);
		act->w=gui_sw,act->h=gui_sh;
		if(act->reg->resize)act->reg->resize(act);
	}
}

void sysbar_keyboard_toggle(){
	keyboard_toggle(&(lv_event_t){
		.target=sysbar.bottom.content.keyboard,
		.code=LV_EVENT_CLICKED
	});
}

void sysbar_keyboard_close(){
	if(sysbar.keyboard)sysbar_keyboard_toggle();
}

void sysbar_keyboard_open(){
	if(!sysbar.keyboard)sysbar_keyboard_toggle();
}

void sysbar_focus_input(lv_obj_t*obj){
	if(sysbar.focus_input){
		if(sysbar.focus_input!=obj)lv_event_send(sysbar.focus_input,LV_EVENT_DEFOCUSED,NULL);
	}
	sysbar.focus_input=obj;
	if(!obj)return;
	if(sysbar.keyboard){
		lv_group_focus_obj(sysbar.keyboard);
		lv_group_set_editing(gui_grp,true);
		lv_keyboard_set_textarea(sysbar.keyboard,sysbar.focus_input);
	}
	lv_event_send(sysbar.focus_input,LV_EVENT_FOCUSED,NULL);
}

static void show_bar(lv_event_t*e __attribute__((unused))){
	sysbar_show_bar();
}

static void back_click(lv_event_t*e __attribute__((unused))){
	guiact_do_back();
}

static void home_click(lv_event_t*e __attribute__((unused))){
	guiact_do_home();
}

static void power_click(lv_event_t*e __attribute__((unused))){
	if(guiact_has_activity_name("reboot-menu"))return;
	if(sysbar.keyboard)keyboard_toggle(&(lv_event_t){
		.target=sysbar.keyboard,
		.code=LV_EVENT_CLICKED
	});
	guiact_start_activity_by_name("reboot-menu",NULL);
}

static void hide_cb(lv_timer_t*t __attribute__((unused))){
	sysbar.hide=NULL;
	if(!sysbar.full_screen)return;
	lv_obj_set_hidden(sysbar.top.bar,true);
	lv_obj_set_hidden(sysbar.bottom.bar,true);
	lv_group_remove_obj(sysbar.bottom.content.back);
	lv_group_remove_obj(sysbar.bottom.content.home);
	lv_group_remove_obj(sysbar.bottom.content.keyboard);
	lv_group_remove_obj(sysbar.bottom.content.power);
}

static lv_obj_t*draw_bottom_button(char*symbol,lv_event_cb_t cb){
	lv_obj_t*btn=lv_btn_create(sysbar.bottom.buttons);
	lv_obj_t*text=lv_label_create(btn);
	lv_label_set_text(text,symbol);
	lv_obj_center(text);
	lv_obj_set_style_outline_width(btn,gui_dpi/200,LV_EVENT_FOCUSED);
	lv_obj_set_style_radius(btn,8,0);
	lv_obj_set_style_pad_all(btn,8,0);
	lv_obj_set_style_border_width(btn,0,0);
	lv_obj_set_style_shadow_width(btn,0,0);
	lv_obj_set_style_text_color(btn,gui_dark?lv_color_white():lv_color_black(),0);
	lv_obj_set_style_bg_color(btn,lv_color_darken(
		lv_obj_get_style_bg_color(sysbar.bottom.bar,0),
		LV_OPA_20
	),0);
	lv_obj_set_style_bg_opa(btn,LV_OPA_20,LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(btn,LV_OPA_0,0);
	lv_obj_set_size(btn,sysbar.size*2,LV_PCT(100));
	lv_group_add_obj(gui_grp,btn);
	lv_obj_add_event_cb(btn,show_bar,LV_EVENT_PRESSING,NULL);
	if(cb)lv_obj_add_event_cb(btn,cb,LV_EVENT_CLICKED,NULL);
	return btn;
}

static void sysbar_draw_bottom(){
	sysbar.bottom.bar=lv_obj_create(sysbar.screen);
	lv_obj_set_style_radius(sysbar.bottom.bar,0,0);
	lv_obj_set_style_pad_all(sysbar.bottom.bar,0,0);
	lv_obj_set_style_border_width(sysbar.bottom.bar,0,0);
	lv_obj_set_scroll_dir(sysbar.bottom.bar,LV_DIR_NONE);
	lv_obj_add_event_cb(sysbar.bottom.bar,show_bar,LV_EVENT_ALL,NULL);
	lv_obj_set_size(sysbar.bottom.bar,gui_w,sysbar.size);
	lv_obj_align(sysbar.bottom.bar,LV_ALIGN_BOTTOM_MID,0,0);
	set_bar_style(sysbar.bottom.bar,LV_PART_MAIN);

	sysbar.bottom.buttons=lv_obj_create(sysbar.bottom.bar);
	lv_obj_set_style_radius(sysbar.bottom.buttons,0,0);
	lv_obj_set_style_pad_all(sysbar.bottom.buttons,gui_dpi/100,0);
	lv_obj_set_style_border_width(sysbar.bottom.buttons,0,0);
	lv_obj_set_size(sysbar.bottom.buttons,LV_SIZE_CONTENT,LV_PCT(100));
	lv_obj_set_style_bg_opa(sysbar.bottom.buttons,LV_OPA_0,0);
	lv_obj_set_flex_flow(sysbar.bottom.buttons,LV_FLEX_FLOW_ROW);
	lv_obj_set_height(sysbar.bottom.buttons,LV_PCT(100));
	lv_obj_center(sysbar.bottom.buttons);

	sysbar.bottom.content.back=draw_bottom_button(LV_SYMBOL_LEFT,back_click);
	sysbar.bottom.content.home=draw_bottom_button(LV_SYMBOL_HOME,home_click);
	sysbar.bottom.content.keyboard=draw_bottom_button(LV_SYMBOL_KEYBOARD,keyboard_toggle);
	sysbar.bottom.content.power=draw_bottom_button(LV_SYMBOL_POWER,power_click);
}

static void sysbar_draw_top(){
	int x=gui_dpi/8;
	sysbar.top.bar=lv_obj_create(sysbar.screen);
	lv_obj_set_style_radius(sysbar.top.bar,0,0);
	lv_obj_set_style_pad_all(sysbar.top.bar,0,0);
	lv_obj_set_style_border_width(sysbar.top.bar,0,0);
	lv_obj_set_scroll_dir(sysbar.top.bar,LV_DIR_NONE);
	lv_obj_add_event_cb(sysbar.top.bar,show_bar,LV_EVENT_ALL,NULL);
	lv_obj_set_size(sysbar.top.bar,gui_w,sysbar.size);
	lv_obj_align(sysbar.top.bar,LV_ALIGN_TOP_MID,0,0);
	set_bar_style(sysbar.top.bar,LV_PART_MAIN);

	sysbar.top.content.time=lv_label_create(sysbar.top.bar);
	lv_label_set_text(sysbar.top.content.time,"00:00");
	lv_obj_align(sysbar.top.content.time,LV_ALIGN_LEFT_MID,x,0);

	#ifndef ENABLE_UEFI
	sysbar.top.content.level=lv_label_create(sysbar.top.bar);
	lv_label_set_text(sysbar.top.content.level,"000%");
	lv_obj_align(sysbar.top.content.level,LV_ALIGN_RIGHT_MID,-x,0);

	sysbar.top.content.battery=lv_label_create(sysbar.top.bar);
	lv_label_set_text(sysbar.top.content.battery,LV_SYMBOL_BATTERY_EMPTY);
	lv_obj_align_to(
		sysbar.top.content.battery,
		sysbar.top.content.level,
		LV_ALIGN_OUT_LEFT_MID,
		-(gui_font_size/2),0
	);
	#endif
}

int sysbar_draw(lv_obj_t*scr){
	sysbar.screen=scr;
	sysbar.size=gui_font_size+(gui_dpi/10);
	gui_sh=gui_h-(sysbar.size*2);
	sysbar.content=lv_obj_create(scr);
	lv_obj_set_scroll_dir(sysbar.content,LV_DIR_NONE);
	lv_obj_set_style_radius(sysbar.content,0,0);
	lv_obj_set_style_pad_all(sysbar.content,0,0);
	lv_obj_set_style_border_width(sysbar.content,0,0);
	lv_obj_set_size(sysbar.content,gui_sw,gui_sh);
	lv_obj_set_pos(sysbar.content,0,sysbar.size);

	sysbar_draw_top();
	sysbar_draw_bottom();
	sysbar_thread(&sysbar);

	lv_coord_t bs=gui_font_size*3;
	lv_coord_t bm=gui_font_size*4;
	sysbar.bar_btn=lv_btn_create(scr);
	lv_obj_add_drag(sysbar.bar_btn);
	lv_obj_add_event_cb(sysbar.bar_btn,show_bar,LV_EVENT_CLICKED,NULL);
	lv_obj_set_checked(sysbar.bar_btn,true);
	lv_obj_set_hidden(sysbar.bar_btn,true);
	lv_obj_set_size(sysbar.bar_btn,bs,bs);
	lv_obj_set_pos(sysbar.bar_btn,gui_w-bm,gui_h-sysbar.size-bm);
	lv_obj_set_style_shadow_color(sysbar.bar_btn,lv_palette_main(LV_PALETTE_GREY),0);
	lv_obj_set_style_shadow_width(sysbar.bar_btn,gui_font_size,0);
	lv_obj_set_style_radius(sysbar.bar_btn,LV_RADIUS_CIRCLE,0);
	lv_obj_t*txt=lv_label_create(sysbar.bar_btn);
	lv_label_set_text(txt,"\uf066");
	lv_obj_center(txt);

	sysbar.edit_menu=lv_list_create(scr);
	for(size_t i=0;btns[i].enabled;i++){
		lv_obj_t*o=lv_list_add_btn(
			sysbar.edit_menu,
			btns[i].img,
			_(btns[i].text)
		);
		sysbar.edit_btns[i]=o;
		lv_obj_add_event_cb(
			o,edit_menu_cb,
			LV_EVENT_CLICKED,
			btns[i].key
		);
		lv_obj_set_style_outline_width(
			o,0,LV_STATE_FOCUSED
		);
		lv_obj_set_style_bg_color(
			o,
			lv_color_darken(
				lv_color_white(),
				LV_OPA_10
			),
			LV_STATE_FOCUSED
		);
	}
	lv_obj_set_hidden(sysbar.edit_menu,true);
	lv_obj_set_size(sysbar.edit_menu,gui_w/2,LV_SIZE_CONTENT);
	lv_obj_update_layout(sysbar.edit_menu);
	lv_obj_set_pos(
		sysbar.edit_menu,
		gui_w-lv_obj_get_width(sysbar.edit_menu)-gui_font_size,
		gui_h-sysbar.size-lv_obj_get_height(sysbar.edit_menu)-gui_font_size
	);

	ctrl_pad_draw();
	snackbar_draw(sysbar.screen,sysbar.size,sysbar.size/2*3);

	lv_timer_create(sysbar_thread_cb,5000,&sysbar);
	return 0;
}

void sysbar_set_full_screen(bool fs){
	if(fs==sysbar.full_screen)return;
	sysbar.full_screen=fs;
	lv_obj_set_hidden(sysbar.bar_btn,!fs);
	lv_obj_set_hidden(sysbar.top.bar,fs);
	lv_obj_set_hidden(sysbar.bottom.bar,fs);
	lv_obj_set_pos(sysbar.content,0,fs?0:sysbar.size);
	if(fs)lv_group_add_obj(gui_grp,sysbar.bar_btn);
	else lv_group_remove_obj(sysbar.bar_btn);
	gui_sh=gui_h-(fs?0:sysbar.size*2);
	lv_obj_set_size(sysbar.content,gui_sw,gui_sh);
	struct gui_activity*act=guiact_get_last();
	if(act){
		act->w=gui_sw;
		act->h=gui_sh;
		lv_obj_set_size(act->page,act->w,act->h);
		if(act->reg->resize)act->reg->resize(act);
	}
	if(fs){
		lv_group_remove_obj(sysbar.bottom.content.back);
		lv_group_remove_obj(sysbar.bottom.content.home);
		lv_group_remove_obj(sysbar.bottom.content.keyboard);
		lv_group_remove_obj(sysbar.bottom.content.power);
	}else{
		lv_group_add_obj(gui_grp,sysbar.bottom.content.back);
		lv_group_add_obj(gui_grp,sysbar.bottom.content.home);
		lv_group_add_obj(gui_grp,sysbar.bottom.content.keyboard);
		lv_group_add_obj(gui_grp,sysbar.bottom.content.power);
	}
}

void sysbar_hide_full_screen_btn(){
	lv_obj_add_flag(sysbar.bar_btn,LV_OBJ_FLAG_HIDDEN);
	lv_group_remove_obj(sysbar.bar_btn);
}

void sysbar_show_bar(){
	if(!sysbar.full_screen)return;
	lv_group_add_obj(gui_grp,sysbar.bottom.content.back);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.home);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.keyboard);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.power);
	lv_obj_set_hidden(sysbar.top.bar,false);
	lv_obj_set_hidden(sysbar.bottom.bar,false);
	if(sysbar.hide)lv_timer_reset(sysbar.hide);
	else{
		sysbar.hide=lv_timer_create(hide_cb,5000,NULL);
		lv_timer_set_repeat_count(sysbar.hide,1);
	}
}

#endif

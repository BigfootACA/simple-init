/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<time.h>
#include"gui.h"
#include"array.h"
#include"hardware.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/clipboard.h"
struct sysbar sysbar;

static void sysbar_thread(struct sysbar*b){
	#ifndef ENABLE_UEFI
	int bats[64]={0};
	int bat,lvl;
	char*sym=NULL;
	#endif
	char timestr[64]={0};
	time_t t=time(NULL);
	struct tm*tt=localtime(&t);
	if(tt&&strftime(timestr,63,"%H:%M",tt)>0)
		lv_label_set_text(b->top.content.time,timestr);
	#ifndef ENABLE_UEFI
	bat=pwr_scan_device(bats,63,true);
	if(bat>=0&&(lvl=pwr_multi_get_capacity(bats))>=0){
		if(lvl<10)sym=LV_SYMBOL_BATTERY_EMPTY;
		else if(lvl<25)sym=LV_SYMBOL_BATTERY_1;
		else if(lvl<45)sym=LV_SYMBOL_BATTERY_2;
		else if(lvl<65)sym=LV_SYMBOL_BATTERY_3;
		else if(lvl>85)sym=LV_SYMBOL_BATTERY_FULL;
		lv_label_set_text_fmt(b->top.content.level,"%d%%",lvl);
		lv_label_set_text(b->top.content.battery,sym);
	}else lv_label_set_text_fmt(b->top.content.level,"---");
	pwr_close_device(bats);
	#endif
}

static void sysbar_thread_cb(lv_task_t*a){
	if(!sysbar.content){
		lv_task_del(a);
		return;
	}
	sysbar_thread((struct sysbar*)a->user_data);
}

static void set_bar_style(lv_obj_t*obj,uint8_t part){
	lv_theme_apply(obj,LV_THEME_SCR);
	lv_obj_set_style_local_bg_color(
		obj,part,
		LV_STATE_DEFAULT,
		lv_color_darken(
			lv_obj_get_style_bg_color(obj,part),
			LV_OPA_20
		)
	);
}

static void edit_menu_show(){
	if(lv_obj_is_visible(sysbar.edit_menu)||!sysbar.focus_input)return;
	lv_obj_set_hidden(sysbar.edit_menu,false);
	lv_group_add_obj(gui_grp,sysbar.edit_menu);
	lv_group_focus_obj(sysbar.edit_menu);
	if(sysbar.keyboard)lv_group_remove_obj(sysbar.keyboard);
	lv_group_set_editing(gui_grp,true);
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(sysbar.focus_input);
	bool sel=ext->sel_start-ext->sel_end>0;
	bool clip=clipboard_get_type()==CLIP_TEXT;
	bool ena=lv_textarea_get_text_sel_en(sysbar.focus_input);
	lv_obj_set_enabled(sysbar.edit_btns[0],sel&&ena);//copy
	lv_obj_set_enabled(sysbar.edit_btns[2],sel&&ena);//cut
	lv_obj_set_enabled(sysbar.edit_btns[4],clip);//paste
	lv_obj_set_enabled(sysbar.edit_btns[7],ena);//select-all
	lv_obj_set_enabled(sysbar.edit_btns[8],sel&&ena);//deselect
}

static void edit_menu_hide(){
	if(!lv_obj_is_visible(sysbar.edit_menu))return;
	lv_obj_set_hidden(sysbar.edit_menu,true);
	if(sysbar.keyboard){
		lv_group_add_obj(gui_grp,sysbar.keyboard);
		lv_group_focus_obj(sysbar.keyboard);
		lv_group_set_editing(gui_grp,true);
	}
	lv_group_remove_obj(sysbar.edit_menu);
}

static void edit_menu_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	if(sysbar.focus_input){
		lv_obj_t*ta=sysbar.focus_input;
		char*c=lv_obj_get_user_data(obj);
		const char*cont=lv_textarea_get_text(ta);
		lv_textarea_ext_t*ext=lv_obj_get_ext_attr(ta);
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
		}
	}
	edit_menu_hide();
}

static void keyboard_toggle(lv_obj_t*obj,lv_event_t e){
	if(obj==sysbar.bottom.content.keyboard&&e!=LV_EVENT_CLICKED)return;
	if(obj==sysbar.keyboard&&e!=LV_EVENT_CANCEL){
		if(
			e==LV_EVENT_VALUE_CHANGED&&
			strcmp(lv_btnmatrix_get_active_btn_text(obj),LV_SYMBOL_EDIT)==0
		){
			edit_menu_show();
			return;
		}
		if(e==LV_EVENT_FOCUSED||e==LV_EVENT_RELEASED){
			const char**x=lv_keyboard_get_map_array(sysbar.keyboard);
			for(size_t i=0;x[i]&&x[i][0];i++){
				if(strcmp(x[i],LV_SYMBOL_OK)!=0)continue;
				char**n=NULL;
				if(!(n=array_dup((char**)x)))break;
				n[i]=LV_SYMBOL_EDIT;
				lv_keyboard_set_map(
					obj,
					lv_keyboard_get_mode(obj),
					(const char**)n
				);
				break;
			}
		}
		lv_keyboard_def_event_cb(obj,e);
		return;
	}
	if(sysbar.keyboard){
		lv_obj_del(sysbar.keyboard);
		lv_obj_set_height(sysbar.content,gui_sh);
		sysbar.keyboard=NULL;
		if(sysbar.focus_input){
			lv_event_send(sysbar.focus_input,LV_EVENT_DEFOCUSED,NULL);
			lv_textarea_set_cursor_hidden(sysbar.focus_input,true);
		}
		lv_group_set_editing(gui_grp,false);
		sysbar_show_bar();
		edit_menu_hide();
		return;
	}
	int w=gui_w,h=gui_h/3;
	sysbar_show_bar();
	edit_menu_hide();
	sysbar.keyboard=lv_keyboard_create(sysbar.screen,NULL);
	lv_obj_move_foreground(sysbar.edit_menu);
	lv_obj_set_size(sysbar.keyboard,w,h);
	lv_obj_set_height(sysbar.content,gui_sh-h);
	lv_obj_set_y(sysbar.keyboard,gui_h-sysbar.size-h);
	lv_obj_set_event_cb(sysbar.keyboard,keyboard_toggle);
	lv_keyboard_set_cursor_manage(sysbar.keyboard,true);
	if(sysbar.focus_input){
		lv_group_focus_obj(sysbar.focus_input);
		lv_scroll_to(sysbar.focus_input,false);
		lv_keyboard_set_textarea(sysbar.keyboard,sysbar.focus_input);
		lv_event_send(sysbar.focus_input,LV_EVENT_FOCUSED,NULL);
	}
	lv_group_add_obj(gui_grp,sysbar.keyboard);
	lv_group_focus_obj(sysbar.keyboard);
	lv_group_set_editing(gui_grp,true);
	if(sysbar.hide)lv_task_del(sysbar.hide);
	sysbar.hide=NULL;
}

void sysbar_keyboard_toggle(){
	keyboard_toggle(sysbar.bottom.content.keyboard,LV_EVENT_CLICKED);
}

void sysbar_keyboard_close(){
	if(sysbar.keyboard)sysbar_keyboard_toggle();
}

void sysbar_keyboard_open(){
	if(!sysbar.keyboard)sysbar_keyboard_toggle();
}

void sysbar_focus_input(lv_obj_t*obj){
	if(sysbar.focus_input){
		lv_textarea_set_cursor_hidden(sysbar.focus_input,true);
		if(sysbar.focus_input!=obj)lv_event_send(sysbar.focus_input,LV_EVENT_DEFOCUSED,NULL);
	}
	sysbar.focus_input=obj;
	if(!obj)return;
	lv_group_focus_obj(sysbar.keyboard);
	lv_group_set_editing(gui_grp,true);
	if(sysbar.keyboard)lv_keyboard_set_textarea(sysbar.keyboard,sysbar.focus_input);
	lv_event_send(sysbar.focus_input,LV_EVENT_FOCUSED,NULL);
}

static void back_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_PRESSING)sysbar_show_bar();
	if(obj!=sysbar.bottom.content.back||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static void home_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_PRESSING)sysbar_show_bar();
	if(obj!=sysbar.bottom.content.home||e!=LV_EVENT_CLICKED)return;
	guiact_do_home();
}

static void power_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_PRESSING)sysbar_show_bar();
	if(obj!=sysbar.bottom.content.power||e!=LV_EVENT_CLICKED)return;
	if(guiact_has_activity_name("reboot-menu"))return;
	if(sysbar.keyboard)keyboard_toggle(sysbar.keyboard,LV_EVENT_CANCEL);
	guiact_start_activity_by_name("reboot-menu",NULL);
}

static void hide_cb(lv_task_t*t){
	if(t!=sysbar.hide||!sysbar.full_screen)return;
	lv_obj_set_hidden(sysbar.top.bar,true);
	lv_obj_set_hidden(sysbar.bottom.bar,true);
	lv_group_remove_obj(sysbar.bottom.content.back);
	lv_group_remove_obj(sysbar.bottom.content.home);
	lv_group_remove_obj(sysbar.bottom.content.keyboard);
	lv_group_remove_obj(sysbar.bottom.content.power);
	lv_task_del(sysbar.hide);
	sysbar.hide=NULL;
}

static void bar_cb(lv_obj_t*obj,lv_event_t e){
	if(obj!=sysbar.bar_btn)return;
	if(e==LV_EVENT_DRAG_END){
		lv_coord_t x=lv_obj_get_x(obj),y=lv_obj_get_y(obj);
		lv_coord_t w=lv_obj_get_width(obj),h=lv_obj_get_height(obj);
		lv_coord_t sx=gui_font_size,sy=gui_font_size;
		lv_coord_t sw=gui_w-w-gui_font_size,sh=gui_h-h-gui_font_size;
		if(x<sx)lv_obj_set_x(obj,sx);
		if(y<sy)lv_obj_set_y(obj,sy);
		if(x>sw)lv_obj_set_x(obj,sw);
		if(y>sh)lv_obj_set_y(obj,sh);
	}else if(e==LV_EVENT_CLICKED)sysbar_show_bar();
}

static void sysbar_cb(lv_obj_t*obj __attribute__((unused)),lv_event_t e){
	if(e==LV_EVENT_PRESSING)sysbar_show_bar();
}

static lv_obj_t*draw_bottom_button(char*symbol,lv_coord_t x,lv_event_cb_t cb){
	int bi=gui_dpi/200;
	if(!sysbar.bottom.style_inited){
		sysbar.bottom.style_inited=true;
		lv_style_init(&sysbar.bottom.btn_style);
		lv_style_set_outline_width(&sysbar.bottom.btn_style,LV_STATE_DEFAULT,bi);
		lv_style_set_border_width(&sysbar.bottom.btn_style,LV_STATE_DEFAULT,0);
		lv_style_set_margin_all(&sysbar.bottom.btn_style,LV_STATE_DEFAULT,0);
		lv_style_set_pad_all(&sysbar.bottom.btn_style,LV_STATE_DEFAULT,0);
		lv_style_set_radius(&sysbar.bottom.btn_style,LV_STATE_DEFAULT,8);
		lv_style_set_bg_color(
			&sysbar.bottom.btn_style,
			LV_STATE_DEFAULT,
			lv_color_darken(
				lv_obj_get_style_bg_color(
					sysbar.screen,
					LV_OBJ_PART_MAIN
				),LV_OPA_20
			)
		);
	}
	lv_obj_t*btn=lv_btn_create(sysbar.bottom.bar,NULL);
	lv_obj_t*text=lv_label_create(btn,NULL);
	lv_label_set_text(text,symbol);
	lv_obj_add_style(btn,LV_BTN_PART_MAIN,&sysbar.bottom.btn_style);
	lv_obj_set_size(btn,sysbar.size*2,sysbar.size-bi);
	lv_obj_align(btn,NULL,LV_ALIGN_IN_TOP_MID,x,bi);
	lv_group_add_obj(gui_grp,btn);
	if(cb)lv_obj_set_event_cb(btn,cb);
	return btn;
}

static void sysbar_draw_bottom(){
	sysbar.bottom.bar=lv_obj_create(sysbar.screen,NULL);
	lv_obj_set_event_cb(sysbar.bottom.bar,sysbar_cb);
	lv_obj_set_size(sysbar.bottom.bar,gui_w,sysbar.size);
	lv_obj_align(sysbar.bottom.bar,NULL,LV_ALIGN_IN_BOTTOM_LEFT,0,0);
	set_bar_style(sysbar.bottom.bar,LV_OBJ_PART_MAIN);

	int bm=gui_dpi/20;
	sysbar.bottom.content.back=draw_bottom_button(LV_SYMBOL_LEFT,-(sysbar.size*3+(bm*2)),back_click);
	sysbar.bottom.content.home=draw_bottom_button(LV_SYMBOL_HOME,-(sysbar.size+bm),home_click);
	sysbar.bottom.content.keyboard=draw_bottom_button(LV_SYMBOL_KEYBOARD,sysbar.size+bm,keyboard_toggle);
	sysbar.bottom.content.power=draw_bottom_button(LV_SYMBOL_POWER,sysbar.size*3+(bm*2),power_click);
}

static void sysbar_draw_top(){
	int x=gui_dpi/10,y=gui_dpi/20;
	sysbar.top.bar=lv_obj_create(sysbar.screen,NULL);
	lv_obj_set_event_cb(sysbar.top.bar,sysbar_cb);
	lv_obj_set_size(sysbar.top.bar,gui_w,sysbar.size);
	lv_obj_align(sysbar.top.bar,NULL,LV_ALIGN_IN_TOP_LEFT,0,0);
	set_bar_style(sysbar.top.bar,LV_OBJ_PART_MAIN);

	sysbar.top.content.time=lv_label_create(sysbar.top.bar,NULL);
	lv_label_set_text(sysbar.top.content.time,"00:00");
	lv_obj_align(sysbar.top.content.time,NULL,LV_ALIGN_IN_TOP_LEFT,x,y);

	#ifndef ENABLE_UEFI
	sysbar.top.content.level=lv_label_create(sysbar.top.bar,NULL);
	lv_label_set_text(sysbar.top.content.level,"000%");
	lv_obj_align(sysbar.top.content.level,NULL,LV_ALIGN_IN_TOP_RIGHT,-x,y);

	sysbar.top.content.battery=lv_label_create(sysbar.top.bar,NULL);
	lv_label_set_text(sysbar.top.content.battery,LV_SYMBOL_BATTERY_EMPTY);
	lv_obj_align(
		sysbar.top.content.battery,NULL,LV_ALIGN_IN_TOP_RIGHT,
		-gui_dpi/5-lv_obj_get_width(sysbar.top.content.level),
		y
	);
	#endif
}

int sysbar_draw(lv_obj_t*scr){
	sysbar.screen=scr;
	sysbar.size=gui_font_size+(gui_dpi/10);
	gui_sh=gui_h-(sysbar.size*2);
	sysbar.content=lv_page_create(scr,NULL);

	lv_obj_set_size(sysbar.content,gui_sw,gui_sh);
	lv_obj_set_pos(sysbar.content,0,sysbar.size);
	lv_theme_apply(sysbar.content,LV_THEME_SCR);

	sysbar_draw_top();
	sysbar_draw_bottom();
	sysbar_thread(&sysbar);

	lv_coord_t bs=gui_font_size*3;
	lv_coord_t bm=gui_font_size*4;
	sysbar.bar_btn=lv_btn_create(scr,NULL);
	lv_obj_set_event_cb(sysbar.bar_btn,bar_cb);
	lv_obj_set_checked(sysbar.bar_btn,true);
	lv_obj_set_hidden(sysbar.bar_btn,true);
	lv_obj_set_drag(sysbar.bar_btn,true);
	lv_obj_set_size(sysbar.bar_btn,bs,bs);
	lv_obj_set_pos(sysbar.bar_btn,gui_w-bm,gui_h-sysbar.size-bm);
	lv_obj_set_style_local_shadow_color(sysbar.bar_btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GRAY);
	lv_obj_set_style_local_shadow_width(sysbar.bar_btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size);
	lv_label_set_text(lv_label_create(sysbar.bar_btn,NULL),"\uf066");

	struct{
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
		{ true, "close",      LV_SYMBOL_CLOSE, "Close"         },
		{ false,NULL,NULL,NULL}
	};
	lv_coord_t h=0,w=gui_w/2;
	sysbar.edit_menu=lv_list_create(scr,NULL);
	for(size_t i=0;btns[i].enabled;i++){
		lv_obj_t*o=lv_list_add_btn(
			sysbar.edit_menu,
			btns[i].img,
			_(btns[i].text)
		);
		sysbar.edit_btns[i]=o;
		lv_obj_set_user_data(o,btns[i].key);
		lv_obj_set_event_cb(o,edit_menu_cb);
		lv_obj_set_style_local_outline_width(
			o,LV_BTN_PART_MAIN,
			LV_STATE_FOCUSED,0
		);
		lv_obj_set_style_local_bg_color(
			o,LV_BTN_PART_MAIN,
			LV_STATE_FOCUSED,
			lv_color_darken(
				LV_COLOR_WHITE,
				LV_OPA_10
			)
		);
		h+=lv_obj_get_height(o);
	}
	lv_obj_set_hidden(sysbar.edit_menu,true);
	lv_obj_set_size(sysbar.edit_menu,w,h);
	lv_obj_set_pos(
		sysbar.edit_menu,
		gui_w-w-gui_font_size,
		gui_h-sysbar.size-h-gui_font_size
	);

	lv_task_create(sysbar_thread_cb,5000,LV_TASK_PRIO_LOW,&sysbar);
	return 0;
}

void sysbar_set_full_screen(bool fs){
	if(fs==sysbar.full_screen)return;
	sysbar.full_screen=fs;
	lv_obj_set_hidden(sysbar.bar_btn,!fs);
	lv_obj_set_hidden(sysbar.top.bar,fs);
	lv_obj_set_hidden(sysbar.bottom.bar,fs);
	lv_obj_set_pos(sysbar.content,0,fs?0:sysbar.size);
	gui_sh=gui_h-(fs?0:sysbar.size*2);
	lv_obj_set_size(sysbar.content,gui_sw,gui_sh);
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
	lv_obj_set_hidden(sysbar.bar_btn,true);
}

void sysbar_show_bar(){
	if(!sysbar.full_screen)return;
	lv_group_add_obj(gui_grp,sysbar.bottom.content.back);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.home);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.keyboard);
	lv_group_add_obj(gui_grp,sysbar.bottom.content.power);
	lv_obj_set_hidden(sysbar.top.bar,false);
	lv_obj_set_hidden(sysbar.bottom.bar,false);
	if(sysbar.hide)lv_task_reset(sysbar.hide);
	else {
		sysbar.hide=lv_task_create(hide_cb,5000,LV_TASK_PRIO_MID,NULL);
		lv_task_once(sysbar.hide);
	}
}

#endif

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
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/filepicker.h"

const char*lv_event_string[]={
	[LV_EVENT_PRESSED]             = "LV_EVENT_PRESSED",
	[LV_EVENT_PRESSING]            = "LV_EVENT_PRESSING",
	[LV_EVENT_PRESS_LOST]          = "LV_EVENT_PRESS_LOST",
	[LV_EVENT_SHORT_CLICKED]       = "LV_EVENT_SHORT_CLICKED",
	[LV_EVENT_LONG_PRESSED]        = "LV_EVENT_LONG_PRESSED",
	[LV_EVENT_LONG_PRESSED_REPEAT] = "LV_EVENT_LONG_PRESSED_REPEAT",
	[LV_EVENT_CLICKED]             = "LV_EVENT_CLICKED",
	[LV_EVENT_RELEASED]            = "LV_EVENT_RELEASED",
	[LV_EVENT_DRAG_BEGIN]          = "LV_EVENT_DRAG_BEGIN",
	[LV_EVENT_DRAG_END]            = "LV_EVENT_DRAG_END",
	[LV_EVENT_DRAG_THROW_BEGIN]    = "LV_EVENT_DRAG_THROW_BEGIN",
	[LV_EVENT_GESTURE]             = "LV_EVENT_GESTURE",
	[LV_EVENT_KEY]                 = "LV_EVENT_KEY",
	[LV_EVENT_FOCUSED]             = "LV_EVENT_FOCUSED",
	[LV_EVENT_DEFOCUSED]           = "LV_EVENT_DEFOCUSED",
	[LV_EVENT_LEAVE]               = "LV_EVENT_LEAVE",
	[LV_EVENT_VALUE_CHANGED]       = "LV_EVENT_VALUE_CHANGED",
	[LV_EVENT_INSERT]              = "LV_EVENT_INSERT",
	[LV_EVENT_REFRESH]             = "LV_EVENT_REFRESH",
	[LV_EVENT_APPLY]               = "LV_EVENT_APPLY",
	[LV_EVENT_CANCEL]              = "LV_EVENT_CANCEL",
	[LV_EVENT_DELETE]              = "LV_EVENT_DELETE",
	[_LV_EVENT_LAST]               = "LV_EVENT_LAST"
};

lv_style_t*lv_style_opa_mask(){
	static lv_style_t bg;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&bg);
		lv_style_set_bg_opa(&bg,LV_STATE_DEFAULT,LV_OPA_50);
		lv_style_set_bg_color(&bg,LV_STATE_DEFAULT,LV_COLOR_BLACK);
		initialized=true;
	}
	return &bg;
}

lv_style_t*lv_style_btn_item(){
	static lv_style_t items;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&items);
		lv_style_set_radius(&items,LV_STATE_DEFAULT,5);
		lv_color_t click=lv_color_make(240,240,240),bg=lv_color_make(64,64,64);
		lv_style_set_border_width(&items,LV_STATE_DEFAULT,1);
		lv_style_set_border_width(&items,LV_STATE_CHECKED,0);
		lv_style_set_border_color(&items,LV_STATE_DEFAULT,click);
		lv_style_set_outline_width(&items,LV_STATE_DEFAULT,1);
		lv_style_set_outline_color(&items,LV_STATE_DEFAULT,bg);
		lv_style_set_bg_color(&items,LV_STATE_CHECKED,bg);
		initialized=true;
	}
	return &items;
}

void lv_style_set_action_button(lv_obj_t*btn,bool status){
	static lv_style_t btn_style;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&btn_style);
		lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
		lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);
		lv_style_set_text_color(&btn_style,LV_STATE_DISABLED,LV_COLOR_WHITE);
		initialized=true;
	}
	lv_obj_set_state(btn,LV_STATE_CHECKED);
	lv_obj_set_enabled(btn,status);
	lv_theme_apply(btn,LV_THEME_BTN);
	lv_obj_add_style(btn,LV_BTN_PART_MAIN,&btn_style);
}

void lv_style_set_btn_item(lv_obj_t*btn){
	lv_obj_add_style(btn,LV_BTN_PART_MAIN,lv_style_btn_item());
}

void lv_style_set_focus_checkbox(lv_obj_t*checkbox){
	static lv_style_t chk,bul;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&chk);
		lv_style_set_outline_width(&chk,LV_STATE_DEFAULT,0);
		lv_style_set_outline_width(&chk,LV_STATE_FOCUSED,0);
		lv_color_t pri_c=lv_theme_get_color_primary();
		lv_color_t bul_c=lv_color_lighten(pri_c,LV_OPA_80);
		lv_style_init(&bul);
		lv_style_set_bg_color(&bul,LV_STATE_FOCUSED,bul_c);
		lv_style_set_bg_color(&bul,LV_STATE_FOCUSED|LV_STATE_CHECKED,pri_c);
		initialized=true;
	}
	lv_obj_add_style(checkbox,LV_CHECKBOX_PART_BG,&chk);
	lv_obj_add_style(checkbox,LV_CHECKBOX_PART_BULLET,&bul);
}

void lv_style_set_focus_radiobox(lv_obj_t*checkbox){
	static lv_style_t chk,bul;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&chk);
		lv_style_set_outline_width(&chk,LV_STATE_DEFAULT,0);
		lv_style_set_outline_width(&chk,LV_STATE_FOCUSED,0);
		lv_color_t pri_c=lv_theme_get_color_primary();
		lv_color_t bul_c=lv_color_lighten(pri_c,LV_OPA_80);
		lv_style_init(&bul);
		lv_style_set_pattern_image(&bul,LV_STATE_CHECKED,NULL);
		lv_style_set_radius(&bul,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
		lv_style_set_bg_color(&bul,LV_STATE_FOCUSED,bul_c);
		lv_style_set_bg_color(&bul,LV_STATE_FOCUSED|LV_STATE_CHECKED,pri_c);
		initialized=true;
	}
	lv_obj_add_style(checkbox,LV_CHECKBOX_PART_BG,&chk);
	lv_obj_add_style(checkbox,LV_CHECKBOX_PART_BULLET,&bul);
}

void lv_style_set_outline(lv_obj_t*obj,uint8_t part){
	static lv_style_t outline;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&outline);
		lv_style_set_outline_width(&outline,LV_STATE_DEFAULT,1);
		lv_style_set_outline_color(&outline,LV_STATE_DEFAULT,gui_dark?LV_COLOR_WHITE:LV_COLOR_BLACK);
		initialized=true;
	}
	lv_obj_add_style(obj,part,&outline);
}

void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part){
	lv_obj_set_style_local_text_font(obj,part,LV_STATE_DEFAULT,gui_font_small);
}

void lv_obj_set_gray160_text_color(lv_obj_t*obj,uint8_t part){
	lv_obj_set_text_color_def_rgb(obj,part,160,160,160);
}

void lv_obj_set_gray240_text_color(lv_obj_t*obj,uint8_t part){
	lv_obj_set_text_color_def_rgb(obj,part,240,240,240);
}

lv_obj_t*lv_create_opa_mask(lv_obj_t*par){
	lv_obj_t*mask=lv_objmask_create(par,NULL);
	lv_obj_set_size(mask,gui_sw,gui_sh);
	lv_obj_add_style(mask,LV_OBJMASK_PART_MAIN,lv_style_opa_mask());
	return mask;
}

bool lv_page_is_top(lv_obj_t*page){
	if(!page)return false;
	lv_obj_t*s=lv_page_get_scrollable(page);
	lv_coord_t pt=lv_obj_get_style_pad_bottom(page,LV_PAGE_PART_BG);
	return lv_obj_get_y(s)>=pt;
}

bool lv_page_is_bottom(lv_obj_t*page){
	if(!page)return false;
	lv_obj_t*s=lv_page_get_scrollable(page);
	lv_coord_t pb=lv_obj_get_style_pad_bottom(page,LV_PAGE_PART_BG);
	return lv_obj_get_height(page)-lv_obj_get_y(s)-lv_obj_get_height(s)>=pb;
}

void lv_page_go_top(lv_obj_t*page){
	if(!page)return;
	lv_obj_t*s=lv_page_get_scrollable(page);
	lv_page_scroll_ver(page,-lv_obj_get_y(s));
}

void lv_page_go_bottom(lv_obj_t*page){
	if(!page)return;
	lv_obj_t*s=lv_page_get_scrollable(page);
	lv_page_scroll_ver(page,-lv_obj_get_height(s));
}

static void _lv_page_scroll_ver(lv_obj_t*obj,lv_coord_t dist,bool anim){
	if(anim)lv_page_scroll_ver(obj,dist);
	else{
		lv_obj_t*scrl=lv_page_get_scrl(obj);
		lv_obj_set_y(scrl,lv_obj_get_y(scrl)+dist);
	}
}

void lv_scroll_to(lv_obj_t*focus,bool anim){
	lv_obj_t*obj=focus;
	while((obj=lv_obj_get_parent(obj)))if(lv_debug_check_obj_type(obj,"lv_page")){
		lv_coord_t oy=lv_obj_get_rel_y(obj,focus),ox=lv_obj_get_rel_x(obj,focus);
		lv_coord_t ph=lv_obj_get_height(obj),pw=lv_obj_get_width(obj);
		lv_coord_t oh=lv_obj_get_height(focus),ow=lv_obj_get_width(focus);
		if(oy<0)_lv_page_scroll_ver(obj,-oy,anim);
		else if(oy+oh>ph)_lv_page_scroll_ver(obj,-oy+ph-oh,anim);
		if(ox<0)_lv_page_scroll_ver(obj,-ox,anim);
		else if(ox+ow>pw)_lv_page_scroll_ver(obj,-ox+pw-ow,anim);
	}
}

lv_coord_t lv_obj_get_rel_y(lv_obj_t*rel,lv_obj_t*obj){
	lv_coord_t c=0;
	do{c+=lv_obj_get_y(obj);}
	while((obj=lv_obj_get_parent(obj))&&obj!=rel);
	return c;
}

lv_coord_t lv_obj_get_rel_x(lv_obj_t*rel,lv_obj_t*obj){
	lv_coord_t c=0;
	do{c+=lv_obj_get_x(obj);}
	while((obj=lv_obj_get_parent(obj))&&obj!=rel);
	return c;
}

void lv_obj_set_enabled(lv_obj_t*obj,bool enabled){
	if(enabled)lv_obj_clear_state(obj,LV_STATE_DISABLED);
	else lv_obj_add_state(obj,LV_STATE_DISABLED);
}

void lv_obj_set_checked(lv_obj_t*obj,bool checked){
	if(checked)lv_obj_add_state(obj,LV_STATE_CHECKED);
	else lv_obj_clear_state(obj,LV_STATE_CHECKED);
}

void lv_default_dropdown_cb(lv_obj_t*obj,lv_event_t e __attribute__((unused))){
	lv_dropdown_ext_t*ex=lv_obj_get_ext_attr(obj);
	lv_group_set_editing(gui_grp,ex->page!=NULL);
}

void lv_textarea_remove_text(lv_obj_t*ta,uint32_t start,uint32_t len){
	if(len==0)return;
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(ta);
	char*txt=lv_label_get_text(ext->label);
	_lv_txt_cut(txt,start,len);
	lv_label_set_text(ext->label,txt);
	lv_textarea_clear_selection(ta);
	ext->sel_start=ext->sel_end=0;
	if(lv_obj_get_width(ext->label)==0){
		lv_style_int_t bw=lv_obj_get_style_border_width(ta,LV_TEXTAREA_PART_CURSOR);
		lv_obj_set_width(ext->label,bw==0?1:bw);
	}
	lv_event_send(ta,LV_EVENT_VALUE_CHANGED,NULL);
}

void lv_img_fill_image(lv_obj_t*img,lv_coord_t w,lv_coord_t h){
	lv_img_ext_t*x=lv_obj_get_ext_attr(img);
	if(x->w<=0||x->h<=0)return;
	int b=(int)(((float)w/x->w)*256);
	int a=(int)(((float)h/x->h)*256);
	lv_img_set_pivot(img,0,0);
	lv_img_set_zoom(img,MAX(a,b));
}

void lv_drag_border(lv_obj_t*obj,lv_coord_t width,lv_coord_t height,lv_coord_t border){
	lv_coord_t x=lv_obj_get_x(obj);
	lv_coord_t y=lv_obj_get_y(obj);
	lv_coord_t w=lv_obj_get_width(obj);
	lv_coord_t h=lv_obj_get_height(obj);
	if(x<border)x=border;
	if(y<border)y=border;
	if(x+w+border>width)x=width-w-border;
	if(y+h+border>height)y=height-h-border;
	lv_obj_set_pos(obj,x,y);
}

void lv_input_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	sysbar_focus_input(obj);
	sysbar_keyboard_open();
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	#ifdef ENABLE_UEFI
	lv_textarea_set_text(user_data,path[0]);
	#else
	lv_textarea_set_text(user_data,path[0]+2);
	#endif
	lv_event_send(user_data,LV_EVENT_DEFOCUSED,NULL);
	return false;
}

static void sel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct filepicker*fp=filepicker_create(select_cb,"Select item");
	filepicker_set_user_data(fp,lv_obj_get_user_data(obj));
	filepicker_set_max_item(fp,1);
}

static void clr_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	lv_textarea_set_text(lv_obj_get_user_data(obj),"");
}

static void inp_cb(lv_obj_t*obj,lv_event_t e){
	lv_event_cb_t cb=lv_obj_get_user_data(obj);
	if(cb)cb(obj,e);
	lv_input_cb(obj,e);
}

lv_obj_t*lv_draw_title(lv_obj_t*box,char*title,lv_coord_t*h){
	(*h)+=gui_font_size;
	lv_obj_t*label=lv_label_create(box,NULL);
	lv_label_set_text(label,_(title));
	lv_obj_set_y(label,(*h));
	return label;
}

lv_obj_t*lv_draw_side_clear_btn(lv_obj_t*box,lv_coord_t*h,lv_coord_t size){
	lv_obj_t*clr=lv_btn_create(box,NULL);
	lv_style_set_action_button(clr,true);
	lv_obj_set_event_cb(clr,clr_cb);
	lv_obj_set_style_local_radius(clr,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_obj_set_size(clr,size,size);
	lv_obj_align(clr,NULL,LV_ALIGN_IN_TOP_RIGHT,-gui_font_size/2,0);
	lv_obj_set_y(clr,(*h));
	lv_label_set_text(lv_label_create(clr,NULL),LV_SYMBOL_CLOSE);
	(*h)+=lv_obj_get_height(clr);
	return clr;
}

void lv_draw_file_input(
	lv_obj_t*box,
	char*title,
	lv_coord_t*h,
	lv_obj_t**clr,
	lv_obj_t**txt,
	lv_obj_t**btn
){
	lv_obj_t*label=lv_draw_title(box,title,h);
	lv_obj_t*c=lv_draw_side_clear_btn(box,h,lv_obj_get_height(label));

	(*h)+=gui_font_size/2;
	lv_obj_t*t=lv_textarea_create(box,NULL);
	lv_obj_set_user_data(c,t);
	lv_textarea_set_text(t,"");
	lv_textarea_set_one_line(t,true);
	lv_textarea_set_cursor_hidden(t,true);
	lv_obj_set_event_cb(t,inp_cb);
	lv_obj_set_y(t,(*h));

	lv_obj_t*b=lv_btn_create(box,NULL);
	lv_style_set_action_button(b,true);
	lv_obj_set_user_data(b,t);
	lv_obj_set_event_cb(b,sel_cb);
	lv_obj_set_style_local_radius(b,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size/2);
	lv_obj_set_size(b,gui_font_size*3,lv_obj_get_height(t));
	lv_obj_set_width(t,lv_page_get_scrl_width(box)-lv_obj_get_width(b)-gui_font_size/2);
	lv_obj_align(b,t,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/4,0);
	lv_label_set_text(lv_label_create(b,NULL),"...");
	(*h)+=lv_obj_get_height(b);
	if(clr)*clr=c;
	if(txt)*txt=t;
	if(btn)*btn=b;
}
#endif

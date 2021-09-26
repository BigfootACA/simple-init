#ifdef ENABLE_GUI
#include<stdio.h>
#include<stdlib.h>
#include"lvgl.h"
#include"gui.h"
#include"tools.h"
#include"defines.h"

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
	if(!status)lv_obj_add_state(btn,LV_STATE_DISABLED);
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

static lv_obj_t*_lv_create_msgbox(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,va_list va){
	char txt[BUFSIZ]={0};
	vsnprintf(txt,BUFSIZ-1,_(text),va);
	static lv_style_t btn;
	static bool initialized=false;
	if(!initialized){
		lv_style_init(&btn);
		lv_style_set_margin_all(&btn,LV_STATE_DEFAULT,0);
		lv_style_set_pad_all(&btn,LV_STATE_DEFAULT,gui_dpi/10);
		lv_style_set_radius(&btn,LV_STATE_DEFAULT,gui_dpi/10);
		initialized=true;
	}
	lv_obj_t*msg=lv_msgbox_create(par,NULL);
	lv_obj_set_click(msg,false);
	lv_obj_set_width(msg,gui_sw/6*5);
	lv_msgbox_set_text(msg,txt);
	if(btns)lv_msgbox_add_btns(msg,btns);
	lv_obj_add_style(msg,LV_MSGBOX_PART_BTN,&btn);
	if(cb)lv_obj_set_event_cb(msg,cb);
	lv_obj_align(msg,NULL,LV_ALIGN_CENTER,0,0);
	return msg;
}

#define use_lv_create_msgbox(obj,par,btns,cb,text) \
	va_list va; \
	va_start(va,text); \
	lv_obj_t*obj=_lv_create_msgbox(par,btns,cb,text,va); \
	va_end(va)

lv_obj_t*lv_create_msgbox(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...){
	use_lv_create_msgbox(o,par,btns,cb,text);
	return o;
}

lv_obj_t*lv_create_msgbox_mask(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...){
	use_lv_create_msgbox(o,lv_create_opa_mask(par),btns,cb,text);
	return o;
}

lv_obj_t*lv_create_ok_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...){
	static const char*btns[2];
	btns[0]=_("OK"),btns[1]="";
	use_lv_create_msgbox(o,par,btns,cb,text);
	return o;
}

lv_obj_t*lv_create_ok_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...){
	static const char*btns[2];
	btns[0]=_("OK"),btns[1]="";
	use_lv_create_msgbox(o,lv_create_opa_mask(par),btns,cb,text);
	return o;
}

lv_obj_t*lv_create_yesno_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...){
	static const char*btns[3];
	btns[0]=_("Yes"),btns[1]=_("No"),btns[2]="";
	use_lv_create_msgbox(o,par,btns,cb,text);
	return o;
}

lv_obj_t*lv_create_yesno_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...){
	static const char*btns[3];
	btns[0]=_("Yes"),btns[1]=_("No"),btns[2]="";
	use_lv_create_msgbox(o,lv_create_opa_mask(par),btns,cb,text);
	return o;
}

void lv_group_add_msgbox(lv_group_t*grp,lv_obj_t*page,bool focus){
	lv_obj_t*o=lv_obj_get_child(page,NULL);
	if(o)do{
		if(!lv_debug_check_obj_type(o,"lv_cont"))continue;
		lv_group_add_obj(grp,o);
		if(focus)lv_group_focus_obj(o);
		focus=false;
	}while((o=lv_obj_get_child(page,o)));
}

void lv_group_remove_msgbox(lv_obj_t*page){
	lv_obj_t*o=lv_obj_get_child(page,NULL);
	if(o)do{
		if(!lv_debug_check_obj_type(o,"lv_cont"))continue;
		lv_group_remove_obj(o);
	}while((o=lv_obj_get_child(page,o)));
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

#endif

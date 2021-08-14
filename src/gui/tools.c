#ifdef ENABLE_GUI
#include<stdio.h>
#include<stdlib.h>
#include<libintl.h>
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

lv_style_t*lv_obj_set_text_font(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_font_t*font){
	lv_style_t*f=malloc(sizeof(lv_style_t));
	if(!f)return NULL;
	lv_style_init(f);
	lv_style_set_text_font(f,state,font);
	if(obj)lv_obj_add_style(obj,part,f);
	return f;
}

lv_style_t*lv_obj_set_value_font(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_font_t*font){
	lv_style_t*f=malloc(sizeof(lv_style_t));
	if(!f)return NULL;
	lv_style_init(f);
	lv_style_set_value_font(f,state,font);
	if(obj)lv_obj_add_style(obj,part,f);
	return f;
}

lv_style_t*lv_obj_set_text_color(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_color_t color){
	lv_style_t*f=malloc(sizeof(lv_style_t));
	if(!f)return NULL;
	lv_style_init(f);
	lv_style_set_text_color(f,state,color);
	if(obj)lv_obj_add_style(obj,part,f);
	return f;
}

lv_style_t*lv_obj_set_bg_color(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_color_t color){
	lv_style_t*f=malloc(sizeof(lv_style_t));
	if(!f)return NULL;
	lv_style_init(f);
	lv_style_set_bg_color(f,state,color);
	if(obj)lv_obj_add_style(obj,part,f);
	return f;
}

void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part){
	static lv_style_t*f=NULL;
	if(!f)f=lv_obj_set_text_font(NULL,LV_STATE_DEFAULT,0,gui_font_small);
	lv_obj_add_style(obj,part,f);
}

void lv_obj_set_gray160_text_color(lv_obj_t*obj,uint8_t part){
	static lv_style_t*f=NULL;
	if(!f)f=lv_obj_set_text_color_def_rgb(NULL,0,160,160,160);
	lv_obj_add_style(obj,part,f);
}

void lv_obj_set_gray240_text_color(lv_obj_t*obj,uint8_t part){
	static lv_style_t*f=NULL;
	if(!f)f=lv_obj_set_text_color_def_rgb(NULL,0,240,240,240);
	lv_obj_add_style(obj,part,f);
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
#endif

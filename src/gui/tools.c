#include<stdio.h>
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

lv_obj_t*lv_create_opa_mask(lv_obj_t*par){
	lv_obj_t*mask=lv_objmask_create(par,NULL);
	lv_obj_set_size(mask,gui_sw,gui_sh);
	lv_obj_add_style(mask,LV_OBJMASK_PART_MAIN,lv_style_opa_mask());
	return mask;
}

static lv_obj_t*_lv_create_msgbox(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,va_list va){
	char txt[BUFSIZ]={0};
	vsnprintf(txt,BUFSIZ-1,_(text),va);
	lv_style_t btn;
	lv_style_init(&btn);
	lv_style_set_margin_all(&btn,LV_STATE_DEFAULT,0);
	lv_style_set_pad_all(&btn,LV_STATE_DEFAULT,gui_dpi/10);
	lv_style_set_radius(&btn,LV_STATE_DEFAULT,gui_dpi/10);
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

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"init_internal.h"
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"msgbox.h"
#include"language.h"
#define TAG "language"

static lv_obj_t*scr,*box,*sel,*btn_ok,*arr_left,*arr_right;

static void ok_action(lv_obj_t*obj,lv_event_t e){
	if(obj!=btn_ok||e!=LV_EVENT_CLICKED)return;
	uint16_t i=lv_dropdown_get_selected(sel);
	if(!languages[i].lang)return;
	const char*lang=lang_concat(&languages[i],true,true);
	tlog_debug("set language to %s",lang);
	lang_set(lang);
	#ifndef ENABLE_UEFI
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strcpy(msg.data.data,lang);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		guiact_do_back();
		msgbox_alert("init control command failed: %s",strerror(ex));
		return;
	}
	#endif
	guiact_do_back();
}

static void arrow_action(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	uint16_t cnt=lv_dropdown_get_option_cnt(sel);
	int16_t cur=lv_dropdown_get_selected(sel);
	if(obj==arr_left)cur--;
	else if(obj==arr_right)cur++;
	else return;
	if(cur<=0)cur=0;
	if(cur>=cnt-1)cur=cnt-1;
	lv_dropdown_set_selected(sel,(uint16_t)cur);
}

static void init_languages(){
	lv_dropdown_clear_options(sel);
	char*lang=lang_get_locale(NULL);
	uint16_t s=0;
	for(size_t i=0;languages[i].name;i++){
		struct language*l=&languages[i];
		lv_dropdown_add_option(sel,l->name,i);
		if(lang_compare(l,lang))s=i;
	}
	lv_dropdown_set_selected(sel,s);
}

static int language_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,sel);
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(sel);
	return 0;
}

static int language_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(sel);
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(arr_right);
	return 0;
}

static int language_menu_draw(struct gui_activity*act){
	int bts=gui_font_size+(gui_dpi/8);
	scr=act->page;

	static lv_style_t bs;
	lv_style_init(&bs);
	lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

	box=lv_page_create(scr,NULL);
	lv_obj_add_style(box,LV_PAGE_PART_BG,&bs);
	lv_obj_set_click(box,false);
	lv_obj_set_width(box,gui_sw/6*5);

	lv_obj_t*txt=lv_label_create(box,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
	lv_obj_set_width(txt,lv_page_get_scrl_width(box));
	lv_label_set_text(txt,_("Select language"));

	sel=lv_dropdown_create(box,NULL);
	lv_obj_set_width(sel,lv_page_get_scrl_width(box)-gui_dpi/10);
	lv_obj_align(sel,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	init_languages();

	btn_ok=lv_btn_create(box,NULL);
	lv_style_set_action_button(btn_ok,true);
	lv_obj_set_size(btn_ok,lv_page_get_scrl_width(box)-gui_dpi/10-bts*3,bts);
	lv_obj_align(btn_ok,sel,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	lv_obj_set_event_cb(btn_ok,ok_action);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	arr_left=lv_btn_create(box,NULL);
	lv_obj_set_size(arr_left,bts,bts);
	lv_obj_set_event_cb(arr_left,arrow_action);
	lv_obj_align(arr_left,btn_ok,LV_ALIGN_OUT_LEFT_MID,-bts/2,0);
	lv_style_set_action_button(arr_left,true);
	lv_label_set_text(lv_label_create(arr_left,NULL),LV_SYMBOL_LEFT);

	arr_right=lv_btn_create(box,NULL);
	lv_obj_set_size(arr_right,bts,bts);
	lv_obj_set_event_cb(arr_right,arrow_action);
	lv_obj_align(arr_right,btn_ok,LV_ALIGN_OUT_RIGHT_MID,bts/2,0);
	lv_style_set_action_button(arr_right,true);
	lv_label_set_text(lv_label_create(arr_right,NULL),LV_SYMBOL_RIGHT);

	lv_obj_set_height(box,lv_obj_get_y(btn_ok)+lv_obj_get_height(btn_ok)+(gui_font_size*2)+gui_dpi/20);
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

struct gui_register guireg_language={
	.name="language-menu",
	.title="Language",
	.icon="language.png",
	.show_app=true,
	.get_focus=language_get_focus,
	.lost_focus=language_lost_focus,
	.draw=language_menu_draw,
	.back=true,
	.mask=true,
};
#endif

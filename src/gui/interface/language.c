#ifdef ENABLE_GUI
#include<stdlib.h>
#include"init_internal.h"
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"language.h"
#define TAG "reboot"

static lv_obj_t*scr,*box,*sel;

static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}

static void ok_action(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	uint16_t i=lv_dropdown_get_selected(sel);
	if(!languages[i].lang)return;
	const char*lang=lang_concat(&languages[i],true,true);
	lang_set(lang);
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_LANGUAGE);
	strcpy(msg.data.data,lang);
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		lv_create_ok_msgbox(scr,ok_msg_click,_("init control command failed: %s"),strerror(ex));
		lv_obj_del_async(box);
		return;
	}
	guiact_do_back();
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

void language_menu_draw(lv_obj_t*screen){
	scr=lv_create_opa_mask(screen);

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

	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);

	lv_obj_t*btn=lv_btn_create(box,NULL);
	lv_obj_add_state(btn,LV_STATE_CHECKED);
	lv_obj_add_style(btn,LV_STATE_DEFAULT,&btn_style);
	lv_obj_set_width(btn,lv_page_get_scrl_width(box)-gui_dpi/10);
	lv_obj_align(btn,sel,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
	lv_obj_set_event_cb(btn,ok_action);
	lv_label_set_text(lv_label_create(btn,NULL),_("OK"));

	lv_obj_set_height(box,lv_obj_get_y(btn)+lv_obj_get_height(btn)+(gui_font_size*2)+gui_dpi/20);
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);

	guiact_register_activity(&(struct gui_activity){
		.name="language-menu",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=true,
		.page=scr
	});
}
#endif

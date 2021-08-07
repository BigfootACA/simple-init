#ifdef ENABLE_GUI
#include<libintl.h>
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"guidrv.h"
#define TAG "reboot"

static lv_obj_t*scr,*box,*value,*slider;

static void backlight_change(lv_obj_t*obj,lv_event_t e){
	if(obj!=slider||e!=LV_EVENT_VALUE_CHANGED)return;
	int16_t val=lv_slider_get_value(slider);
	if(val<=0)val=1;
	lv_label_set_text_fmt(value,"%d%%",val);
	guidrv_set_brightness(val);
}

static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}

void backlight_menu_draw(lv_obj_t*screen){
	scr=lv_create_opa_mask(screen);
	int level=guidrv_get_brightness();
	if(level<0)lv_create_ok_msgbox(scr,ok_msg_click,"get brightness failed: %m");
	else{
		static lv_style_t bs;
		lv_style_init(&bs);
		lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

		box=lv_obj_create(scr,NULL);
		lv_obj_add_style(box,LV_PAGE_PART_BG,&bs);
		lv_obj_set_click(box,false);
		lv_obj_set_width(box,gui_sw/6*5);

		lv_obj_t*txt=lv_label_create(box,NULL);
		lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
		lv_obj_set_width(txt,lv_obj_get_width(box));
		lv_label_set_align(txt,LV_LABEL_ALIGN_CENTER);
		lv_obj_align(txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_font_size);
		lv_label_set_text(txt,_("Change backlight level"));

		value=lv_label_create(box,NULL);
		lv_label_set_text_fmt(value,"%d%%",level);
		lv_obj_align(value,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);

		slider=lv_slider_create(box,NULL);
		lv_slider_set_range(slider,0,100);
		lv_slider_set_value(slider,level,LV_ANIM_OFF);
		lv_obj_set_event_cb(slider,backlight_change);
		lv_obj_align(slider,value,LV_ALIGN_OUT_BOTTOM_MID,gui_font_size,gui_font_size);
		lv_obj_set_width(slider,lv_obj_get_width(box)-gui_font_size*4);

		lv_obj_set_height(box,lv_obj_get_y(slider)+lv_obj_get_height(slider)+(gui_font_size*2));
		lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	}

	guiact_register_activity(&(struct gui_activity){
		.name="backlight-menu",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=true,
		.page=scr
	});
}
#endif

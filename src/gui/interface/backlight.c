#ifdef ENABLE_GUI
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"guidrv.h"
#define TAG "backlight"

static lv_obj_t*box,*value,*slider,*arr_left,*arr_right;

static void backlight_change(lv_obj_t*obj,lv_event_t e){
	if(obj!=slider||e!=LV_EVENT_VALUE_CHANGED)return;
	int16_t val=lv_slider_get_value(slider);
	if(val<=0)val=1;
	lv_label_set_text_fmt(value,"%d%%",val);
	guidrv_set_brightness(val);
}

static void backlight_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	int16_t val=lv_slider_get_value(slider);
	if(obj==arr_left)val--;
	else if(obj==arr_right)val++;
	else return;
	if(val<=1)val=1;
	if(val>=100)val=100;
	lv_label_set_text_fmt(value,"%d%%",val);
	lv_slider_set_value(slider,val,LV_ANIM_ON);
	guidrv_set_brightness(val);
}

static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}

static int backlight_menu_draw(struct gui_activity*act){
	int bts=gui_font_size+(gui_dpi/8);
	int level=guidrv_get_brightness();
	if(level<0)lv_create_ok_msgbox(act->page,ok_msg_click,_("get brightness failed: %m"));
	else{
		static lv_style_t bs;
		lv_style_init(&bs);
		lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

		box=lv_obj_create(act->page,NULL);
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

		static lv_style_t btn_style;
		lv_style_init(&btn_style);
		lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
		lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);

		arr_left=lv_btn_create(box,NULL);
		lv_obj_set_size(arr_left,bts,bts);
		lv_obj_set_event_cb(arr_left,backlight_click);
		lv_obj_align(arr_left,slider,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
		lv_obj_add_style(arr_left,LV_BTN_PART_MAIN,&btn_style);
		lv_obj_add_state(arr_left,LV_STATE_CHECKED);
		lv_label_set_text(lv_label_create(arr_left,NULL),LV_SYMBOL_LEFT);

		arr_right=lv_btn_create(box,NULL);
		lv_obj_set_size(arr_right,bts,bts);
		lv_obj_set_event_cb(arr_right,backlight_click);
		lv_obj_align(arr_right,slider,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
		lv_obj_add_style(arr_right,LV_BTN_PART_MAIN,&btn_style);
		lv_obj_add_state(arr_right,LV_STATE_CHECKED);
		lv_label_set_text(lv_label_create(arr_right,NULL),LV_SYMBOL_RIGHT);

		lv_obj_set_height(box,
			lv_obj_get_y(slider)+
			lv_obj_get_height(slider)+
			(gui_font_size*2)+
			bts
		);
		lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	}
	return 0;
}

static int backlight_get_focus(struct gui_activity*d __attribute__((unused))){
	if(guidrv_get_brightness()<0){
		lv_group_add_msgbox(gui_grp,d->page,true);
		return 0;
	}
	lv_group_add_obj(gui_grp,slider);
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(slider);
	return 0;
}

static int backlight_lost_focus(struct gui_activity*d __attribute__((unused))){
	if(guidrv_get_brightness()<0){
		lv_group_remove_msgbox(d->page);
		return 0;
	}
	lv_group_remove_obj(slider);
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(arr_right);
	return 0;
}

struct gui_register guireg_backlight={
	.name="backlight-menu",
	.title="Backlight",
	.icon="backlight.png",
	.show_app=true,
	.draw=backlight_menu_draw,
	.lost_focus=backlight_lost_focus,
	.get_focus=backlight_get_focus,
	.back=true,
	.mask=true,
};
#endif

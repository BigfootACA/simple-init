#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"tools.h"
#include"guidrv.h"
#define TAG "mouse"

extern int gui_mouse_scale;
static lv_obj_t*box,*value,*slider,*arr_left,*arr_right;

static void mouse_scale_change(lv_obj_t*obj,lv_event_t e){
	if(obj!=slider||e!=LV_EVENT_VALUE_CHANGED)return;
	gui_mouse_scale=lv_slider_get_value(slider);
	if(gui_mouse_scale<=0)gui_mouse_scale=1;
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	guidrv_set_brightness(gui_mouse_scale);
}

static void mouse_scale_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	gui_mouse_scale=lv_slider_get_value(slider);
	if(obj==arr_left)gui_mouse_scale--;
	else if(obj==arr_right)gui_mouse_scale++;
	else return;
	if(gui_mouse_scale<=0)gui_mouse_scale=1;
	if(gui_mouse_scale>=64)gui_mouse_scale=64;
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	lv_slider_set_value(slider,gui_mouse_scale,LV_ANIM_ON);
}

static int mouse_menu_draw(struct gui_activity*act){
	int bts=gui_font_size+(gui_dpi/8);

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
	lv_label_set_text(txt,_("Change mouse scale level"));

	value=lv_label_create(box,NULL);
	lv_label_set_text_fmt(value,"%d",gui_mouse_scale);
	lv_obj_align(value,txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);

	slider=lv_slider_create(box,NULL);
	lv_slider_set_range(slider,1,64);
	lv_slider_set_value(slider,gui_mouse_scale,LV_ANIM_OFF);
	lv_obj_set_event_cb(slider,mouse_scale_change);
	lv_obj_set_width(slider,lv_obj_get_width(box)-gui_font_size*4);
	lv_obj_align(slider,value,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);

	arr_left=lv_btn_create(box,NULL);
	lv_obj_set_size(arr_left,bts,bts);
	lv_obj_set_event_cb(arr_left,mouse_scale_click);
	lv_obj_align(arr_left,slider,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
	lv_style_set_action_button(arr_left,true);
	lv_label_set_text(lv_label_create(arr_left,NULL),LV_SYMBOL_LEFT);

	arr_right=lv_btn_create(box,NULL);
	lv_obj_set_size(arr_right,bts,bts);
	lv_obj_set_event_cb(arr_right,mouse_scale_click);
	lv_obj_align(arr_right,slider,LV_ALIGN_OUT_BOTTOM_RIGHT,0,gui_font_size);
	lv_style_set_action_button(arr_right,true);
	lv_label_set_text(lv_label_create(arr_right,NULL),LV_SYMBOL_RIGHT);

	lv_obj_set_height(box,
		lv_obj_get_y(slider)+
		lv_obj_get_height(slider)+
		(gui_font_size*2)+
		bts
	);
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int mouse_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,slider);
	lv_group_add_obj(gui_grp,arr_left);
	lv_group_add_obj(gui_grp,arr_right);
	lv_group_focus_obj(slider);
	return 0;
}

static int mouse_lost_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_remove_obj(slider);
	lv_group_remove_obj(arr_left);
	lv_group_remove_obj(arr_right);
	return 0;
}

struct gui_register guireg_mouse_menu={
	.name="mouse-tools-menu",
	.title="Mouse Tools",
	.icon="mouse.png",
	.show_app=true,
	.draw=mouse_menu_draw,
	.lost_focus=mouse_lost_focus,
	.get_focus=mouse_get_focus,
	.back=true,
	.mask=true,
};
#endif
#endif

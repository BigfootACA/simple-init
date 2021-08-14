#ifndef TOOLS_H
#define TOOLS_H
extern lv_style_t*lv_style_opa_mask();
extern lv_style_t*lv_style_btn_item();
extern void lv_style_set_btn_item(lv_obj_t*btn);
extern void lv_style_set_focus_checkbox(lv_obj_t*checkbox);
extern lv_style_t*lv_obj_set_text_font(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_font_t*font);
extern lv_style_t*lv_obj_set_value_font(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_font_t*font);
extern lv_style_t*lv_obj_set_text_color(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_color_t color);
extern lv_style_t*lv_obj_set_bg_color(lv_obj_t*obj,lv_state_t state,uint8_t part,lv_color_t color);
extern void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_gray160_text_color(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_gray240_text_color(lv_obj_t*obj,uint8_t part);
extern lv_obj_t*lv_create_opa_mask(lv_obj_t*par);
extern lv_obj_t*lv_create_msgbox(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_msgbox_mask(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_ok_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_ok_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_yesno_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_yesno_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);

#define lv_obj_set_text_font_def(obj,part,font) lv_obj_set_text_font(obj,LV_STATE_DEFAULT,part,font)
#define lv_obj_set_bg_color_def(obj,part,color) lv_obj_set_bg_color(obj,LV_STATE_DEFAULT,part,color)
#define lv_obj_set_bg_color_rgb(obj,state,part,r,g,b) lv_obj_set_bg_color(obj,state,part,lv_color_make(r,g,b))
#define lv_obj_set_bg_color_def_rgb(obj,part,r,g,b) lv_obj_set_bg_color_def(obj,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def(obj,part,color) lv_obj_set_text_color(obj,LV_STATE_DEFAULT,part,color)
#define lv_obj_set_text_color_rgb(obj,state,part,r,g,b) lv_obj_set_text_color(obj,state,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def_rgb(obj,part,r,g,b) lv_obj_set_text_color_def(obj,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def_rgb_lbl(obj,r,g,b) lv_obj_set_text_color_def_rgb(obj,LV_LABEL_PART_MAIN,r,g,b)
#define lv_obj_set_text_color_rgb_lbl(obj,state,r,g,b) lv_obj_set_text_color_rgb(obj,state,LV_LABEL_PART_MAIN,r,g,b)
#define lv_obj_set_text_color_lbl(obj,state,color) lv_obj_set_text_color(obj,state,LV_LABEL_PART_MAIN,color)
#endif

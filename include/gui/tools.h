#ifndef _TOOLS_H
#define _TOOLS_H
#include"gui.h"
extern lv_style_t*lv_style_opa_mask();
extern lv_style_t*lv_style_btn_item();
extern void lv_style_set_btn_item(lv_obj_t*btn);
extern void lv_style_set_focus_checkbox(lv_obj_t*checkbox);
extern void lv_style_set_action_button(lv_obj_t*btn,bool status);
extern void lv_style_set_outline(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_small_text_font(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_gray160_text_color(lv_obj_t*obj,uint8_t part);
extern void lv_obj_set_gray240_text_color(lv_obj_t*obj,uint8_t part);
extern lv_obj_t*lv_create_opa_mask(lv_obj_t*par);
extern bool lv_page_is_top(lv_obj_t*page);
extern bool lv_page_is_bottom(lv_obj_t*page);
extern void lv_page_go_top(lv_obj_t*page);
extern void lv_page_go_bottom(lv_obj_t*page);
extern lv_coord_t lv_obj_get_rel_y(lv_obj_t*rel,lv_obj_t*obj);
extern lv_coord_t lv_obj_get_rel_x(lv_obj_t*rel,lv_obj_t*obj);

#define lv_obj_set_text_font_def(obj,part,font) lv_obj_set_text_font(obj,LV_STATE_DEFAULT,part,font)
#define lv_obj_set_bg_color_def(obj,part,color) lv_obj_set_bg_color(obj,LV_STATE_DEFAULT,part,color)
#define lv_obj_set_bg_color_rgb(obj,state,part,r,g,b) lv_obj_set_bg_color(obj,state,part,lv_color_make(r,g,b))
#define lv_obj_set_bg_color_def_rgb(obj,part,r,g,b) lv_obj_set_bg_color_def(obj,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def(obj,part,color) lv_obj_set_style_local_text_color(obj,LV_STATE_DEFAULT,part,color)
#define lv_obj_set_text_color_rgb(obj,state,part,r,g,b) lv_obj_set_text_color(obj,state,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def_rgb(obj,part,r,g,b) lv_obj_set_text_color_def(obj,part,lv_color_make(r,g,b))
#define lv_obj_set_text_color_def_rgb_lbl(obj,r,g,b) lv_obj_set_text_color_def_rgb(obj,LV_LABEL_PART_MAIN,r,g,b)
#define lv_obj_set_text_color_rgb_lbl(obj,state,r,g,b) lv_obj_set_text_color_rgb(obj,state,LV_LABEL_PART_MAIN,r,g,b)
#define lv_obj_set_text_color_lbl(obj,state,color) lv_obj_set_text_color(obj,state,LV_LABEL_PART_MAIN,color)
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _GUI_STRING_H
#define _GUI_STRING_H
#include"gui.h"
#define DECL_STR_CONV(_name,_type)\
	extern const char*lv_##_name##_to_string(const _type);\
	extern const char*lv_##_name##_to_name(const _type);\
	extern bool lv_string_to_##_name(const char*string,_type*);\
	extern bool lv_name_to_##_name(const char*name,_type*);
DECL_STR_CONV(event_code,lv_event_code_t)
DECL_STR_CONV(flag,lv_obj_flag_t)
DECL_STR_CONV(state,lv_state_t)
DECL_STR_CONV(part,lv_part_t)
DECL_STR_CONV(style_prop,lv_style_prop_t)
DECL_STR_CONV(blend_mode,lv_blend_mode_t)
DECL_STR_CONV(border_side,lv_border_side_t)
DECL_STR_CONV(dither_mode,lv_dither_mode_t)
DECL_STR_CONV(grad_dir,lv_grad_dir_t)
DECL_STR_CONV(text_decor,lv_text_decor_t)
DECL_STR_CONV(label_long_mode,lv_label_long_mode_t)
DECL_STR_CONV(arc_mode,lv_arc_mode_t)
DECL_STR_CONV(bar_mode,lv_bar_mode_t)
DECL_STR_CONV(btnmatrix_ctrl,lv_btnmatrix_ctrl_t)
DECL_STR_CONV(img_size_mode,lv_img_size_mode_t)
DECL_STR_CONV(roller_mode,lv_roller_mode_t)
DECL_STR_CONV(slider_mode,lv_slider_mode_t)
DECL_STR_CONV(table_cell_ctrl,lv_table_cell_ctrl_t)
DECL_STR_CONV(scr_load_anim,lv_scr_load_anim_t)
DECL_STR_CONV(key,lv_key_t)
DECL_STR_CONV(flex_flow,lv_flex_flow_t)
DECL_STR_CONV(flex_align,lv_flex_align_t)
DECL_STR_CONV(grid_align,lv_grid_align_t)
DECL_STR_CONV(chart_axis,lv_chart_axis_t)
DECL_STR_CONV(chart_type,lv_chart_type_t)
DECL_STR_CONV(chart_update_mode,lv_chart_update_mode_t)
DECL_STR_CONV(colorwheel_mode,lv_colorwheel_mode_t)
DECL_STR_CONV(imgbtn_state,lv_imgbtn_state_t)
DECL_STR_CONV(keyboard_mode,lv_keyboard_mode_t)
DECL_STR_CONV(menu_mode_header,lv_menu_mode_header_t)
DECL_STR_CONV(align,lv_align_t)
DECL_STR_CONV(dir,lv_dir_t)
DECL_STR_CONV(base_dir,lv_base_dir_t)
DECL_STR_CONV(opa,lv_opa_t)
DECL_STR_CONV(palette,lv_palette_t)
DECL_STR_CONV(text_align,lv_text_align_t)
DECL_STR_CONV(text_cmd_state,lv_text_cmd_state_t)
DECL_STR_CONV(text_flag,lv_text_flag_t)
#endif

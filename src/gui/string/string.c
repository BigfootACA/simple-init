/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"array.h"
#include"enum_conv.h"
#include"gui/tools.h"

const char*lv_event_string[]={
	#define STR(_type,_str) [(_type)] = (#_type),
	#include"event.lst"
	#undef STR
};

#define NAME event_string
#define CODE event_code
#define TYPE lv_event_code_t
#define TARGET "event.lst"
#include"template.h"

#define NAME obj_flag_string
#define CODE flag
#define TYPE lv_obj_flag_t
#define TARGET "obj_flag.lst"
#include"template.h"

#define NAME state_string
#define CODE state
#define TYPE lv_state_t
#define TARGET "state.lst"
#include"template.h"

#define NAME part_string
#define CODE part
#define TYPE lv_part_t
#define TARGET "part.lst"
#include"template.h"

#define NAME style_prop_string
#define CODE style_prop
#define TYPE lv_style_prop_t
#define TARGET "style_prop.lst"
#include"template.h"

#define NAME blend_mode_string
#define CODE blend_mode
#define TYPE lv_blend_mode_t
#define TARGET "blend_mode.lst"
#include"template.h"

#define NAME border_side_string
#define CODE border_side
#define TYPE lv_border_side_t
#define TARGET "border_side.lst"
#include"template.h"

#define NAME dither_mode_string
#define CODE dither_mode
#define TYPE lv_dither_mode_t
#define TARGET "dither_mode.lst"
#include"template.h"

#define NAME grad_dir_string
#define CODE grad_dir
#define TYPE lv_grad_dir_t
#define TARGET "grad_dir.lst"
#include"template.h"

#define NAME text_decor_string
#define CODE text_decor
#define TYPE lv_text_decor_t
#define TARGET "text_decor.lst"
#include"template.h"

#define NAME label_long_mode_string
#define CODE label_long_mode
#define TYPE lv_label_long_mode_t
#define TARGET "label_long.lst"
#include"template.h"

#define NAME arc_mode_string
#define CODE arc_mode
#define TYPE lv_arc_mode_t
#define TARGET "arc_mode.lst"
#include"template.h"

#define NAME bar_mode_string
#define CODE bar_mode
#define TYPE lv_bar_mode_t
#define TARGET "bar_mode.lst"
#include"template.h"

#define NAME btnmatrix_ctrl_string
#define CODE btnmatrix_ctrl
#define TYPE lv_btnmatrix_ctrl_t
#define TARGET "btnmatrix_ctrl.lst"
#include"template.h"

#define NAME img_size_mode_string
#define CODE img_size_mode
#define TYPE lv_img_size_mode_t
#define TARGET "img_size_mode.lst"
#include"template.h"

#define NAME roller_mode_string
#define CODE roller_mode
#define TYPE lv_roller_mode_t
#define TARGET "roller_mode.lst"
#include"template.h"

#define NAME slider_mode_string
#define CODE slider_mode
#define TYPE lv_slider_mode_t
#define TARGET "slider_mode.lst"
#include"template.h"

#define NAME table_cell_ctrl_string
#define CODE table_cell_ctrl
#define TYPE lv_table_cell_ctrl_t
#define TARGET "table_cell_ctrl.lst"
#include"template.h"

#define NAME scr_load_anim_string
#define CODE scr_load_anim
#define TYPE lv_scr_load_anim_t
#define TARGET "scr_load_anim.lst"
#include"template.h"

#define NAME key_string
#define CODE key
#define TYPE lv_key_t
#define TARGET "key.lst"
#include"template.h"

#define NAME flex_flow_string
#define CODE flex_flow
#define TYPE lv_flex_flow_t
#define TARGET "flex_flow.lst"
#include"template.h"

#define NAME flex_align_string
#define CODE flex_align
#define TYPE lv_flex_align_t
#define TARGET "flex_align.lst"
#include"template.h"

#define NAME grid_align_string
#define CODE grid_align
#define TYPE lv_grid_align_t
#define TARGET "grid_align.lst"
#include"template.h"

#define NAME chart_axis_string
#define CODE chart_axis
#define TYPE lv_chart_axis_t
#define TARGET "chart_axis.lst"
#include"template.h"

#define NAME chart_type_string
#define CODE chart_type
#define TYPE lv_chart_type_t
#define TARGET "chart_type.lst"
#include"template.h"

#define NAME chart_update_mode_string
#define CODE chart_update_mode
#define TYPE lv_chart_update_mode_t
#define TARGET "chart_update_mode.lst"
#include"template.h"

#define NAME colorwheel_mode_string
#define CODE colorwheel_mode
#define TYPE lv_colorwheel_mode_t
#define TARGET "colorwheel_mode.lst"
#include"template.h"

#define NAME imgbtn_state_string
#define CODE imgbtn_state
#define TYPE lv_imgbtn_state_t
#define TARGET "imgbtn_state.lst"
#include"template.h"

#define NAME keyboard_mode_string
#define CODE keyboard_mode
#define TYPE lv_keyboard_mode_t
#define TARGET "keyboard_mode.lst"
#include"template.h"

#define NAME menu_mode_header_string
#define CODE menu_mode_header
#define TYPE lv_menu_mode_header_t
#define TARGET "menu_mode_header.lst"
#include"template.h"

#define NAME align_string
#define CODE align
#define TYPE lv_align_t
#define TARGET "align.lst"
#include"template.h"

#define NAME dir_string
#define CODE dir
#define TYPE lv_dir_t
#define TARGET "dir.lst"
#include"template.h"

#define NAME base_dir_string
#define CODE base_dir
#define TYPE lv_base_dir_t
#define TARGET "base_dir.lst"
#include"template.h"

#define NAME opa_string
#define CODE opa
#define TYPE lv_opa_t
#define TARGET "opa.lst"
#include"template.h"

#define NAME palette_string
#define CODE palette
#define TYPE lv_palette_t
#define TARGET "palette.lst"
#include"template.h"

#define NAME text_align_string
#define CODE text_align
#define TYPE lv_text_align_t
#define TARGET "text_align.lst"
#include"template.h"

#define NAME text_cmd_state_string
#define CODE text_cmd_state
#define TYPE lv_text_cmd_state_t
#define TARGET "text_cmd_state.lst"
#include"template.h"

#define NAME text_flag_string
#define CODE text_flag
#define TYPE lv_text_flag_t
#define TARGET "text_flag.lst"
#include"template.h"

#endif

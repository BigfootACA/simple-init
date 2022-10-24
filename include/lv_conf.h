/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef LV_CONF_H
#define LV_CONF_H
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
extern int gui_dpi;
extern bool gui_dark;
extern void*lv_mem_pool;
extern size_t lv_mem_pool_size;
extern uint32_t custom_tick_get(void);
#define LV_COLOR_DEPTH                      32
#define LV_COLOR_16_SWAP                    0
#define LV_COLOR_SCREEN_TRANSP              1
#define LV_COLOR_MIX_ROUND_OFS              0
#define LV_COLOR_CHROMA_KEY                 lv_color_hex(0x00ff00)
#define LV_COLOR_TRANSP                     LV_COLOR_LIME
#define LV_INDEXED_CHROMA                   1
#define LV_ANTIALIAS                        1
#define LV_DISP_DEF_REFR_PERIOD             8
#define LV_DISP_ROT_MAX_BUF                 1048576
#define LV_DPI                              gui_dpi
#define LV_DRAW_COMPLEX                     1
#define LV_SHADOW_CACHE_SIZE                0
#define LV_CIRCLE_CACHE_SIZE                4
#define LV_DISP_SMALL_LIMIT                 30
#define LV_DISP_MEDIUM_LIMIT                50
#define LV_DISP_LARGE_LIMIT                 70
#define LV_MEM_CUSTOM                       0
#define LV_MEM_SIZE                         lv_mem_pool_size
#define LV_MEM_ADR                          lv_mem_pool
#define LV_MEM_CUSTOM_INCLUDE               <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC                 malloc
#define LV_MEM_CUSTOM_FREE                  free
#define LV_USE_MEM_MONITOR                  0
#define LV_ENABLE_GC                        0
#define LV_INDEV_DEF_READ_PERIOD            15
#define LV_INDEV_DEF_DRAG_LIMIT             10
#define LV_INDEV_DEF_DRAG_THROW             10
#define LV_INDEV_DEF_LONG_PRESS_TIME        400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME    100
#define LV_INDEV_DEF_GESTURE_LIMIT          50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY   3
#define LV_USE_ANIMATION                    1
#define LV_USE_SHADOW                       1
#define LV_SHADOW_CACHE_SIZE                0
#define LV_USE_BLEND_MODES                  1
#define LV_USE_OPA_SCALE                    1
#define LV_USE_GROUP                        1
#define LV_USE_GPU                          1
#define LV_USE_GPU_STM32_DMA2D              0
#ifdef ENABLE_SDL2
#define LV_USE_GPU_SDL                      1
#endif
#define LV_USE_FILESYSTEM                   1
#define LV_USE_USER_DATA                    1
#define LV_USE_PERF_MONITOR                 0
#define LV_IMG_CF_INDEXED                   1
#define LV_IMG_CF_ALPHA                     1
#define LV_IMG_CACHE_DEF_SIZE               0
#define LV_GRADIENT_MAX_STOPS               2
#define LV_GRAD_CACHE_DEF_SIZE              0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TASK_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value)      struct _silence_gcc_warning
#define LV_TICK_CUSTOM                      1
#define LV_TICK_CUSTOM_INCLUDE              <stdint.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR        (custom_tick_get())
#define LV_DEBUG_ASSERT(expr,msg,value) {if(!(expr)){\
	LV_LOG_ERROR(__func__);\
	lv_debug_log_error(msg,(uint64_t)((uintptr_t)value));\
	abort();while(1);\
}}
#define LV_USE_LOG                          1
#define LV_LOG_LEVEL                        LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF                       0
#define LV_LOG_TRACE_MEM                    0
#define LV_LOG_TRACE_TIMER                  1
#define LV_LOG_TRACE_INDEV                  1
#define LV_LOG_TRACE_DISP_REFR              1
#define LV_LOG_TRACE_EVENT                  0
#define LV_LOG_TRACE_OBJ_CREATE             0
#define LV_LOG_TRACE_LAYOUT                 1
#define LV_LOG_TRACE_ANIM                   1
#define LV_FONT_MONTSERRAT_12               0
#define LV_FONT_MONTSERRAT_14               0
#define LV_FONT_MONTSERRAT_16               0
#define LV_FONT_MONTSERRAT_18               0
#define LV_FONT_MONTSERRAT_20               0
#define LV_FONT_MONTSERRAT_22               0
#define LV_FONT_MONTSERRAT_24               0
#define LV_FONT_MONTSERRAT_26               0
#define LV_FONT_MONTSERRAT_28               0
#define LV_FONT_MONTSERRAT_30               0
#define LV_FONT_MONTSERRAT_32               0
#define LV_FONT_MONTSERRAT_34               0
#define LV_FONT_MONTSERRAT_36               0
#define LV_FONT_MONTSERRAT_38               0
#define LV_FONT_MONTSERRAT_40               0
#define LV_FONT_MONTSERRAT_42               0
#define LV_FONT_MONTSERRAT_44               0
#define LV_FONT_MONTSERRAT_46               0
#define LV_FONT_MONTSERRAT_48               0
#define LV_FONT_MONTSERRAT_12_SUBPX         0
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0
#define LV_FONT_SIMSUN_16_CJK               0
#define LV_FONT_UNSCII_8                    0
#define LV_FONT_CUSTOM_DECLARE \
	LV_FONT_DECLARE(*gui_font) \
	LV_FONT_DECLARE(*gui_font_small) \
	LV_FONT_DECLARE(*symbol_font)
#define LV_FONT_DEFAULT gui_font
#define LV_FONT_FMT_TXT_LARGE               1
#define LV_FONT_SUBPX_BGR                   0
#define LV_USE_THEME_EMPTY                  1
#define LV_USE_THEME_BASIC                  1
#define LV_USE_THEME_TEMPLATE               1
#define LV_USE_THEME_MATERIAL               1
#define LV_USE_THEME_MONO                   1
#define LV_THEME_DEFAULT_INCLUDE            <stdint.h>
#define LV_THEME_DEFAULT_INIT               lv_theme_material_init
#define LV_THEME_DEFAULT_COLOR_PRIMARY      LV_PALETTE_RED
#define LV_THEME_DEFAULT_COLOR_SECONDARY    LV_PALETTE_BLUE
#define LV_THEME_DEFAULT_FLAG               LV_THEME_MATERIAL_FLAG_LIGHT
#define LV_THEME_DEFAULT_FONT_SMALL         gui_font_small
#define LV_THEME_DEFAULT_FONT_NORMAL        LV_FONT_DEFAULT
#define LV_THEME_DEFAULT_FONT_SUBTITLE      LV_FONT_DEFAULT
#define LV_THEME_DEFAULT_FONT_TITLE         LV_FONT_DEFAULT
#define LV_TXT_ENC                          LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS                  " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN          0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD                    "#"
#define LV_USE_BIDI                         0
#define LV_USE_ARABIC_PERSIAN_CHARS         0
#define LV_SPRINTF_CUSTOM                   0
#define LV_USE_OBJ_REALIGN                  1
#define LV_USE_EXT_CLICK_AREA               LV_EXT_CLICK_AREA_TINY
#define LV_USE_ARC                          1
#define LV_USE_ANIMIMG                      1
#define LV_USE_BAR                          1
#define LV_USE_BTN                          1
#define LV_USE_BTNMATRIX                    1
#define LV_USE_CANVAS                       1
#define LV_USE_CHECKBOX                     1
#define LV_USE_DROPDOWN                     1
#define LV_USE_IMG                          1
#define LV_USE_LABEL                        1
#define LV_LABEL_TEXT_SELECTION             1
#define LV_USE_LINE                         1
#define LV_USE_ROLLER                       1
#define LV_ROLLER_INF_PAGES                 7
#define LV_USE_SLIDER                       1
#define LV_USE_SWITCH                       1
#define LV_USE_TEXTAREA                     1
#define LV_USE_TABLE                        1
#define LV_USE_CALENDAR                     1
#define LV_USE_CHART                        1
#define LV_USE_COLORWHEEL                   1
#define LV_USE_IMGBTN                       1
#define LV_USE_KEYBOARD                     1
#define LV_USE_LED                          1
#define LV_USE_LIST                         1
#define LV_USE_MENU                         1
#define LV_USE_METER                        1
#define LV_USE_MSGBOX                       1
#define LV_USE_SPINBOX                      1
#define LV_USE_SPINNER                      1
#define LV_USE_TABVIEW                      1
#define LV_USE_TILEVIEW                     1
#define LV_USE_WIN                          1
#define LV_USE_SPAN                         1
#define LV_SPAN_SNIPPET_STACK_SIZE          64
#define LV_USE_THEME_DEFAULT                1
#define LV_THEME_DEFAULT_DARK               gui_dark
#define LV_THEME_DEFAULT_GROW               0
#define LV_THEME_DEFAULT_TRANSITION_TIME    80
#define LV_USE_FLEX                         1
#define LV_USE_GRID                         1
#define LV_USE_ASSERT_NULL                  1
#define LV_USE_ASSERT_MALLOC                1
#define LV_USE_ASSERT_STYLE                 0
#define LV_USE_ASSERT_MEM_INTEGRITY         0
#define LV_USE_ASSERT_OBJ                   0
#define LV_ASSERT_HANDLER                   abort();
#define LV_USE_LARGE_COORD                  1
#define LV_CHART_AXIS_TICK_LABEL_MAX_LEN    256
#define LV_DROPDOWN_DEF_ANIM_TIME           200
#define LV_IMGBTN_TILED                     0
#define LV_LED_BRIGHT_MIN                   120
#define LV_LED_BRIGHT_MAX                   255
#define LV_LABEL_WAIT_CHAR_COUNT            3
#define LV_LABEL_TEXT_SEL                   1
#define LV_LABEL_LONG_TXT_HINT              1
#define LV_LIST_DEF_ANIM_TIME               100
#define LV_LINEMETER_PRECISE                0
#define LV_PAGE_DEF_ANIM_TIME               400
#define LV_SPINNER_DEF_ARC_LENGTH           60
#define LV_SPINNER_DEF_SPIN_TIME            1000
#define LV_SPINNER_DEF_ANIM                 LV_SPINNER_TYPE_SPINNING_ARC
#define LV_ROLLER_DEF_ANIM_TIME             200
#define LV_ROLLER_INF_PAGES                 7
#define LV_TEXTAREA_DEF_CURSOR_BLINK_TIME   400
#define LV_TEXTAREA_DEF_PWD_SHOW_TIME       1500
#define LV_TABLE_COL_MAX                    12
#define LV_TABVIEW_DEF_ANIM_TIME            300
#define LV_TILEVIEW_DEF_ANIM_TIME           300
#endif

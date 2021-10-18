/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FONT_H
#define _FONT_H
#include"gui.h"
typedef enum{
	FT_FONT_STYLE_NORMAL = 0,
	FT_FONT_STYLE_ITALIC = 1<<0,
	FT_FONT_STYLE_BOLD   = 1<<1
}lv_ft_style;
extern void lv_ft_destroy(lv_font_t*font);
extern bool lv_freetype_init(uint16_t max_faces, uint16_t max_sizes, uint32_t max_bytes);
extern void lv_freetype_destroy(void);
extern lv_font_t*lv_ft_init(const char*name,int weight,lv_ft_style style);
extern lv_font_t*lv_ft_init_data(unsigned char*data,long size,int weight,lv_ft_style style);
#ifdef ASSETS_H
extern lv_font_t*lv_ft_init_assets(entry_dir*assets,char*path,int weight,lv_ft_style style);
#endif
extern lv_font_t*lv_ft_init_rootfs(char*path,int weight,lv_ft_style style);
#endif

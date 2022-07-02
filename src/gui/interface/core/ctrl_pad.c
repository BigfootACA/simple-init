/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include"gui.h"
#include"gui/tools.h"
#include"gui/sysbar.h"

struct ctrl_pad_btn{
	lv_obj_t*btn;
	lv_obj_t*lbl;
};

static struct{
	int mode;
	bool show;
	lv_obj_t*pad;
	lv_obj_t*target;
	struct ctrl_pad_btn btn_home,arr_up,btn_end;
	struct ctrl_pad_btn arr_left,btn_mode,arr_right;
	struct ctrl_pad_btn btn_move,arr_down,btn_close;
}ctrl_pad;

void ctrl_pad_show(void){
	if(ctrl_pad.show)return;
	ctrl_pad.show=true;
	lv_obj_set_hidden(ctrl_pad.pad,false);
	lv_group_add_obj(gui_grp,ctrl_pad.btn_home.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.arr_up.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.btn_end.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.arr_left.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.btn_mode.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.arr_right.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.btn_move.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.arr_down.btn);
	lv_group_add_obj(gui_grp,ctrl_pad.btn_close.btn);
}

bool ctrl_pad_is_show(void){
	return ctrl_pad.show;
}

void ctrl_pad_toggle(void){
	if(ctrl_pad.show)ctrl_pad_hide();
	else ctrl_pad_show();
}

void ctrl_pad_hide(void){
	if(!ctrl_pad.show)return;
	ctrl_pad.show=false;
	lv_obj_set_hidden(ctrl_pad.pad,true);
	lv_group_remove_obj(ctrl_pad.btn_home.btn);
	lv_group_remove_obj(ctrl_pad.arr_up.btn);
	lv_group_remove_obj(ctrl_pad.btn_end.btn);
	lv_group_remove_obj(ctrl_pad.arr_left.btn);
	lv_group_remove_obj(ctrl_pad.btn_mode.btn);
	lv_group_remove_obj(ctrl_pad.arr_right.btn);
	lv_group_remove_obj(ctrl_pad.btn_move.btn);
	lv_group_remove_obj(ctrl_pad.arr_down.btn);
	lv_group_remove_obj(ctrl_pad.btn_close.btn);
}

static void ctrl_pad_cb(lv_event_t*e){
	int m=ctrl_pad.mode;
	lv_obj_t*view=ctrl_pad.target;
	const lv_coord_t dist=gui_font_size*16;
	if(!view)view=sysbar.focus_input;
	lv_coord_t sx=lv_obj_get_scroll_x(view);
	lv_coord_t sy=lv_obj_get_scroll_y(view);
	if(e->target==ctrl_pad.btn_mode.btn){
		lv_label_set_text(ctrl_pad.btn_mode.lbl,m==1?"\uf034":"\uf035");
		ctrl_pad.mode=m==0?1:0;
	}else if(e->target==ctrl_pad.btn_move.btn){
		lv_align_t a;
		lv_coord_t p,y=lv_obj_get_y(ctrl_pad.pad);
		if(y>(lv_coord_t)gui_h/2)a=LV_ALIGN_TOP_MID,p=gui_font_size;
		else a=LV_ALIGN_BOTTOM_MID,p=-gui_font_size;
		lv_obj_align_to(ctrl_pad.pad,NULL,a,0,p);
	}else if(e->target==ctrl_pad.btn_close.btn)ctrl_pad_hide();
	else if(!view)return;
	else if(e->target==ctrl_pad.btn_home.btn)
		lv_obj_scroll_to_y(view,0,LV_ANIM_ON);
	else if(e->target==ctrl_pad.btn_end.btn)
		lv_obj_scroll_to_y(view,lv_obj_get_scroll_bottom(view),LV_ANIM_ON);
	else if(e->target==ctrl_pad.arr_up.btn){
		sy-=dist;
		if(m==0)lv_obj_scroll_to_y(view,sy,LV_ANIM_ON);
		else lv_textarea_cursor_up(view);
	}else if(e->target==ctrl_pad.arr_down.btn){
		sy+=dist;
		if(m==0)lv_obj_scroll_to_y(view,sy,LV_ANIM_ON);
		else lv_textarea_cursor_down(view);
	}else if(e->target==ctrl_pad.arr_left.btn){
		sx-=dist;
		if(m==0)lv_obj_scroll_to_x(view,sx,LV_ANIM_ON);
		else lv_textarea_cursor_left(view);
	}else if(e->target==ctrl_pad.arr_right.btn){
		sx+=dist;
		if(m==0)lv_obj_scroll_to_x(view,sx,LV_ANIM_ON);
		else lv_textarea_cursor_right(view);
	}
}

static void ctrl_pad_draw_button(struct ctrl_pad_btn*btn,lv_coord_t x,lv_coord_t y,const char*str){
	btn->btn=lv_btn_create(ctrl_pad.pad);
	lv_obj_set_grid_cell(
		btn->btn,
		LV_GRID_ALIGN_STRETCH,x,1,
		LV_GRID_ALIGN_STRETCH,y,1
	);
	btn->lbl=lv_label_create(btn->btn);
	lv_label_set_text(btn->lbl,str);
	lv_obj_clear_flag(btn->btn,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(btn->btn,LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_flag(btn->lbl,LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_event_cb(btn->btn,ctrl_pad_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_style_radius(btn->btn,LV_RADIUS_CIRCLE,0);
	lv_obj_center(btn->lbl);
}

void ctrl_pad_draw(void){
	memset(&ctrl_pad,0,sizeof(ctrl_pad));
	static lv_coord_t grid[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};

	ctrl_pad.pad=lv_obj_create(sysbar.screen);
	lv_obj_clear_flag(ctrl_pad.pad,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_hidden(ctrl_pad.pad,true);
	lv_obj_set_size(ctrl_pad.pad,gui_font_size*8,gui_font_size*8);
	lv_obj_align_to(ctrl_pad.pad,NULL,LV_ALIGN_BOTTOM_MID,0,-gui_font_size);
	lv_obj_set_grid_dsc_array(ctrl_pad.pad,grid,grid);

	ctrl_pad_draw_button(&ctrl_pad.btn_home,  0,0,LV_SYMBOL_PREV);
	ctrl_pad_draw_button(&ctrl_pad.arr_up,    1,0,LV_SYMBOL_UP);
	ctrl_pad_draw_button(&ctrl_pad.btn_end,   2,0,LV_SYMBOL_NEXT);
	ctrl_pad_draw_button(&ctrl_pad.arr_left,  0,1,LV_SYMBOL_LEFT);
	ctrl_pad_draw_button(&ctrl_pad.btn_mode,  1,1,"\uf034");
	ctrl_pad_draw_button(&ctrl_pad.arr_right, 2,1,LV_SYMBOL_RIGHT);
	ctrl_pad_draw_button(&ctrl_pad.btn_move,  0,2,"\uf0b2");
	ctrl_pad_draw_button(&ctrl_pad.arr_down,  1,2,LV_SYMBOL_DOWN);
	ctrl_pad_draw_button(&ctrl_pad.btn_close, 2,2,LV_SYMBOL_CLOSE);
	lv_obj_add_drag(ctrl_pad.pad);
}

void ctrl_pad_set_target(lv_obj_t*target){
	ctrl_pad.target=target;
}

#endif
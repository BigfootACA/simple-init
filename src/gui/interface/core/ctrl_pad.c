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
	lv_coord_t bts,btm,btx;
	lv_coord_t pts,ptm;
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

static void ctrl_pad_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	int m=ctrl_pad.mode;
	lv_obj_t*view=ctrl_pad.target;
	const lv_coord_t dist=gui_font_size*16;
	if(!view)view=sysbar.focus_input;
	if(obj==ctrl_pad.btn_mode.btn){
		lv_label_set_text(ctrl_pad.btn_mode.lbl,m==1?"\uf034":"\uf035");
		ctrl_pad.mode=m==0?1:0;
	}else if(obj==ctrl_pad.btn_move.btn){
		lv_align_t a;
		lv_coord_t p,y=lv_obj_get_y(ctrl_pad.pad);
		if(y>(lv_coord_t)gui_h/2)a=LV_ALIGN_IN_TOP_MID,p=ctrl_pad.ptm;
		else a=LV_ALIGN_IN_BOTTOM_MID,p=-ctrl_pad.ptm;
		lv_obj_align(ctrl_pad.pad,NULL,a,0,p);
	}else if(obj==ctrl_pad.btn_close.btn)ctrl_pad_hide();
	else if(!view)return;
	else if(obj==ctrl_pad.btn_home.btn)lv_page_go_top(view);
	else if(obj==ctrl_pad.btn_end.btn)lv_page_go_bottom(view);
	else if(obj==ctrl_pad.arr_up.btn){
		if(m==0)lv_page_scroll_ver(view,dist);
		else lv_textarea_cursor_up(view);
	}else if(obj==ctrl_pad.arr_down.btn){
		if(m==0)lv_page_scroll_ver(view,-dist);
		else lv_textarea_cursor_down(view);
	}else if(obj==ctrl_pad.arr_left.btn){
		if(m==0)lv_page_scroll_hor(view,dist);
		else lv_textarea_cursor_left(view);
	}else if(obj==ctrl_pad.arr_right.btn){
		if(m==0)lv_page_scroll_hor(view,-dist);
		else lv_textarea_cursor_right(view);
	}
}

static void ctrl_pad_draw_button(struct ctrl_pad_btn*btn,lv_coord_t x,lv_coord_t y,const char*str){
	btn->btn=lv_btn_create(ctrl_pad.pad,NULL);
	lv_obj_set_drag_parent(btn->btn,true);
	lv_style_set_action_button(btn->btn,true);
	btn->lbl=lv_label_create(btn->btn,NULL);
	lv_label_set_text(btn->lbl,str);
	lv_obj_set_event_cb(btn->btn,ctrl_pad_cb);
	lv_obj_set_size(btn->btn,ctrl_pad.bts,ctrl_pad.bts);
	lv_obj_set_pos(btn->btn,ctrl_pad.btx*x+ctrl_pad.btm,ctrl_pad.btx*y+ctrl_pad.btm);
	lv_obj_set_style_local_radius(btn->btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
}

static void drag_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_DRAG_END)return;
	lv_drag_border(obj,gui_w,gui_h,gui_font_size);
}

void ctrl_pad_draw(void){
	memset(&ctrl_pad,0,sizeof(ctrl_pad));
	ctrl_pad.bts=gui_font_size*2;
	ctrl_pad.btm=gui_font_size/2;
	ctrl_pad.btx=ctrl_pad.bts+ctrl_pad.btm;
	ctrl_pad.pts=ctrl_pad.btx*3+ctrl_pad.btm;
	ctrl_pad.ptm=gui_font_size*2+sysbar.size;

	ctrl_pad.pad=lv_obj_create(sysbar.screen,NULL);
	lv_obj_set_event_cb(ctrl_pad.pad,drag_cb);
	lv_obj_set_drag(ctrl_pad.pad,true);
	lv_obj_set_click(ctrl_pad.pad,false);
	lv_obj_set_hidden(ctrl_pad.pad,true);
	lv_obj_set_size(ctrl_pad.pad,ctrl_pad.pts,ctrl_pad.pts);
	lv_obj_align(ctrl_pad.pad,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-ctrl_pad.ptm);

	ctrl_pad_draw_button(&ctrl_pad.btn_home,  0,0,LV_SYMBOL_PREV);
	ctrl_pad_draw_button(&ctrl_pad.arr_up,    1,0,LV_SYMBOL_UP);
	ctrl_pad_draw_button(&ctrl_pad.btn_end,   2,0,LV_SYMBOL_NEXT);
	ctrl_pad_draw_button(&ctrl_pad.arr_left,  0,1,LV_SYMBOL_LEFT);
	ctrl_pad_draw_button(&ctrl_pad.btn_mode,  1,1,"\uf034");
	ctrl_pad_draw_button(&ctrl_pad.arr_right, 2,1,LV_SYMBOL_RIGHT);
	ctrl_pad_draw_button(&ctrl_pad.btn_move,  0,2,"\uf0b2");
	ctrl_pad_draw_button(&ctrl_pad.arr_down,  1,2,LV_SYMBOL_DOWN);
	ctrl_pad_draw_button(&ctrl_pad.btn_close, 2,2,LV_SYMBOL_CLOSE);
}

void ctrl_pad_set_target(lv_obj_t*target){
	ctrl_pad.target=target;
}

#endif
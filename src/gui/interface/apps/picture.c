/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */


#ifdef ENABLE_GUI
#include<stdint.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<strings.h>
#include<stdbool.h>
#include"gui.h"
#include"gui/tools.h"
#include"gui/activity.h"
#include"gui/sysbar.h"
#include"gui/filepicker.h"

struct picture_viewer{
	char path[PATH_MAX];
	lv_obj_t*scr,*img,*btn;
	struct{
		lv_obj_t*pad;
		lv_obj_t*zoom_big,*arr_up,*zoom_small,*reload;
		lv_obj_t*arr_left,*close,*arr_right,*full_screen;
		lv_obj_t*rotate_left,*arr_down,*rotate_right,*open;
		lv_coord_t bts,btm,btx;
	}pad;
};

static void reload_image(struct picture_viewer*pv){
	lv_obj_set_hidden(pv->img,true);
	lv_img_set_src(pv->img,pv->path);
	lv_img_ext_t*e=lv_obj_get_ext_attr(pv->img);
	if(e->w<=0||e->h<=0)return;
	else lv_obj_set_hidden(pv->img,false);
	lv_obj_set_pos(pv->img,0,0);
	lv_img_set_angle(pv->img,0);
	lv_img_set_zoom(pv->img,256);
	if(e->w>=(lv_coord_t)gui_sw||e->h>=(lv_coord_t)gui_sh){
		lv_img_fill_image(pv->img,gui_sw,gui_sh);
		lv_img_set_pivot(pv->img,0,0);
	}else lv_obj_align(pv->img,NULL,LV_ALIGN_CENTER,0,0);
}

static void open_image(struct picture_viewer*pv,const char*path){
	if(strcasecmp(pv->path,path)==0)return;
	memset(pv->path,0,sizeof(pv->path));
	strncpy(pv->path,path,sizeof(pv->path)-1);
	reload_image(pv);
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	open_image(user_data,path[0]);
	return false;
}

static void pad_get_focus(struct picture_viewer*pv){
	if(!pv)return;
	lv_group_add_obj(gui_grp,pv->pad.zoom_big);
	lv_group_add_obj(gui_grp,pv->pad.arr_up);
	lv_group_add_obj(gui_grp,pv->pad.zoom_small);
	lv_group_add_obj(gui_grp,pv->pad.reload);
	lv_group_add_obj(gui_grp,pv->pad.arr_left);
	lv_group_add_obj(gui_grp,pv->pad.close);
	lv_group_add_obj(gui_grp,pv->pad.arr_right);
	lv_group_add_obj(gui_grp,pv->pad.full_screen);
	lv_group_add_obj(gui_grp,pv->pad.rotate_left);
	lv_group_add_obj(gui_grp,pv->pad.arr_down);
	lv_group_add_obj(gui_grp,pv->pad.rotate_right);
	lv_group_add_obj(gui_grp,pv->pad.open);
}

static void pad_lost_focus(struct picture_viewer*pv){
	if(!pv)return;
	lv_group_remove_obj(pv->pad.zoom_big);
	lv_group_remove_obj(pv->pad.arr_up);
	lv_group_remove_obj(pv->pad.zoom_small);
	lv_group_remove_obj(pv->pad.reload);
	lv_group_remove_obj(pv->pad.arr_left);
	lv_group_remove_obj(pv->pad.close);
	lv_group_remove_obj(pv->pad.arr_right);
	lv_group_remove_obj(pv->pad.full_screen);
	lv_group_remove_obj(pv->pad.rotate_left);
	lv_group_remove_obj(pv->pad.arr_down);
	lv_group_remove_obj(pv->pad.rotate_right);
	lv_group_remove_obj(pv->pad.open);
}

static void pad_show(struct picture_viewer*pv,bool show){
	lv_obj_set_hidden(pv->pad.pad,!show);
	lv_obj_set_hidden(pv->btn,show);
	if(show){
		pad_get_focus(pv);
		lv_group_remove_obj(pv->btn);
	}else{
		pad_lost_focus(pv);
		lv_group_add_obj(gui_grp,pv->btn);
	}
}

static void pad_btn_cb(lv_obj_t*obj,lv_event_t e){
	const lv_coord_t move_dist=gui_font_size*2;
	const lv_coord_t rotate_dist=900;
	const lv_coord_t zoom_dist=32;
	struct picture_viewer*pv=lv_obj_get_user_data(obj);
	if(e!=LV_EVENT_CLICKED||!pv)return;
	int t=0;
	lv_img_ext_t*d=lv_obj_get_ext_attr(pv->img);
	lv_coord_t x=lv_obj_get_x(pv->img);
	lv_coord_t y=lv_obj_get_y(pv->img);
	int32_t a=d->angle;
	int16_t z=d->zoom;
	if(obj==pv->pad.arr_up)y-=move_dist,t=1;
	else if(obj==pv->pad.arr_down)y+=move_dist,t=1;
	else if(obj==pv->pad.arr_left)x-=move_dist,t=1;
	else if(obj==pv->pad.arr_right)x+=move_dist,t=1;
	else if(obj==pv->pad.rotate_right)a+=rotate_dist,t=2;
	else if(obj==pv->pad.rotate_left)a-=rotate_dist,t=2;
	else if(obj==pv->pad.rotate_right)a+=rotate_dist,t=2;
	else if(obj==pv->pad.rotate_left)a-=rotate_dist,t=2;
	else if(obj==pv->pad.zoom_big)z+=zoom_dist,t=3;
	else if(obj==pv->pad.zoom_small)z-=zoom_dist,t=3;
	else if(obj==pv->pad.reload)reload_image(pv);
	else if(obj==pv->pad.close)pad_show(pv,false);
	else if(obj==pv->pad.open){
		struct filepicker*fp=filepicker_create(select_cb,"Open picture");
		filepicker_set_user_data(fp,pv);
		filepicker_set_max_item(fp,1);
	}else if(obj==pv->pad.full_screen){
		sysbar_set_full_screen(!sysbar.full_screen);
		sysbar_hide_full_screen_btn();
		lv_obj_set_size(pv->scr,gui_sw,gui_sh);
	}
	switch(t){
		case 1:lv_obj_set_pos(pv->img,x,y);break;
		case 3:
			if(z<32)z=32;
			if(z>768)z=768;
			lv_img_set_zoom(pv->img,z);
		break;
		case 2:
			if(a<0)a+=3600;
			else if(a>=3600)a=0,x=0,y=0;
			lv_img_set_angle(pv->img,a);
		break;
	}
}

static void drag_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_DRAG_END)return;
	lv_drag_border(obj,gui_sw,gui_sh,gui_font_size);
}

static lv_obj_t*pad_draw_button(struct picture_viewer*pv,lv_coord_t x,lv_coord_t y,const char*str){
	lv_obj_t*btn=lv_btn_create(pv->pad.pad,NULL);
	lv_obj_set_user_data(btn,pv);
	lv_obj_set_event_cb(btn,pad_btn_cb);
	lv_obj_set_drag_parent(btn,true);
	lv_style_set_action_button(btn,true);
	lv_label_set_text(lv_label_create(btn,NULL),str);
	lv_obj_set_size(btn,pv->pad.bts,pv->pad.bts);
	lv_obj_set_pos(btn,pv->pad.btx*x+pv->pad.btm,pv->pad.btx*y+pv->pad.btm);
	lv_obj_set_style_local_radius(btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	return btn;
}

static void pad_draw(struct picture_viewer*pv){
	pv->pad.bts=gui_font_size*2;
	pv->pad.btm=gui_font_size/2;
	pv->pad.btx=pv->pad.bts+pv->pad.btm;
	lv_coord_t ptw=pv->pad.btx*4+pv->pad.btm;
	lv_coord_t pth=pv->pad.btx*3+pv->pad.btm;

	pv->pad.pad=lv_obj_create(pv->scr,NULL);
	lv_obj_set_drag(pv->pad.pad,true);
	lv_obj_set_click(pv->pad.pad,false);
	lv_obj_set_hidden(pv->pad.pad,true);
	lv_obj_set_size(pv->pad.pad,ptw,pth);
	lv_obj_set_event_cb(pv->pad.pad,drag_cb);
	lv_obj_align(pv->pad.pad,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-gui_font_size*2);

	pv->pad.zoom_big     = pad_draw_button(pv,0,0,"\uf00e");
	pv->pad.arr_up       = pad_draw_button(pv,1,0,LV_SYMBOL_UP);
	pv->pad.zoom_small   = pad_draw_button(pv,2,0,"\uf010");
	pv->pad.reload       = pad_draw_button(pv,3,0,LV_SYMBOL_REFRESH);
	pv->pad.arr_left     = pad_draw_button(pv,0,1,LV_SYMBOL_LEFT);
	pv->pad.close        = pad_draw_button(pv,1,1,LV_SYMBOL_CLOSE);
	pv->pad.arr_right    = pad_draw_button(pv,2,1,LV_SYMBOL_RIGHT);
	pv->pad.full_screen  = pad_draw_button(pv,3,1,"\uf066");
	pv->pad.rotate_left  = pad_draw_button(pv,0,2,"\uf0e2");
	pv->pad.arr_down     = pad_draw_button(pv,1,2,LV_SYMBOL_DOWN);
	pv->pad.rotate_right = pad_draw_button(pv,2,2,"\uf01e");
	pv->pad.open         = pad_draw_button(pv,3,2,LV_SYMBOL_UPLOAD);
}

static int picture_get_focus(struct gui_activity*d){
	struct picture_viewer*pv=d->data;
	if(!pv)return 0;
	lv_group_add_obj(gui_grp,pv->btn);
	pad_get_focus(pv);
	return 0;
}

static int picture_lost_focus(struct gui_activity*d){
	struct picture_viewer*pv=d->data;
	if(!pv)return 0;
	lv_group_remove_obj(pv->btn);
	pad_lost_focus(pv);
	return 0;
}

static int do_clean(struct gui_activity*act){
	free(act->data);
	act->data=NULL;
	return 0;
}

static void btn_cb(lv_obj_t*obj,lv_event_t e){
	struct picture_viewer*pv=lv_obj_get_user_data(obj);
	if(!pv||pv->btn!=obj)return;
	switch(e){
		case LV_EVENT_DRAG_END:drag_cb(obj,e);break;
		case LV_EVENT_CLICKED:pad_show(pv,true);break;
	}
}

static int picture_draw(struct gui_activity*act){
	struct picture_viewer*pv=malloc(sizeof(struct picture_viewer));
	if(!pv)return -1;
	memset(pv,0,sizeof(struct picture_viewer));
	pv->scr=act->page,act->data=pv;
	lv_obj_set_style_local_bg_color(pv->scr,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLACK);

	pv->img=lv_img_create(pv->scr,NULL);
	lv_obj_set_drag(pv->img,true);
	lv_obj_set_hidden(pv->img,true);

	lv_coord_t bts=gui_font_size*3;
	lv_coord_t btm=bts+gui_font_size;
	pv->btn=lv_btn_create(pv->scr,NULL);
	lv_obj_set_event_cb(pv->btn,btn_cb);
	lv_obj_set_user_data(pv->btn,pv);
	lv_obj_set_checked(pv->btn,true);
	lv_obj_set_drag(pv->btn,true);
	lv_obj_set_size(pv->btn,bts,bts);
	lv_obj_set_pos(pv->btn,gui_sw-btm,gui_sh-btm);
	lv_obj_set_style_local_shadow_color(pv->btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GRAY);
	lv_obj_set_style_local_shadow_width(pv->btn,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size);
	lv_label_set_text(lv_label_create(pv->btn,NULL),"\uf0c9");

	if(act->args)open_image(pv,act->args);
	pad_draw(pv);
	return 0;
}

struct gui_register guireg_picture_viewer={
	.name="picture-viewer",
	.title="Picture Viewer",
	.icon="photo-viewer.svg",
	.open_file=true,
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=picture_draw,
	.lost_focus=picture_lost_focus,
	.get_focus=picture_get_focus,
	.back=true,
};
#endif

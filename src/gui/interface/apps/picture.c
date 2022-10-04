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
#include"logger.h"
#include"gui/tools.h"
#include"gui/activity.h"
#include"gui/sysbar.h"
#include"gui/filepicker.h"
#define TAG "picture"

struct picture_viewer{
	char path[PATH_MAX];
	lv_obj_t*scr,*img,*info,*btn;
	lv_obj_t*pad;
};

static void reload_image(struct picture_viewer*pv){
	lv_obj_add_flag(pv->img,LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(pv->info,LV_OBJ_FLAG_HIDDEN);
	lv_img_set_src(pv->img,pv->path);
	lv_img_t*e=(lv_img_t*)pv->img;
	if(e->w<=0||e->h<=0){
		lv_obj_clear_flag(pv->info,LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(pv->info,_("Picture load failed"));
		lv_obj_align_to(pv->info,NULL,LV_ALIGN_CENTER,0,0);
		return;
	}else lv_obj_clear_flag(pv->img,LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_pos(pv->img,0,0);
	lv_img_set_angle(pv->img,0);
	lv_img_set_zoom(pv->img,256);
	if(e->w>=(lv_coord_t)gui_sw||e->h>=(lv_coord_t)gui_sh){
		lv_img_fill_image(pv->img,gui_sw,gui_sh);
		lv_img_set_pivot(pv->img,0,0);
	}else lv_obj_align_to(pv->img,NULL,LV_ALIGN_CENTER,0,0);
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
	if(!lv_obj_has_flag(pv->pad,LV_OBJ_FLAG_HIDDEN)){
		lv_group_add_obj(gui_grp,pv->pad);
		lv_group_focus_obj(pv->pad);
		lv_group_set_editing(gui_grp,true);
	}
}

static void pad_lost_focus(struct picture_viewer*pv){
	if(!pv)return;
	lv_group_remove_obj(pv->pad);
	lv_group_set_editing(gui_grp,false);
}

static void pad_show(struct picture_viewer*pv,bool show){
	if(show){
		lv_obj_clear_flag(pv->pad,LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(pv->btn,LV_OBJ_FLAG_HIDDEN);
		pad_get_focus(pv);
		lv_group_remove_obj(pv->btn);
	}else{
		lv_obj_add_flag(pv->pad,LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(pv->btn,LV_OBJ_FLAG_HIDDEN);
		pad_lost_focus(pv);
		lv_group_add_obj(gui_grp,pv->btn);
	}
}
enum pad_btns{
	BTN_ZOOM_BIG=0,    BTN_MOVE_UP=1,   BTN_ZOOM_SMALL=2,    BTN_RELOAD_IMAGE=3,
	BTN_MOVE_LEFT=4,   BTN_CLOSE_PAD=5, BTN_MOVE_RIGHT=6,    BTN_FULL_SCREEN=7,
	BTN_ROTATE_LEFT=8, BTN_MOVE_DOWN=9, BTN_ROTATE_RIGHT=10, BTN_OPEN_NEW_IMAGE=11,
};

static void pad_btn_cb(lv_event_t*e){
	const lv_coord_t move_dist=gui_font_size*2;
	const lv_coord_t rotate_dist=900;
	const lv_coord_t zoom_dist=32;
	struct picture_viewer*pv=e->user_data;
	int t=0;
	lv_img_t*d=(lv_img_t*)pv->img;
	lv_coord_t x=lv_obj_get_x(pv->img);
	lv_coord_t y=lv_obj_get_y(pv->img);
	int32_t a=d->angle;
	int16_t z=d->zoom;
	uint16_t u=lv_btnmatrix_get_selected_btn(pv->pad);
	tlog_debug("trigger %d",u);
	switch(u){
		case BTN_MOVE_UP:y-=move_dist,t=1;break;
		case BTN_MOVE_DOWN:y+=move_dist,t=1;break;
		case BTN_MOVE_LEFT:x-=move_dist,t=1;break;
		case BTN_MOVE_RIGHT:x+=move_dist,t=1;break;
		case BTN_ROTATE_RIGHT:a+=rotate_dist,t=2;break;
		case BTN_ROTATE_LEFT:a-=rotate_dist,t=2;break;
		case BTN_ZOOM_BIG:z+=zoom_dist,t=3;break;
		case BTN_ZOOM_SMALL:z-=zoom_dist,t=3;break;
		case BTN_RELOAD_IMAGE:reload_image(pv);break;
		case BTN_CLOSE_PAD:pad_show(pv,false);break;
		case BTN_OPEN_NEW_IMAGE:{
			struct filepicker*fp=filepicker_create(select_cb,"Open picture");
			filepicker_set_user_data(fp,pv);
			filepicker_set_max_item(fp,1);
		}break;
		case BTN_FULL_SCREEN:{
			sysbar_set_full_screen(!sysbar.full_screen);
			sysbar_hide_full_screen_btn();
			lv_obj_set_size(pv->scr,gui_sw,gui_sh);
		}break;
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


static void pad_draw(struct picture_viewer*pv){
	static const char*btns[]={
		// 0: zoom big      1: move up        2: zoom small      3: reload image
		"\uf00e",           LV_SYMBOL_UP,     "\uf010",          LV_SYMBOL_REFRESH, "\n",
		// 4: move left     5: close pad      6: move right      7: full screen
		LV_SYMBOL_LEFT,     LV_SYMBOL_CLOSE,  LV_SYMBOL_RIGHT,   "\uf066",          "\n",
		// 8: rotate left   9: move down      10: rotate right   11:open new image
		"\uf0e2",           LV_SYMBOL_DOWN,   "\uf01e",          LV_SYMBOL_UPLOAD,  ""
	};
	pv->pad=lv_btnmatrix_create(pv->scr);
	lv_btnmatrix_set_map(pv->pad,btns);
	lv_obj_set_size(pv->pad,gui_font_size*10,gui_font_size*7);
	lv_obj_set_style_radius(pv->pad,LV_RADIUS_CIRCLE,LV_PART_ITEMS);
	lv_obj_set_style_text_align(pv->pad,LV_ALIGN_CENTER,LV_PART_ITEMS);
	lv_obj_set_style_text_color(pv->pad,lv_color_white(),LV_PART_ITEMS);
	lv_obj_set_style_bg_color(pv->pad,lv_palette_main(LV_PALETTE_BLUE),LV_PART_ITEMS);
	lv_obj_align_to(pv->pad,NULL,LV_ALIGN_BOTTOM_MID,0,-gui_font_size*2);
	lv_obj_add_event_cb(pv->pad,pad_btn_cb,LV_EVENT_VALUE_CHANGED,pv);
	lv_obj_add_flag(pv->pad,LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_drag(pv->pad);
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

static void btn_cb(lv_event_t*e){
	pad_show(e->user_data,true);
}

static int picture_draw(struct gui_activity*act){
	struct picture_viewer*pv=malloc(sizeof(struct picture_viewer));
	if(!pv)return -1;
	memset(pv,0,sizeof(struct picture_viewer));
	pv->scr=act->page,act->data=pv;
	lv_obj_set_style_bg_color(pv->scr,lv_color_black(),0);

	pv->img=lv_img_create(pv->scr);
	lv_obj_add_flag(pv->img,LV_OBJ_FLAG_HIDDEN);

	pv->info=lv_label_create(pv->scr);
	lv_obj_set_style_text_color(pv->info,lv_color_white(),0);
	lv_label_set_text(pv->info,_("Nothing opens"));
	lv_obj_align_to(pv->info,NULL,LV_ALIGN_CENTER,0,0);

	lv_coord_t bts=gui_font_size*3;
	lv_coord_t btm=bts+gui_font_size;
	pv->btn=lv_btn_create(pv->scr);
	lv_obj_add_drag(pv->btn);
	lv_obj_add_event_cb(pv->btn,btn_cb,LV_EVENT_CLICKED,pv);
	lv_obj_set_checked(pv->btn,true);
	lv_obj_set_size(pv->btn,bts,bts);
	lv_obj_set_pos(pv->btn,gui_sw-btm,gui_sh-btm);
	lv_obj_set_style_radius(pv->btn,LV_RADIUS_CIRCLE,0);
	lv_obj_set_style_shadow_color(pv->btn,lv_palette_main(LV_PALETTE_GREY),0);
	lv_obj_set_style_shadow_width(pv->btn,gui_font_size,0);
	lv_obj_t*txt=lv_label_create(pv->btn);
	lv_label_set_text(txt,"\uf0c9");
	lv_obj_center(txt);

	if(act->args)open_image(pv,act->args);
	pad_draw(pv);
	return 0;
}

struct gui_register guireg_picture_viewer={
	.name="picture-viewer",
	.title="Picture Viewer",
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.png$",
		".*\\.jpg$",
		".*\\.jpeg$",
		".*\\.bmp$",
		".*\\.svg$",
		".*\\.psd$",
		".*\\.tga$",
		".*\\.gif$",
		".*\\.hdr$",
		".*\\.pic$",
		".*\\.pnm$",
		".*\\.ppm$",
		".*\\.pgm$",
		NULL
	},
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=picture_draw,
	.lost_focus=picture_lost_focus,
	.get_focus=picture_get_focus,
	.back=true,
};
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/fileview.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/filepicker.h"

struct filepicker{
	char text[BUFSIZ],path[PATH_MAX];
	filepicker_callback callback;
	lv_obj_t*mask,*box,*label;
	lv_obj_t*view,*cur_path;
	lv_obj_t*reload,*new,*home;
	lv_obj_t*ok,*cancel;
	uint32_t max;
	void*user_data;
	struct gui_activity*act;
	struct fileview*fv;
};

struct create_data{
	struct filepicker*fp;
	uint16_t xid;
};

static void filepicker_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct filepicker*fp;
	if(!(fp=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=fp)return;
	void*dp;
	uint16_t cnt=0,i;
	size_t ss,vs,cs,as;
	char**fn=NULL,**name,*path;
	if(obj==fp->ok){
		cnt=fileview_get_checked_count(fp->fv);
		ss=strlen(path=fileview_get_lvgl_path(fp->fv));
		if(path[ss-1]=='/')path[ss-1]=0;
		vs=sizeof(char*)*(cnt+1),cs=ss+260,as=vs+(cs*cnt);
		if((dp=malloc(as))){
			memset(dp,0,as);
			fn=dp,name=fileview_get_checked(fp->fv);
			for(i=0;i<cnt;i++){
				fn[i]=(char*)(dp+vs+(cs*i));
				snprintf(fn[i],cs-1,"%s/%s",path,name[i]);
			}
			free(name);
		}else fn=NULL,cnt=0;
	}
	if(!fp->callback)free(fn);
	else if(fp->callback(obj==fp->ok,(const char**)fn,cnt,fp->user_data))return;
	lv_obj_set_user_data(fp->ok,NULL);
	lv_obj_set_user_data(fp->cancel,NULL);
	fp->act->args=NULL;
	free(fp);
	guiact_do_back();
}

static bool create_name_cb(bool ok,const char*name,void*user_data){
	if(!ok||!user_data)return false;
	struct create_data*cd=user_data;
	if(!cd->fp)return false;
	char*fp=fileview_get_lvgl_path(cd->fp->fv);
	char xp[PATH_MAX]={0};
	snprintf(xp,PATH_MAX-1,"%s/%s",fp,name);
	lv_res_t r;
	switch(cd->xid){
		case 0:r=lv_fs_creat(xp);break;//file
		case 1:r=lv_fs_mkdir(xp);break;//folder
		default:return false;
	}
	fileview_set_path(cd->fp->fv,NULL);
	if(r!=LV_FS_RES_OK)msgbox_alert(
		"Create '%s' failed: %s",
		name,lv_fs_res_to_i18n_string(r)
	);
	return false;
}

static bool create_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data){
	static struct create_data cd;
	cd.xid=id,cd.fp=user_data;
	struct inputbox*in=inputbox_create(create_name_cb,"Create item name");
	inputbox_set_user_data(in,&cd);
	return false;
}

static void btn_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct filepicker*fp;
	if(!(fp=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=fp)return;
	if(obj==fp->reload)fileview_set_path(fp->fv,NULL);
	else if(obj==fp->home)fileview_set_path(fp->fv,fp->path);
	else if(obj==fp->new){
		static const char*types[]={
			LV_SYMBOL_FILE,
			LV_SYMBOL_DIRECTORY,
			""
		};
		if(fsext_is_multi&&fileview_is_top(fp->fv))return;
		msgbox_set_user_data(msgbox_create_custom(
			create_cb,types,
			"Choose type to create"
		),fp);
	}
}

static void on_change_dir(struct fileview*fv,char*old __attribute__((unused)),char*new){
	struct filepicker*fp=fileview_get_data(fv);
	if(!fp)return;
	lv_label_set_text(fp->cur_path,new);
	lv_obj_set_enabled(fp->new,!fsext_is_multi||!fileview_is_top(fv));
}

static void on_item_select(
	struct fileview*fv,
	char*name __attribute__((unused)),
	enum item_type type __attribute__((unused)),
	bool checked __attribute__((unused)),
	uint16_t cnt
){
	if(fsext_is_multi&&fileview_is_top(fv))return;
	struct filepicker*fp=fileview_get_data(fv);
	if(!fp)return;
	lv_obj_set_enabled(fp->ok,cnt>0&&(cnt<=fp->max||fp->max==0));
}

static int filepicker_draw(struct gui_activity*act){
	lv_coord_t box_h=gui_dpi/8;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3,xw=MIN(max_w,cur_w);
	lv_coord_t max_h=gui_dpi*6,cur_h=gui_sh/5*2,xh=MIN(max_h,cur_h);
	struct filepicker*fp=act->args;
	fp->act=act;

	fp->mask=lv_create_opa_mask(act->page);
	fp->box=lv_obj_create(fp->mask,NULL);
	lv_obj_set_style_local_border_width(fp->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(fp->box,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_border_width(fp->box,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_width(fp->box,xw);

	lv_coord_t lh=gui_sh/8;
	fp->label=lv_label_create(fp->box,NULL);
	lv_label_set_long_mode(fp->label,LV_LABEL_LONG_BREAK);
	lv_label_set_align(fp->label,LV_LABEL_ALIGN_CENTER);
	lv_obj_set_width(fp->label,lv_obj_get_width(fp->box));
	lv_label_set_text(fp->label,fp->text);
	lv_obj_set_pos(fp->label,0,box_h);
	if(lv_obj_get_height(fp->label)>lh){
		lv_label_set_long_mode(fp->label,LV_LABEL_LONG_DOT);
		lv_obj_set_height(fp->label,lh);
	}
	box_h+=lv_obj_get_height(fp->label);

	box_h+=gui_dpi/12;
	fp->view=lv_page_create(fp->box,NULL);
	lv_color_t c=lv_obj_get_style_border_color(fp->view,LV_PAGE_PART_BG);
	lv_obj_set_style_local_border_color(fp->view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,c);
	lv_obj_set_style_local_border_color(fp->view,LV_PAGE_PART_BG,LV_STATE_PRESSED,c);
	lv_obj_set_style_local_border_color(fp->view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,c);
	lv_obj_set_size(fp->view,lv_obj_get_width(fp->box)-gui_font_size,xh);
	lv_obj_set_pos(fp->view,gui_font_size/2,box_h);
	box_h+=xh;

	fp->fv=fileview_create(fp->view);
	fileview_set_margin(fp->fv,0);
	fileview_set_verbose(fp->fv,false);
	fileview_set_item_height(fp->fv,gui_dpi/3);
	fileview_set_path(fp->fv,fp->path);
	fileview_set_on_item_select(fp->fv,on_item_select);
	fileview_set_on_change_dir(fp->fv,on_change_dir);
	fileview_set_data(fp->fv,fp);

	box_h+=gui_dpi/12;
	fp->cur_path=lv_label_create(fp->box,NULL);
	lv_label_set_long_mode(fp->cur_path,LV_LABEL_LONG_CROP);
	lv_label_set_align(fp->cur_path,LV_LABEL_ALIGN_RIGHT);
	lv_label_set_text(fp->cur_path,fp->path);
	lv_obj_set_width(fp->cur_path,lv_obj_get_width(fp->box)-gui_font_size);
	lv_obj_set_small_text_font(fp->cur_path,LV_LABEL_PART_MAIN);
	lv_obj_set_pos(fp->cur_path,gui_font_size/2,box_h);
	box_h+=lv_obj_get_height(fp->cur_path);

	lv_coord_t
		bm=gui_font_size/2,
		b1w=lv_obj_get_width(fp->box)/3,
		b2w=lv_obj_get_width(fp->box)/2,
		bh=gui_font_size+(gui_dpi/8);

	box_h+=bm;
	fp->reload=lv_btn_create(fp->box,NULL);
	lv_label_set_text(lv_label_create(fp->reload,NULL),LV_SYMBOL_REFRESH);
	lv_obj_set_style_local_margin_bottom(fp->reload,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fp->reload,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fp->reload,b1w-bm,bh);
	lv_obj_set_user_data(fp->reload,fp);
	lv_obj_set_event_cb(fp->reload,btn_click);
	lv_obj_set_pos(fp->reload,bm/2,box_h);

	fp->new=lv_btn_create(fp->box,NULL);
	lv_obj_set_enabled(fp->new,!fsext_is_multi);
	lv_label_set_text(lv_label_create(fp->new,NULL),LV_SYMBOL_PLUS);
	lv_obj_set_style_local_margin_bottom(fp->new,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fp->new,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fp->new,b1w-bm,bh);
	lv_obj_set_user_data(fp->new,fp);
	lv_obj_set_event_cb(fp->new,btn_click);
	lv_obj_set_pos(fp->new,bm/2+b1w,box_h);

	fp->home=lv_btn_create(fp->box,NULL);
	lv_label_set_text(lv_label_create(fp->home,NULL),LV_SYMBOL_HOME);
	lv_obj_set_style_local_margin_bottom(fp->home,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fp->home,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fp->home,b1w-bm,bh);
	lv_obj_set_user_data(fp->home,fp);
	lv_obj_set_event_cb(fp->home,btn_click);
	lv_obj_set_pos(fp->home,bm/2+b1w*2,box_h);
	box_h+=bh;

	box_h+=bm;
	fp->ok=lv_btn_create(fp->box,NULL);
	lv_obj_set_enabled(fp->ok,false);
	lv_label_set_text(lv_label_create(fp->ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_style_local_margin_bottom(fp->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fp->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fp->ok,b2w-bm,bh);
	lv_obj_set_user_data(fp->ok,fp);
	lv_obj_set_event_cb(fp->ok,filepicker_click);
	lv_obj_set_pos(fp->ok,bm/2,box_h);

	fp->cancel=lv_btn_create(fp->box,NULL);
	lv_label_set_text(lv_label_create(fp->cancel,NULL),LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_margin_bottom(fp->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,bm);
	lv_obj_set_style_local_radius(fp->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(fp->cancel,b2w-bm,bh);
	lv_obj_set_user_data(fp->cancel,fp);
	lv_obj_set_event_cb(fp->cancel,filepicker_click);
	lv_obj_set_pos(fp->cancel,bm/2+b2w,box_h);
	box_h+=bh;

	box_h+=gui_dpi/8;
	lv_obj_set_height(fp->box,box_h);
	lv_obj_align(fp->box,NULL,LV_ALIGN_CENTER,0,0);

	return 0;
}

static int filepicker_clean(struct gui_activity*d){
	struct filepicker*box=d->args;
	if(!box)return 0;
	free(box);
	d->args=NULL;
	return 0;
}

static int filepicker_get_focus(struct gui_activity*d){
	struct filepicker*box=d->args;
	if(!box)return 0;
	fileview_add_group(box->fv,gui_grp);
	lv_group_add_obj(gui_grp,box->reload);
	lv_group_add_obj(gui_grp,box->new);
	lv_group_add_obj(gui_grp,box->home);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int filepicker_lost_focus(struct gui_activity*d){
	struct filepicker*box=d->args;
	if(!box)return 0;
	fileview_remove_group(box->fv);
	lv_group_remove_obj(box->reload);
	lv_group_remove_obj(box->new);
	lv_group_remove_obj(box->home);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

struct gui_register guireg_filepicker={
	.name="filepicker",
	.title="File Picker",
	.show_app=false,
	.draw=filepicker_draw,
	.quiet_exit=filepicker_clean,
	.get_focus=filepicker_get_focus,
	.lost_focus=filepicker_lost_focus,
	.back=true,
	.mask=true,
};

static void filepicker_cb(lv_task_t*t){
	guiact_start_activity(&guireg_filepicker,t->user_data);
}

struct filepicker*filepicker_create(filepicker_callback callback,const char*title,...){
	struct filepicker*fp=malloc(sizeof(struct filepicker));
	if(!fp)return NULL;
	memset(fp,0,sizeof(struct filepicker));
	if(title){
		va_list va;
		va_start(va,title);
		vsnprintf(fp->text,sizeof(fp->text)-1,_(title),va);
		va_end(va);
	}
	strcpy(fp->path,"/");
	fp->callback=callback;
	lv_task_once(lv_task_create(filepicker_cb,0,LV_TASK_PRIO_LOWEST,fp));
	return fp;
}

void filepicker_set_title(struct filepicker*fp,const char*title,...){
	if(!fp)return;
	memset(fp->text,0,sizeof(fp->text));
	if(!title)return;
	va_list va;
	va_start(va,title);
	vsnprintf(fp->text,sizeof(fp->text)-1,_(title),va);
	va_end(va);
}

void filepicker_set_user_data(struct filepicker*fp,void*user_data){
	if(!fp)return;
	fp->user_data=user_data;
}

void filepicker_set_max_item(struct filepicker*fp,uint16_t max){
	if(!fp)return;
	fp->max=max;
}

void filepicker_set_path(struct filepicker*fp,const char*path,...){
	if(!fp)return;
	memset(fp->path,0,PATH_MAX);
	if(!path||!*path){
		strcpy(fp->path,"/");
		return;
	}
	va_list va;
	va_start(va,path);
	vsnprintf(fp->path,PATH_MAX-1,path,va);
	va_end(va);
}
#endif

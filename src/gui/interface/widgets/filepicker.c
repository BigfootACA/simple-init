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
#include"confd.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/fileview.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/filepicker.h"

struct filepicker{
	char text[BUFSIZ];
	char path[PATH_MAX];
	char disp_path[PATH_MAX];
	filepicker_callback callback;
	lv_obj_t*box,*label;
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

static void filepicker_click(lv_event_t*e){
	struct filepicker*fp=e->user_data;
	if(guiact_get_last()->args!=fp)return;
	void*dp;
	uint16_t cnt=0,i;
	size_t ss,vs,cs,as;
	char**fn=NULL,**name,*path;
	if(e->target==fp->ok){
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
	else if(fp->callback(
		e->target==fp->ok,
		(const char**)fn,cnt,fp->user_data
	))return;
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

static void btn_click(lv_event_t*e){
	struct filepicker*fp=e->user_data;
	if(guiact_get_last()->args!=fp)return;
	if(e->target==fp->reload)fileview_set_path(fp->fv,NULL);
	else if(e->target==fp->home)fileview_set_path(fp->fv,"/");
	else if(e->target==fp->new){
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
	confd_set_string("gui.filepicker.dir",new);
	lv_label_set_text(fp->cur_path,new);
	lv_obj_set_enabled(fp->new,!fsext_is_multi||!fileview_is_top(fv));
	memset(fp->disp_path,0,sizeof(fp->disp_path));
	strncpy(fp->disp_path,new,sizeof(fp->disp_path)-1);
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
	if(cnt==fp->max)lv_group_focus_obj(fp->ok);
}

static bool on_item_click(struct fileview*fv,char*item,enum item_type type){
	if(type!=TYPE_DIR){
		struct filepicker*fp=fileview_get_data(fv);
		uint16_t cnt=fileview_get_checked_count(fv);
		if(cnt<fp->max)fileview_check_item(fv,item,true);
	}
	return true;
}

static int filepicker_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		0,LV_GRID_FR(1),0,0,0,
		LV_GRID_TEMPLATE_LAST
	};
	struct filepicker*fp=act->args;
	if(grid_row[0]==0)grid_row[0]=gui_font_size;
	if(grid_row[2]==0)grid_row[2]=gui_font_size;
	if(grid_row[3]==0)grid_row[3]=gui_font_size*1.5;
	if(grid_row[4]==0)grid_row[4]=gui_font_size*1.5;
	fp->act=act;

	fp->box=lv_obj_create(act->page);
	lv_obj_set_style_pad_all(fp->box,gui_font_size,0);
	lv_obj_set_style_pad_row(fp->box,gui_font_size,0);
	lv_obj_set_style_max_width(fp->box,lv_pct(80),0);
	lv_obj_set_style_max_height(fp->box,lv_pct(80),0);
	lv_obj_set_style_min_width(fp->box,gui_dpi*2,0);
	lv_obj_set_grid_dsc_array(fp->box,grid_col,grid_row);
	lv_obj_set_height(fp->box,lv_pct(70));
	lv_obj_center(fp->box);

	fp->label=lv_label_create(fp->box);
	lv_obj_set_style_text_align(fp->label,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(fp->label,LV_LABEL_LONG_WRAP);
	lv_label_set_text(fp->label,fp->text);
	lv_obj_set_grid_cell(
		fp->label,
		LV_GRID_ALIGN_STRETCH,0,6,
		LV_GRID_ALIGN_STRETCH,0,1
	);

	fp->view=lv_obj_create(fp->box);
	lv_obj_set_width(fp->view,lv_pct(100));
	lv_obj_set_grid_cell(
		fp->view,
		LV_GRID_ALIGN_STRETCH,0,6,
		LV_GRID_ALIGN_STRETCH,1,1
	);

	fp->fv=fileview_create(fp->view);
	fileview_set_verbose(fp->fv,false);
	fileview_set_on_item_click(fp->fv,on_item_click);
	fileview_set_on_item_select(fp->fv,on_item_select);
	fileview_set_on_change_dir(fp->fv,on_change_dir);
	fileview_set_data(fp->fv,fp);

	fp->cur_path=lv_label_create(fp->box);
	lv_obj_set_style_text_align(fp->cur_path,LV_TEXT_ALIGN_RIGHT,0);
	lv_label_set_long_mode(fp->cur_path,LV_LABEL_LONG_CLIP);
	lv_label_set_text(fp->cur_path,fp->path);
	lv_obj_set_small_text_font(fp->cur_path,LV_PART_MAIN);
	lv_obj_set_grid_cell(
		fp->cur_path,
		LV_GRID_ALIGN_STRETCH,0,6,
		LV_GRID_ALIGN_STRETCH,2,1
	);

	fp->reload=lv_btn_create(fp->box);
	lv_obj_t*lbl_reload=lv_label_create(fp->reload);
	lv_label_set_text(lbl_reload,LV_SYMBOL_REFRESH);
	lv_obj_center(lbl_reload);
	lv_obj_add_event_cb(fp->reload,btn_click,LV_EVENT_CLICKED,fp);
	lv_obj_set_grid_cell(
		fp->reload,
		LV_GRID_ALIGN_STRETCH,0,2,
		LV_GRID_ALIGN_STRETCH,3,1
	);

	fp->new=lv_btn_create(fp->box);
	lv_obj_set_enabled(fp->new,!fsext_is_multi);
	lv_obj_t*lbl_new=lv_label_create(fp->new);
	lv_label_set_text(lbl_new,LV_SYMBOL_PLUS);
	lv_obj_center(lbl_new);
	lv_obj_add_event_cb(fp->new,btn_click,LV_EVENT_CLICKED,fp);
	lv_obj_set_grid_cell(
		fp->new,
		LV_GRID_ALIGN_STRETCH,2,2,
		LV_GRID_ALIGN_STRETCH,3,1
	);

	fp->home=lv_btn_create(fp->box);
	lv_obj_t*lbl_home=lv_label_create(fp->home);
	lv_label_set_text(lbl_home,LV_SYMBOL_HOME);
	lv_obj_center(lbl_home);
	lv_obj_add_event_cb(fp->home,btn_click,LV_EVENT_CLICKED,fp);
	lv_obj_set_grid_cell(
		fp->home,
		LV_GRID_ALIGN_STRETCH,4,2,
		LV_GRID_ALIGN_STRETCH,3,1
	);

	fp->ok=lv_btn_create(fp->box);
	lv_obj_set_enabled(fp->ok,false);
	lv_obj_t*lbl_ok=lv_label_create(fp->ok);
	lv_label_set_text(lbl_ok,LV_SYMBOL_OK);
	lv_obj_center(lbl_ok);
	lv_obj_add_event_cb(fp->ok,filepicker_click,LV_EVENT_CLICKED,fp);
	lv_obj_set_grid_cell(
		fp->ok,
		LV_GRID_ALIGN_STRETCH,0,3,
		LV_GRID_ALIGN_STRETCH,4,1
	);

	fp->cancel=lv_btn_create(fp->box);
	lv_obj_t*lbl_cancel=lv_label_create(fp->cancel);
	lv_label_set_text(lbl_cancel,LV_SYMBOL_CLOSE);
	lv_obj_center(lbl_cancel);
	lv_obj_add_event_cb(fp->cancel,filepicker_click,LV_EVENT_CLICKED,fp);
	lv_obj_set_grid_cell(
		fp->cancel,
		LV_GRID_ALIGN_STRETCH,3,3,
		LV_GRID_ALIGN_STRETCH,4,1
	);

	return 0;
}

static int filepicker_clean(struct gui_activity*d){
	struct filepicker*box=d->args;
	if(!box)return 0;
	fileview_free(box->fv);
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

static int filepicker_load(struct gui_activity*d){
	struct filepicker*box=d->args;
	if(!box)return 0;
	if(!box->disp_path[0])strncpy(
		box->disp_path,box->path,
		sizeof(box->disp_path)-1
	);
	fileview_set_path(box->fv,box->disp_path);
	return 0;
}

static int do_back(struct gui_activity*d __attribute__((unused))){
	struct filepicker*box=d->args;
	if(!box)return 0;
	if(!fileview_is_top(box->fv)){
		fileview_go_back(box->fv);
		return 1;
	}
	return 0;
}

struct gui_register guireg_filepicker={
	.name="filepicker",
	.title="File Picker",
	.show_app=false,
	.draw=filepicker_draw,
	.ask_exit=do_back,
	.quiet_exit=filepicker_clean,
	.get_focus=filepicker_get_focus,
	.lost_focus=filepicker_lost_focus,
	.data_load=filepicker_load,
	.back=true,
	.mask=true,
	.allow_exclusive=true,
};

static void filepicker_cb(lv_timer_t*t){
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
	char*p=confd_get_string("gui.filepicker.dir",NULL);
	strcpy(fp->path,p?p:"/");
	if(p)free(p);
	fp->callback=callback;
	lv_timer_set_repeat_count(lv_timer_create(filepicker_cb,0,fp),1);
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

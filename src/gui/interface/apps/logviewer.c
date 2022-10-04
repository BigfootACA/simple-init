/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<math.h>
#include"str.h"
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#define TAG "logviewer"

struct log_viewer{
	bool load;
	list*file;
	uint16_t per_page,page_cnt,page_cur;
	lv_obj_t*txt,*view,*content;
	lv_obj_t*pager,*arr_top,*arr_left;
	lv_obj_t*page,*slider,*arr_right,*arr_bottom;
	lv_obj_t*btns,*btn_ctrl,*btn_reload;
};

static void update_slider(struct log_viewer*v){
	int16_t val=lv_slider_get_value(v->slider);
	lv_label_set_text_fmt(
		v->page,"%d/%d",
		MIN(v->page_cnt,val+1),
		v->page_cnt
	);
}

static void calc_pages(struct log_viewer*v){
	int cnt;
	int32_t max;
	uint16_t p=0;
	lv_obj_update_layout(v->view);
	const lv_font_t*f=lv_obj_get_style_text_font(v->content,0);
	if(f)p=lv_obj_get_content_height(v->view)/lv_font_get_line_height(f);
	if(p<=0)p=64;
	v->per_page=confd_get_integer("gui.logviewer.per_page",p*3);
	if(v->per_page<64)v->per_page=64;
	if((cnt=list_count(v->file))>=0){
		v->page_cnt=ceil((double)cnt/(double)v->per_page);
		max=MIN(MAX(v->page_cnt-1,1),INT16_MAX);
	}else max=1,v->page_cnt=0;
	if(v->page_cur>v->page_cnt)v->page_cur=v->page_cnt;
	if(v->page_cur<1)v->page_cur=1;
	lv_slider_set_range(v->slider,0,max);
	update_slider(v);
}

static bool load_file(struct log_viewer*v){
	#ifdef ENABLE_UEFI
	list*l=NULL;
	#else
	FILE*f=NULL;
	size_t bs=0,len;
	#endif
	size_t xs=8192,s;
	char*buff=NULL,*xb=NULL;
	if(v->load)return true;
	if(v->file){
		list_free_all_def(v->file);
		v->file=NULL;
	}
	if(!(xb=malloc(xs)))EDONE();
	#ifdef ENABLE_UEFI
	if((l=list_first(logbuffer)))do{
		bool changed=false;
		LIST_DATA_DECLARE(b,l,struct log_buff*);
		if(!b||!b->tag||!b->content)continue;
		s=8;
		if(b->tag[0])s+=strlen(b->tag);
		if(b->content[0])s+=strlen(b->content);
		if(s>xs){
			xs=s,free(xb);
			if(!(xb=malloc(xs)))EDONE();
		}
		memset(xb,0,xs);
		if(b->tag[0]){
			strlcat(xb,b->tag,xs-1);
			strlcat(xb,": ",xs-1);
			changed=true;
		}
		if(b->content[0]){
			strlcat(xb,b->content,xs-1);
			changed=true;
		}
		if(changed)list_obj_add_new_strdup(
			&v->file,xb
		);
	}while((l=l->next));
	#else
	if(!(f=fopen(_PATH_DEV"/logger.log","r")))
		EDONE(telog_warn("open logger.log failed"));
	while(getline(&buff,&bs,f)>=0){
		if(!buff)continue;
		len=strlen(buff),s=len+(strcnt(buff,"\t")*8)+1;
		if(s>xs){
			xs=s,free(xb);
			if(!(xb=malloc(xs)))EDONE();
		}
		memset(xb,0,xs);
		tlog_debug("new line");
		for(size_t i=0,p=0;i<len&&p<xs;i++)switch(buff[i]){
			case '\t':
				for(size_t x=8,t=p;x>t%8;x--)
					xb[p++]=' ';
			break;
			case '\n':case '\r':break;
			default:xb[p++]=buff[i];
		}
		if(buff)free(buff);
		buff=NULL,bs=0;
		if(xb[0])list_obj_add_new_strdup(
			&v->file,xb
		);

	}
	fclose(f);
	#endif
	v->load=true;
	free(xb);
	calc_pages(v);
	return true;
	done:
	#ifndef ENABLE_UEFI
	if(f)fclose(f);
	#endif
	if(xb)free(xb);
	if(buff)free(buff);
	list_free_all_def(v->file);
	v->file=NULL;
	return false;
}

static void load_log_task(void*data){
	list*l=NULL;
	char*buff=NULL;
	size_t start=0,end=0,i,size;
	struct log_viewer*v=data;
	if(!load_file(v))goto fail;
	if(v->per_page<=0||v->page_cur<=0)goto fail;
	if(v->page_cur>v->page_cnt)v->page_cur=v->page_cnt;
	if(v->page_cur<1)v->page_cur=1;
	start=(v->page_cur-1)*v->per_page;
	end=v->page_cur*v->per_page;
	size=4,i=0;
	if((l=list_first(v->file)))do{
		LIST_DATA_DECLARE(b,l,char*);
		if(b&&i>=start&&i<end)
			size+=strlen(b)+4;
		i++;
	}while((l=l->next));
	if(!(buff=malloc(size)))goto fail;
	memset(buff,0,size);
	i=0;
	if((l=list_first(v->file)))do{
		LIST_DATA_DECLARE(b,l,char*);
		if(b&&i>=start&&i<end){
			strlcat(buff,b,size-1);
			strlcat(buff,"\n",size-1);
		}
		i++;
	}while((l=l->next));
	lv_obj_scroll_to(v->view,0,0,LV_ANIM_OFF);
	lv_label_set_text(v->content,buff);
	return;
	fail:
	lv_label_set_text_fmt(
		v->content,
		_("Load log failed: %s"),
		_(strerror(errno))
	);
}

static void load_log(struct log_viewer*v){
	lv_async_call(load_log_task,v);
}

static void pager_change(lv_event_t*e){
	struct log_viewer*v=e->user_data;
	v->page_cur=lv_slider_get_value(v->slider)+1;
	update_slider(v);
	load_log(v);
}

static void pager_click(lv_event_t*e){
	struct log_viewer*v=e->user_data;
	int16_t val=lv_slider_get_value(v->slider);
	int16_t max=lv_slider_get_max_value(v->slider);
	if(e->target==v->arr_left)val--;
	else if(e->target==v->arr_right)val++;
	else if(e->target==v->arr_top)val=0;
	else if(e->target==v->arr_bottom)val=max;
	else return;
	lv_slider_set_value(v->slider,val,LV_ANIM_ON);
	pager_change(e);
}

static void reload_click(lv_event_t*e){
	struct log_viewer*v=e->user_data;
	v->load=false;
	load_log(v);
}

static void ctrl_click(lv_event_t*e __attribute__((unused))){
	ctrl_pad_show();
}

static int logviewer_get_focus(struct gui_activity*d){
	struct log_viewer*v=d->data;
	if(!v)return 0;
	lv_group_add_obj(gui_grp,v->arr_top);
	lv_group_add_obj(gui_grp,v->arr_left);
	lv_group_add_obj(gui_grp,v->slider);
	lv_group_add_obj(gui_grp,v->arr_right);
	lv_group_add_obj(gui_grp,v->arr_bottom);
	lv_group_add_obj(gui_grp,v->btn_ctrl);
	lv_group_add_obj(gui_grp,v->btn_reload);
	ctrl_pad_set_target(v->view);
	return 0;
}

static int logviewer_lost_focus(struct gui_activity*d){
	struct log_viewer*v=d->data;
	if(!v)return 0;
	lv_group_remove_obj(v->arr_top);
	lv_group_remove_obj(v->arr_left);
	lv_group_remove_obj(v->slider);
	lv_group_remove_obj(v->arr_right);
	lv_group_remove_obj(v->arr_bottom);
	lv_group_remove_obj(v->btn_ctrl);
	lv_group_remove_obj(v->btn_reload);
	ctrl_pad_set_target(NULL);
	return 0;
}

static int logviewer_resize(struct gui_activity*d){
	struct log_viewer*v=d->data;
	if(!v)return 0;
	calc_pages(v);
	return 0;
}

static int do_clean(struct gui_activity*act){
	struct log_viewer*v=act->data;
	if(!v)return 0;
	list_free_all_def(v->file);
	free(v);
	act->data=NULL;
	return 0;
}

static int logviewer_init(struct gui_activity*act){
	struct log_viewer*v;
	static size_t s=sizeof(struct log_viewer);
	if(!(v=malloc(s)))return -1;
	memset(v,0,s);
	act->data=v;
	return 0;
}

static int logviewer_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_FR(2),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(2),
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	struct log_viewer*v=act->data;
	if(!v)return -1;
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	v->txt=lv_label_create(act->page);
	lv_label_set_text(v->txt,_("Loggerd Viewer"));
	lv_obj_set_size(v->txt,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(v->txt,gui_font_size/2,0);
	lv_obj_set_style_text_align(v->txt,LV_TEXT_ALIGN_CENTER,0);

	v->view=lv_obj_create(act->page);
	lv_obj_set_width(v->view,lv_pct(100));
	lv_obj_set_flex_grow(v->view,1);

	v->content=lv_label_create(v->view);
	lv_obj_set_style_text_font(v->content,gui_font_small,0);
	lv_obj_set_size(v->content,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
	lv_label_set_text(v->content,_("Loading..."));

	v->pager=lv_draw_line_wrapper(act->page,grid_col,grid_row);
	lv_obj_set_style_pad_column(v->pager,gui_font_size,0);

	lv_draw_buttons_arg(
		v->pager,
		#define BTN(tgt,title,cb,cp,cs,rp)&(struct button_dsc){\
			&tgt,true,title,cb,v,cp,cs,rp,1,NULL\
		}
		BTN(v->arr_top,    LV_SYMBOL_PREV,  pager_click,  0,1,0),
		BTN(v->arr_left,   LV_SYMBOL_LEFT,  pager_click,  1,1,0),
		BTN(v->arr_right,  LV_SYMBOL_RIGHT, pager_click,  6,1,0),
		BTN(v->arr_bottom, LV_SYMBOL_NEXT,  pager_click,  7,1,0),
		BTN(v->btn_ctrl,   _("Control"),    ctrl_click,   0,3,1),
		BTN(v->btn_reload, _("Reload"),     reload_click, 5,3,1),
		NULL
	);

	v->slider=lv_slider_create(v->pager);
	lv_obj_add_event_cb(
		v->slider,pager_change,
		LV_EVENT_VALUE_CHANGED,v
	);
	lv_slider_set_range(v->slider,0,1);
	lv_obj_set_grid_cell(
		v->slider,
		LV_GRID_ALIGN_STRETCH,2,4,
		LV_GRID_ALIGN_CENTER,0,1
	);

	v->page=lv_label_create(v->pager);
	lv_label_set_text(v->page,"0/0");
	lv_obj_set_grid_cell(
		v->page,
		LV_GRID_ALIGN_CENTER,3,2,
		LV_GRID_ALIGN_CENTER,1,1
	);

	return 0;
}

static int do_load(struct gui_activity*act){
	load_log(act->data);
	return 0;
}

struct gui_register guireg_logviewer={
	.name="logger-viewer",
	.title="Loggerd Viewer",
	.show_app=true,
	.quiet_exit=do_clean,
	.init=logviewer_init,
	.draw=logviewer_draw,
	.resize=logviewer_resize,
	.lost_focus=logviewer_lost_focus,
	.get_focus=logviewer_get_focus,
	.data_load=do_load,
	.back=true,
};
#endif

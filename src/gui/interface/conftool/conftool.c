/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include"str.h"
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "conftool"

static lv_obj_t*view,*scr,*info,*lbl_path;
static lv_obj_t*btn_add,*btn_reload,*btn_delete,*btn_edit,*btn_home,*btn_load,*btn_save;
static list*path=NULL,*items=NULL;
static lv_coord_t bm,bw,bh,si;

struct conf_item{
	bool parent;
	char name[255];
	enum conf_type type;
	lv_obj_t*btn,*lbl,*w_img,*img,*val,*size;
};

static char*get_string_path(char*sub){
	static char string[BUFSIZ],*p,*s,*e;
	if(!path)return sub?sub:strcpy(string,"/");
	memset(string,0,sizeof(string));
	p=string,s=p,e=p+BUFSIZ-1;
	list*o=list_first(path);
	if(o)do{
		if(p!=s)*p++='.';
		LIST_DATA_DECLARE(item,o,char*);
		strncpy(p,item,e-p-1);
		p+=strlen(item);
	}while((o=o->next));
	if(sub){
		if(p!=s)*p++='.';
		strncpy(p,sub,e-p-1);
	}
	return string;
}

static const char*get_icon(struct conf_item*ci){
	if(ci->parent)return "inode-parent";
	switch(ci->type){
		case TYPE_KEY:return "inode-dir";
		case TYPE_STRING:
		case TYPE_INTEGER:
		case TYPE_BOOLEAN:return "text-x-plain";
		default:return "unknown";
	}
}

static int clean_item(void*d){
	struct conf_item*i=(struct conf_item*)d;
	if(i){
		if(i->btn)lv_obj_del(i->btn);
		free(i);
	}
	return 0;
}

static void clean_view(){
	if(info)lv_obj_del(info);
	list_free_all(items,clean_item);
	items=NULL,info=NULL;
	lv_obj_set_enabled(btn_delete,false);
	lv_obj_set_enabled(btn_edit,false);
}

static void set_info(char*text){
	clean_view();
	if(info)lv_obj_del(info);
	info=lv_label_create(view);
	lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(info,lv_obj_get_width(view)-gui_font_size);
	lv_obj_set_style_text_align(info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(info,text);
}

extern struct gui_register guireg_conftool_create;
static void call_create(bool create,char*sub){
	static char xpath[PATH_MAX]={0};
	xpath[0]=create?0:1;
	strncpy(xpath+1,get_string_path(sub),PATH_MAX-2);
	guiact_start_activity(&guireg_conftool_create,xpath);
}

static void load_view();
static void go_back(){
	list_obj_del(&path,list_last(path),list_default_free);
	load_view();
}

static size_t get_selected(){
	size_t c=0;
	list*o=list_first(items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct conf_item*);
		if(item&&item->btn&&lv_obj_is_checked(item->btn))c++;
	}while((o=o->next));
	return c;
}

static void click_item(struct conf_item*ci){
	if(!ci)return;
	switch(ci->type){
		case TYPE_KEY:
			if(ci->parent&&path)go_back();
			else list_obj_add_new_strdup(&path,ci->name);
			load_view();
		break;
		case TYPE_STRING:
		case TYPE_INTEGER:
		case TYPE_BOOLEAN:
			call_create(false,ci->name);
		break;
	}
}

static void check_item(struct conf_item*ci,bool checked){
	if(!ci)return;
	if(ci->parent){
		go_back();
		return;
	}
	lv_obj_set_checked(ci->btn,checked);
	size_t c=get_selected();
	if(c==0){
		lv_obj_set_enabled(btn_delete,false);
		lv_obj_set_enabled(btn_edit,false);
	}else if(c==1){
		lv_obj_set_enabled(btn_delete,true);
		lv_obj_set_enabled(btn_edit,true);
	}else{
		lv_obj_set_enabled(btn_delete,true);
		lv_obj_set_enabled(btn_edit,false);
	}
}

static void item_click(lv_event_t*e){
	struct conf_item*ci=e->user_data;
	lv_indev_t*i=lv_indev_get_act();
	if(i&&i->proc.long_pr_sent)return;
	size_t c=get_selected();
	if(c>0)check_item(ci,!lv_obj_is_checked(ci->btn));
	else click_item(ci);
}

static void item_check(lv_event_t*e){
	struct conf_item*ci=e->user_data;
	if(ci->parent)return;
	e->stop_processing=1;
	check_item(ci,!lv_obj_is_checked(ci->btn));
}

static void add_item(bool parent,char*name){
	static lv_coord_t grid_col[]={
		0,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	char*p=get_string_path(name);
	enum conf_type type=parent?TYPE_KEY:confd_get_type(p);
	if(type<=0)return;
	struct conf_item*ci=malloc(sizeof(struct conf_item));
	if(!ci)return;
	memset(ci,0,sizeof(struct conf_item));
	if(name)strncpy(ci->name,name,sizeof(ci->name)-1);
	ci->type=type,ci->parent=parent;
	if(grid_col[0]==0)grid_col[0]=gui_font_size*3;

	// conf item button
	ci->btn=lv_btn_create(view);
	lv_obj_set_width(ci->btn,lv_pct(100));
	lv_obj_set_content_height(ci->btn,grid_col[0]);
	lv_style_set_btn_item(ci->btn);
	lv_obj_set_grid_dsc_array(ci->btn,grid_col,grid_row);
	lv_obj_add_event_cb(ci->btn,item_click,LV_EVENT_CLICKED,ci);
	lv_obj_add_event_cb(ci->btn,item_check,LV_EVENT_LONG_PRESSED,ci);
	lv_group_add_obj(gui_grp,ci->btn);
	if(list_obj_add_new(&items,ci)!=0){
		telog_error("cannot add conf item list");
		abort();
	}

	// conf image
	ci->w_img=lv_obj_create(ci->btn);
	lv_obj_set_size(ci->w_img,grid_col[0],grid_col[0]);
	lv_obj_clear_flag(ci->w_img,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(ci->w_img,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_border_width(ci->w_img,0,0);
	lv_obj_set_style_bg_opa(ci->w_img,LV_OPA_0,0);
	lv_obj_set_grid_cell(
		ci->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,2
	);
	ci->img=lv_img_create(ci->w_img);
	lv_img_src_try(ci->img,"mime",get_icon(ci),NULL);
	lv_img_set_size_mode(ci->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_fill_image(ci->img,grid_col[0],grid_col[0]);
	lv_obj_center(ci->img);

	// conf name
	ci->lbl=lv_label_create(ci->btn);
	lv_label_set_text(ci->lbl,parent?_("Parent key"):name);

	if(ci->type!=TYPE_KEY){
		lv_obj_set_grid_cell(
			ci->lbl,
			LV_GRID_ALIGN_START,1,1,
			LV_GRID_ALIGN_CENTER,0,1
		);
		ci->val=lv_label_create(ci->btn);
		lv_label_set_long_mode(ci->val,LV_LABEL_LONG_DOT);
		lv_obj_set_small_text_font(ci->val,LV_PART_MAIN);
		lv_obj_set_grid_cell(
			ci->val,
			LV_GRID_ALIGN_STRETCH,1,2,
			LV_GRID_ALIGN_CENTER,1,1
		);
		ci->size=lv_label_create(ci->btn);
		lv_obj_set_style_text_align(ci->size,LV_TEXT_ALIGN_RIGHT,0);
		lv_label_set_long_mode(ci->size,LV_LABEL_LONG_CLIP);
		lv_obj_set_grid_cell(
			ci->size,
			LV_GRID_ALIGN_STRETCH,2,1,
			LV_GRID_ALIGN_CENTER,0,1
		);
		char size[32]={0},*r;
		switch(type){
			case TYPE_KEY:break;
			case TYPE_STRING:
				if(!(r=confd_get_string(p,"")))break;
				lv_label_set_text(ci->val,r);
				lv_label_set_text(ci->size,
					make_readable_str_buf(size,31,strlen(r),1,0)
				);
				free(r);
			break;
			case TYPE_INTEGER:
				lv_label_set_text_fmt(ci->val,"%lld",(long long int)confd_get_integer(p,0));
				lv_label_set_text_fmt(ci->size,"%zu",sizeof(int64_t));
			break;
			case TYPE_BOOLEAN:
				lv_label_set_text(ci->val,BOOL2STR(confd_get_boolean(p,false)));
				lv_label_set_text_fmt(ci->size,"%zu",sizeof(bool));
			break;
		}
	}else lv_obj_set_grid_cell(
		ci->lbl,
		LV_GRID_ALIGN_START,1,2,
		LV_GRID_ALIGN_CENTER,0,2
	);
}

static void load_view(){
	size_t i;
	bm=gui_font_size;
	lv_obj_update_layout(view);
	bw=lv_obj_get_content_width(view);
	bh=gui_font_size*3,si=bh-gui_font_size;
	lv_label_set_text(lbl_path,get_string_path(NULL));
	clean_view();
	if(path)add_item(true,NULL);
	char*p=get_string_path(NULL);
	char**xs=confd_ls(p);
	if(!xs){
		if(!path)set_info(_("list items failed"));
		return;
	}
	for(i=0;xs[i];i++)add_item(false,xs[i]);
	if(i==0&&!path)set_info(_("nothing here"));
	tlog_debug("found %zu items in '%s'",i,p);
	if(xs[0])free(xs[0]);
	free(xs);
}

static int do_load(struct gui_activity*d){
	if(d->args){
		char buff[PATH_MAX],*cur=buff,*next=NULL;
		memset(buff,0,sizeof(buff));
		strncpy(buff,d->args,sizeof(buff)-1);
		list_free_all_def(path);
		path=NULL;
		do{
			if((next=strchr(cur,'.')))*next++=0;
			if(!next&&(next=strchr(cur,'/')))*next++=0;
			if(!next&&(next=strchr(cur,'\\')))*next++=0;
			if(*cur)list_obj_add_new_strdup(&path,cur);
		}while((cur=next));
		d->args=NULL;
	}
	load_view();
	return 0;
}

static int conftool_get_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct conf_item*);
		if(item->btn)lv_group_add_obj(gui_grp,item->btn);
	}while((o=o->next));
	lv_group_add_obj(gui_grp,btn_add);
	lv_group_add_obj(gui_grp,btn_reload);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_edit);
	lv_group_add_obj(gui_grp,btn_home);
	lv_group_add_obj(gui_grp,btn_save);
	lv_group_add_obj(gui_grp,btn_load);
	return 0;
}

static int conftool_lost_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct conf_item*);
		if(item->btn)lv_group_remove_obj(item->btn);
	}while((o=o->next));
	lv_group_remove_obj(btn_add);
	lv_group_remove_obj(btn_reload);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_edit);
	lv_group_remove_obj(btn_home);
	lv_group_remove_obj(btn_save);
	lv_group_remove_obj(btn_load);
	return 0;
}

static bool call_delete(uint16_t id,const char*text __attribute__((unused)),void*data __attribute__((unused))){
	if(id==0){
		list*o=list_first(items);
		if(o)do{
			LIST_DATA_DECLARE(item,o,struct conf_item*);
			if(!item||!item->name[0]||item->parent)continue;
			if(!item->btn||!lv_obj_is_checked(item->btn))continue;
			confd_delete(get_string_path(item->name));
		}while((o=o->next));
		load_view();
	}
	return false;
}

static bool config_save_cb(bool ok,const char**p,uint16_t cnt,void*user_data __attribute__((unused))){
	if(!ok)return false;
	if(!p||!p[0]||p[1]||cnt!=1)return true;
	guiact_start_activity_by_name("config-save",(void*)p[0]);
	return false;
}

static bool config_load_cb(bool ok,const char**p,uint16_t cnt,void*user_data __attribute__((unused))){
	if(!ok)return false;
	if(!p||!p[0]||p[1]||cnt!=1)return true;
	guiact_start_activity_by_name("config-load",(void*)p[0]);
	return false;
}

static void btns_cb(lv_event_t*e){
	if(strcmp(guiact_get_last()->name,"config-manager")!=0)return;
	tlog_info("click button %s",(char*)e->user_data);
	if(e->target==btn_add){
		call_create(true,NULL);
	}else if(e->target==btn_reload){
		load_view();
	}else if(e->target==btn_delete){
		msgbox_create_yesno(call_delete,"Are you sure you want to delete the selected items?");
	}else if(e->target==btn_edit){
		list*o=list_first(items);
		if(o)do{
			LIST_DATA_DECLARE(item,o,struct conf_item*);
			if(
				item&&item->btn&&!item->parent&&
				lv_obj_is_checked(item->btn)
			){
				call_create(false,get_string_path(item->name));
				break;
			}
		}while((o=o->next));
	}else if(e->target==btn_home){
		list_free_all_def(path);
		path=NULL;
		load_view();
	}else if(e->target==btn_save){
		filepicker_set_max_item(filepicker_create(config_save_cb,"Select file to save"),1);
	}else if(e->target==btn_load){
		filepicker_set_max_item(filepicker_create(config_load_cb,"Select file to load"),1);
	}
}

static int conftool_draw(struct gui_activity*act){
	scr=act->page;
	lv_obj_set_flex_flow(scr,LV_FLEX_FLOW_COLUMN);

	view=lv_obj_create(scr);
	lv_obj_set_width(view,lv_pct(100));
	lv_obj_set_style_border_width(view,0,0);
	lv_obj_set_flex_flow(view,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(view,1);

	// current path
	lbl_path=lv_label_create(scr);
	lv_obj_set_style_text_align(lbl_path,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(lbl_path,LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(lbl_path,lv_pct(100));
	lv_label_set_text(lbl_path,get_string_path(NULL));

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,data,title,x)&(struct button_dsc){\
			&tgt,en,_(title),btns_cb,data,x,1,0,1,NULL\
		}
		BTN(btn_add,    true,  "add",    LV_SYMBOL_PLUS,    0),
		BTN(btn_reload, true,  "reload", LV_SYMBOL_REFRESH, 1),
		BTN(btn_delete, false, "delete", LV_SYMBOL_TRASH,   2),
		BTN(btn_edit,   false, "edit",   LV_SYMBOL_EDIT,    3),
		BTN(btn_home,   true,  "home",   LV_SYMBOL_HOME,    4),
		BTN(btn_save,   true,  "save",   LV_SYMBOL_SAVE,    5),
		BTN(btn_load,   true,  "load",   LV_SYMBOL_UPLOAD,  6),
		#undef BTN
		NULL
	);

	return 0;
}

static int do_clean(struct gui_activity*d __attribute__((unused))){
	list_free_all_def(items);
	list_free_all_def(path);
	items=NULL,path=NULL,info=NULL;
	return 0;
}

static int do_back(struct gui_activity*d __attribute__((unused))){
	if(path){
		go_back();
		return 1;
	}
	return 0;
}

struct gui_register guireg_conftool={
	.name="config-manager",
	.title="Config Manager",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.get_focus=conftool_get_focus,
	.lost_focus=conftool_lost_focus,
	.draw=conftool_draw,
	.data_load=do_load,
	.back=true
};
#endif

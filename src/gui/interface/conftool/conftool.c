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
#define MIME_DIR _PATH_USR"/share/pixmaps/mime"
#define MIME_EXT ".svg"

static lv_obj_t*view,*scr,*info,*lbl_path,*last_btn;
static lv_obj_t*btn_add,*btn_reload,*btn_delete,*btn_edit,*btn_home,*btn_load,*btn_save;
static list*path=NULL,*items=NULL;
static lv_coord_t bm,bw,bh,si;
static lv_style_t img_s;

struct conf_item{
	bool parent;
	char name[255];
	enum conf_type type;
	lv_obj_t*btn,*chk,*img,*val,*size;
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
	items=NULL,info=NULL,last_btn=NULL;
	lv_obj_set_enabled(btn_delete,false);
	lv_obj_set_enabled(btn_edit,false);
}

static void set_info(char*text){
	clean_view();
	if(info)lv_obj_del(info);
	info=lv_label_create(view,NULL);
	lv_label_set_long_mode(info,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(info,lv_page_get_scrl_width(view)-gui_font_size);
	lv_label_set_align(info,LV_LABEL_ALIGN_CENTER);
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

static void click_item(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct conf_item*ci=(struct conf_item*)lv_obj_get_user_data(obj);
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

static void check_item(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	struct conf_item*ci=(struct conf_item*)lv_obj_get_user_data(obj);
	if(!ci)return;
	if(ci->parent){
		go_back();
		return;
	}
	(lv_checkbox_is_checked(obj)?
		lv_obj_add_state:
		lv_obj_clear_state
	)(ci->btn,LV_STATE_CHECKED);
	size_t c=0;
	list*o=list_first(items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct conf_item*);
		if(item&&item->chk&&lv_checkbox_is_checked(item->chk))c++;
	}while((o=o->next));
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

static void add_item(bool parent,char*name){
	char*p=get_string_path(name);
	enum conf_type type=parent?TYPE_KEY:confd_get_type(p);
	if(type<=0)return;
	struct conf_item*ci=malloc(sizeof(struct conf_item));
	if(!ci)return;
	memset(ci,0,sizeof(struct conf_item));
	if(name)strncpy(ci->name,name,sizeof(ci->name)-1);
	ci->type=type,ci->parent=parent;

	// conf item button
	ci->btn=lv_btn_create(view,NULL);
	lv_obj_set_size(ci->btn,bw,bh);
	lv_style_set_btn_item(ci->btn);
	lv_obj_set_click(ci->btn,false);
	lv_obj_align(
		ci->btn,last_btn,last_btn?
			 LV_ALIGN_OUT_BOTTOM_LEFT:
			 LV_ALIGN_IN_TOP_MID,
		0,bm/8+(last_btn?gui_dpi/20:0)
	);
	last_btn=ci->btn;
	if(list_obj_add_new(&items,ci)!=0){
		telog_error("cannot add conf item list");
		abort();
	}

	// line for button text
	lv_obj_t*line=lv_line_create(ci->btn,NULL);
	lv_obj_set_width(line,bw);

	// conf image
	ci->img=lv_img_create(line,NULL);
	lv_obj_set_size(ci->img,si,si);
	lv_obj_align(ci->img,ci->btn,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
	lv_obj_set_click(ci->img,true);
	lv_obj_set_event_cb(ci->img,click_item);
	lv_obj_set_user_data(ci->img,ci);
	lv_obj_add_style(ci->img,LV_IMG_PART_MAIN,&img_s);
	char ipath[PATH_MAX];
	memset(ipath,0,PATH_MAX);
	snprintf(ipath,PATH_MAX-1,MIME_DIR"/%s"MIME_EXT,get_icon(ci));
	lv_img_set_src(ci->img,ipath);
	lv_img_ext_t*ext=lv_obj_get_ext_attr(ci->img);
	if((ext->w<=0||ext->h<=0)){
		memset(ipath,0,PATH_MAX);
		strncpy(ipath,MIME_DIR"/inode-file"MIME_EXT,PATH_MAX-1);
		lv_img_set_src(ci->img,ipath);
		ext=lv_obj_get_ext_attr(ci->img);
	}
	if(ext->w>0&&ext->h>0)lv_img_set_zoom(ci->img,(int)(((float)si/MAX(ext->w,ext->h))*256));
	lv_img_set_pivot(ci->img,0,0);
	lv_group_add_obj(gui_grp,ci->img);

	// conf name and checkbox
	ci->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(ci->chk,parent?_("Parent key"):name);
	lv_style_set_focus_checkbox(ci->chk);
	lv_obj_set_event_cb(ci->chk,check_item);
	lv_obj_set_user_data(ci->chk,ci);
	lv_obj_align(
		ci->chk,NULL,
		LV_ALIGN_IN_LEFT_MID,
		gui_font_size+si,
		ci->type==TYPE_KEY?0:-gui_font_size
	);
	lv_group_add_obj(gui_grp,ci->chk);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(ci->chk);
	lv_label_set_long_mode(e->label,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT
	);

	lv_coord_t lbl_w=bw-si-gui_font_size*2-lv_obj_get_width(e->bullet);
	if(lv_obj_get_width(e->label)>lbl_w)lv_obj_set_width(e->label,lbl_w);

	if(ci->type!=TYPE_KEY){
		ci->val=lv_label_create(line,NULL);
		lv_label_set_long_mode(ci->val,LV_LABEL_LONG_DOT);
		lv_obj_set_width(ci->val,bw-si-(gui_font_size*2));
		lv_obj_set_small_text_font(ci->val,LV_LABEL_PART_MAIN);
		lv_obj_align(
			ci->val,NULL,LV_ALIGN_IN_LEFT_MID,
			gui_font_size+si,gui_font_size
		);
		ci->size=lv_label_create(line,NULL);
		lv_label_set_align(ci->size,LV_LABEL_ALIGN_RIGHT);
		lv_label_set_long_mode(ci->size,LV_LABEL_LONG_CROP);
		lv_coord_t xs=lv_obj_get_width(ci->size),min=bw/5;
		if(xs>min){
			lv_obj_set_width(ci->size,min);
			xs=min;
		}
		lv_obj_align(
			ci->size,ci->btn,LV_ALIGN_IN_TOP_RIGHT,
			-gui_font_size/2,gui_font_size/2
		);
		lbl_w-=xs+gui_dpi/20;
		if(lv_obj_get_width(e->label)>lbl_w)
			lv_obj_set_width(e->label,lbl_w);
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
				lv_label_set_text_fmt(ci->val,"%ld",confd_get_integer(p,0));
				lv_label_set_text_fmt(ci->size,"%ld",sizeof(int64_t));
			break;
			case TYPE_BOOLEAN:
				lv_label_set_text(ci->val,BOOL2STR(confd_get_boolean(p,false)));
				lv_label_set_text_fmt(ci->size,"%d",sizeof(bool));
			break;
		}
	}
}

static void load_view(){
	size_t i;
	bm=gui_font_size;
	bw=lv_page_get_scrl_width(view)-bm;
	bh=gui_dpi/2,si=bh-gui_font_size;
	lv_style_init(&img_s);
	lv_style_set_outline_width(&img_s,LV_STATE_FOCUSED,gui_dpi/100);
	lv_style_set_outline_color(&img_s,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_style_set_radius(&img_s,LV_STATE_FOCUSED,gui_dpi/50);
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

static int conftool_get_focus(struct gui_activity*d __attribute__((unused))){
	load_view();
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
		if(item->img)lv_group_remove_obj(item->img);
		if(item->chk)lv_group_remove_obj(item->chk);
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
			if(!item->chk||!lv_checkbox_is_checked(item->chk))continue;
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

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(strcmp(guiact_get_last()->name,"config-manager")!=0)return;
	tlog_info("click button %s",(char*)lv_obj_get_user_data(obj));
	if(obj==btn_add){
		call_create(true,NULL);
	}else if(obj==btn_reload){
		load_view();
	}else if(obj==btn_delete){
		msgbox_create_yesno(call_delete,"Are you sure you want to delete the selected items?");
	}else if(obj==btn_edit){
		list*o=list_first(items);
		if(o)do{
			LIST_DATA_DECLARE(item,o,struct conf_item*);
			if(
				item&&item->chk&&!item->parent&&
				lv_checkbox_is_checked(item->chk)
			){
				call_create(false,get_string_path(item->name));
				break;
			}
		}while((o=o->next));
	}else if(obj==btn_home){
		list_free_all_def(path);
		path=NULL;
		load_view();
	}else if(obj==btn_save){
		filepicker_set_max_item(filepicker_create(config_save_cb,"Select file to save"),1);
	}else if(obj==btn_load){
		filepicker_set_max_item(filepicker_create(config_load_cb,"Select file to load"),1);
	}
}

static int conftool_draw(struct gui_activity*act){
	lv_coord_t btx=gui_font_size,btm=btx/2,btw=(gui_sw-btm)/7-btm,bth=btx*2;
	scr=act->page;

	view=lv_page_create(scr,NULL);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_width(view,gui_sw);

	// current path
	lbl_path=lv_label_create(scr,NULL);
	lv_label_set_align(lbl_path,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(lbl_path,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_size(lbl_path,gui_sw,gui_dpi/7);
	lv_obj_align(lbl_path,NULL,LV_ALIGN_IN_BOTTOM_LEFT,0,-bth-btm*3);
	lv_obj_set_size(view,gui_sw,lv_obj_get_y(lbl_path));
	lv_label_set_text(lbl_path,get_string_path(NULL));

	lv_obj_t*line=lv_obj_create(act->page,NULL);
	lv_obj_set_size(line,gui_sw,gui_dpi/100);
	lv_obj_align(line,lbl_path,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_theme_apply(line,LV_THEME_SCR);
	lv_obj_set_style_local_bg_color(
		line,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,
		lv_color_darken(lv_obj_get_style_bg_color(line,LV_OBJ_PART_MAIN),LV_OPA_20)
	);

	btn_add=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_add,btw,bth);
	lv_obj_set_event_cb(btn_add,btns_cb);
	lv_obj_set_user_data(btn_add,"add");
	lv_obj_align(btn_add,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(btn_add,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_add,NULL),LV_SYMBOL_PLUS);

	btn_reload=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_reload,btw,bth);
	lv_obj_set_event_cb(btn_reload,btns_cb);
	lv_obj_set_user_data(btn_reload,"reload");
	lv_obj_align(btn_reload,btn_add,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_reload,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_reload,NULL),LV_SYMBOL_REFRESH);

	btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_delete,false);
	lv_obj_set_size(btn_delete,btw,bth);
	lv_obj_set_event_cb(btn_delete,btns_cb);
	lv_obj_set_user_data(btn_delete,"delete");
	lv_obj_align(btn_delete,btn_reload,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_delete,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_delete,NULL),LV_SYMBOL_TRASH);

	btn_edit=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_edit,false);
	lv_obj_set_size(btn_edit,btw,bth);
	lv_obj_set_event_cb(btn_edit,btns_cb);
	lv_obj_set_user_data(btn_edit,"edit");
	lv_obj_align(btn_edit,btn_delete,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_edit,NULL),LV_SYMBOL_EDIT);

	btn_home=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_home,btw,bth);
	lv_obj_set_event_cb(btn_home,btns_cb);
	lv_obj_set_user_data(btn_home,"home");
	lv_obj_align(btn_home,btn_edit,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_home,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_home,NULL),LV_SYMBOL_HOME);

	btn_save=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_save,btw,bth);
	lv_obj_set_event_cb(btn_save,btns_cb);
	lv_obj_set_user_data(btn_save,"save");
	lv_obj_align(btn_save,btn_home,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_save,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_save,NULL),LV_SYMBOL_SAVE);

	btn_load=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_load,btw,bth);
	lv_obj_set_event_cb(btn_load,btns_cb);
	lv_obj_set_user_data(btn_load,"load");
	lv_obj_align(btn_load,btn_save,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_load,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_load,NULL),LV_SYMBOL_UPLOAD);

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
	.icon="conftool.svg",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.get_focus=conftool_get_focus,
	.lost_focus=conftool_lost_focus,
	.draw=conftool_draw,
	.back=true
};
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"array.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/filetab.h"
#include"gui/fileopen.h"
#include"gui/inputbox.h"
#include"gui/activity.h"
#define TAG "filemgr"

static struct fm_button{
	char*name;
	char*symbol;
	bool enabled;
	lv_obj_t*btn;
	lv_obj_t*lbl;
}fm_btns[]={
	{ .name="prev",    .symbol=LV_SYMBOL_LEFT,      .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="refresh", .symbol=LV_SYMBOL_REFRESH,   .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="edit",    .symbol=LV_SYMBOL_EDIT,      .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="home",    .symbol=LV_SYMBOL_HOME,      .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="info",    .symbol=LV_SYMBOL_LIST,      .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="next",    .symbol=LV_SYMBOL_RIGHT,     .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="paste",   .symbol=LV_SYMBOL_PASTE,     .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="copy",    .symbol=LV_SYMBOL_COPY,      .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="delete",  .symbol=LV_SYMBOL_TRASH,     .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="new",     .symbol=LV_SYMBOL_PLUS,      .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="cut",     .symbol=LV_SYMBOL_CUT,       .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="back",    .symbol=LV_SYMBOL_BACKSPACE, .enabled=true,  .btn=NULL, .lbl=NULL },
};
static lv_obj_t*tabview,*scr,*path;
static struct filetab*tabs[4],*active=NULL;

static void set_btn_enabled(char*name,bool enabled){
	for(size_t i=0;i<ARRLEN(fm_btns);i++)
		if(strcmp(fm_btns[i].name,name)==0)
			lv_obj_set_enabled(fm_btns[i].btn,enabled);
}

static int filemgr_get_focus(struct gui_activity*d __attribute__((unused))){
	for(size_t i=0;i<ARRLEN(fm_btns);i++)
		lv_group_add_obj(gui_grp,fm_btns[i].btn);
	if(active)filetab_add_group(active,gui_grp);
	return 0;
}

static int filemgr_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(size_t i=0;i<ARRLEN(fm_btns);i++)
		lv_group_remove_obj(fm_btns[i].btn);
	if(active)filetab_remove_group(active);
	return 0;
}

static void update_active(){
	struct filetab*old=active;
	filetab_remove_group(active);
	active=NULL;
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(!filetab_is_active(tabs[i]))continue;
		active=tabs[i];
		filetab_add_group(active,gui_grp);
		lv_label_set_text(path,filetab_get_path(active));
		uint16_t id=filetab_get_id(active);
		confd_set_integer("gui.filemgr.active",id);
		if(old!=active)tlog_debug("switch to tab %d",id);
		break;
	}
}

int do_cleanup(struct gui_activity*d __attribute__((unused))){
	active=NULL;
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(tabs[i])filetab_free(tabs[i]);
		tabs[i]=NULL;
	}
	return 0;
}

static void tabview_cb(lv_event_t*e __attribute__((unused))){
	update_active();
}

static void on_change_dir(struct filetab*fv,char*old __attribute__((unused)),char*new){
	if(!filetab_is_active(fv))return;
	lv_label_set_text(path,new);
	size_t tab=0;
	for(tab=0;tab<ARRLEN(tabs);tab++)if(tabs[tab]==fv)break;
	confd_set_string_array("gui.filemgr.tab",(int)tab,"dir",new);
}

static bool on_item_click(struct filetab*fv,char*item,enum item_type type){
	if(!filetab_is_active(fv)||type==TYPE_DIR)return true;
	char full_path[PATH_MAX]={0};
	char*parent=filetab_get_lvgl_path(fv);
	char*x=parent+strlen(parent)-1;
	if(*x=='/')*x=0;
	snprintf(full_path,PATH_MAX-1,"%s/%s",parent,item);
	fileopen_open(full_path);
	return true;
}

static void on_item_select(
	struct filetab*fv,
	char*name __attribute__((unused)),
	enum item_type type __attribute__((unused)),
	bool checked __attribute__((unused)),
	uint16_t cnt
){
	if(fv!=active)return;
	if(fsext_is_multi&&filetab_is_top(fv))return;
	if(cnt==1){
		set_btn_enabled("edit",true);
		set_btn_enabled("info",true);
	}else{
		set_btn_enabled("edit",false);
		set_btn_enabled("info",false);
	}
	if(cnt>0){
		set_btn_enabled("cut",true);
		set_btn_enabled("copy",true);
		set_btn_enabled("delete",true);
	}else{
		set_btn_enabled("cut",false);
		set_btn_enabled("copy",false);
		set_btn_enabled("delete",false);
	}
}

static void tabview_create(){
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(tabs[i])continue;
		if(!(tabs[i]=filetab_create(tabview,NULL))){
			tlog_error("cannot allocate filetab");
			abort();
		}
		filetab_set_on_item_select(tabs[i],on_item_select);
		filetab_set_on_item_click(tabs[i],on_item_click);
		filetab_set_on_change_dir(tabs[i],on_change_dir);
		filetab_set_show_parent(tabs[i],false);
		char*p=confd_get_string_array("gui.filemgr.tab",(int)i,"dir",NULL);
		filetab_set_path(tabs[i],p?p:"/");
		if(p)free(p);
	}
	lv_tabview_set_act(
		tabview,
		confd_get_integer("gui.filemgr.active",0),
		LV_ANIM_OFF
	);
	update_active();
}

static int do_back(struct gui_activity*d __attribute__((unused))){
	lv_obj_t*tab=filetab_get_tab(active);
	if(!tab)return 0;
	if(lv_obj_get_scroll_y(tab)!=0){
		lv_obj_scroll_to_y(tab,0,LV_ANIM_ON);
		return 1;
	}
	if(!filetab_is_top(active)){
		filetab_go_back(active);
		return 1;
	}
	return 0;
}

static bool create_name_cb(bool ok,const char*name,void*user_data){
	if(!ok)return false;
	char*fp=filetab_get_lvgl_path(active);
	char xp[PATH_MAX]={0};
	snprintf(xp,PATH_MAX-1,"%s/%s",fp,name);
	lv_res_t r;
	switch(*(uint16_t*)user_data){
		case 0:r=lv_fs_creat(xp);break;//file
		case 1:r=lv_fs_mkdir(xp);break;//folder
		default:return false;
	}
	filetab_set_path(active,NULL);
	if(r!=LV_FS_RES_OK)msgbox_alert(
		"Create '%s' failed: %s",
		name,lv_fs_res_to_i18n_string(r)
	);
	return false;
}

static bool create_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data __attribute__((unused))){
	static uint16_t xid;
	xid=id;
	struct inputbox*in=inputbox_create(create_name_cb,"Create item name");
	inputbox_set_user_data(in,&xid);
	return false;
}

static bool rename_cb(bool ok,const char*name,void*user_data __attribute__((unused))){
	if(!ok||filetab_get_checked_count(active)!=1)return false;
	char*buff=malloc(PATH_MAX*3);
	if(!buff)return true;
	memset(buff,0,PATH_MAX*3);
	char*oldname=buff;
	char*oldpath=oldname+PATH_MAX;
	char*newpath=oldpath+PATH_MAX;
	char*fp=filetab_get_lvgl_path(active);
	char**sel=filetab_get_checked(active);
	if(!sel||!sel[0]||sel[1]){
		if(sel)free(sel);
		msgbox_alert("File select invalid");
		free(buff);
		return true;
	}
	strncpy(oldname,sel[0],PATH_MAX-1);
	snprintf(oldpath,PATH_MAX-1,"%s/%s",fp,sel[0]);
	snprintf(newpath,PATH_MAX-1,"%s/%s",fp,name);
	lv_res_t r=lv_fs_rename(oldpath,newpath);
	filetab_set_path(active,NULL);
	if(r!=LV_FS_RES_OK)msgbox_alert(
		"Rename '%s' to '%s' failed: %s",
		oldname,name,
		lv_fs_res_to_i18n_string(r)
	);
	free(buff);
	free(sel);
	return false;
}

static void btns_cb(lv_event_t*e){
	if(strcmp(guiact_get_last()->name,"file-manager")!=0)return;
	char*btn=(char*)e->user_data;
	tlog_info("click button %s",btn);
	if(strcmp(btn,"prev")==0){
		uint16_t act=lv_tabview_get_tab_act(tabview);
		if(act<=0)act=4;
		lv_tabview_set_act(tabview,act-1,LV_ANIM_ON);
		update_active();
	}else if(strcmp(btn,"refresh")==0){
		filetab_set_path(active,NULL);
	}else if(strcmp(btn,"edit")==0){
		if(filetab_get_checked_count(active)!=1)return;
		struct inputbox*in=inputbox_create(rename_cb,"Item rename");
		char**sel=filetab_get_checked(active);
		if(!sel||!sel[0]||sel[1]){
			if(sel)free(sel);
			msgbox_alert("File select invalid");
			return;
		}
		inputbox_set_content(in,"%s",sel[0]);
		free(sel);
	}else if(strcmp(btn,"home")==0){
		filetab_set_path(active,"/");
	}else if(strcmp(btn,"info")==0){
		msgbox_alert("This function does not implemented");
	}else if(strcmp(btn,"next")==0){
		uint16_t act=lv_tabview_get_tab_act(tabview)+1;
		if(act>=4)act=0;
		lv_tabview_set_act(tabview,act,LV_ANIM_ON);
		update_active();
	}else if(strcmp(btn,"paste")==0){
		msgbox_alert("This function does not implemented");
	}else if(strcmp(btn,"copy")==0){
		msgbox_alert("This function does not implemented");
	}else if(strcmp(btn,"delete")==0){
		msgbox_alert("This function does not implemented");
	}else if(strcmp(btn,"new")==0){
		static const char*types[]={
			LV_SYMBOL_FILE,
			LV_SYMBOL_DIRECTORY,
			""
		};
		if(fsext_is_multi&&filetab_is_top(active))return;
		msgbox_create_custom(create_cb,types,"Choose type to create");
	}else if(strcmp(btn,"cut")==0){
		msgbox_alert("This function does not implemented");
	}else if(strcmp(btn,"back")==0){
		if(!filetab_is_top(active)){
			lv_obj_t*tab=filetab_get_tab(active);
			if(tab&&lv_obj_get_scroll_y(tab)!=0)
				lv_obj_scroll_to_y(tab,0,LV_ANIM_OFF);
			filetab_go_back(active);
		}
	}
}

static int filemgr_draw(struct gui_activity*act){
	scr=act->page;
	lv_obj_set_flex_flow(scr,LV_FLEX_FLOW_COLUMN);

	// file view
	tabview=lv_tabview_create(scr,LV_DIR_TOP,gui_font_size*2);
	lv_obj_set_width(tabview,lv_pct(100));
	lv_obj_add_event_cb(tabview,tabview_cb,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_set_flex_grow(tabview,1);

	// current path
	path=lv_label_create(scr);
	lv_obj_set_style_text_align(path,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(path,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_size(path,lv_pct(100),gui_font_size*1.2);

	lv_obj_t*btns=lv_obj_create(act->page);
	lv_obj_set_style_radius(btns,0,0);
	lv_obj_set_scroll_dir(btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(btns,0,0);
	lv_obj_set_style_bg_opa(btns,LV_OPA_0,0);
	lv_obj_clear_flag(btns,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(btns,lv_pct(100));

	bool lb=gui_sw>gui_sh;
	size_t cs=ARRLEN(fm_btns);
	if(!lb)cs/=2;
	static lv_coord_t grid_col[16],grid_row[4];
	grid_row[0]=LV_GRID_FR(1);
	grid_row[1]=lb?LV_GRID_TEMPLATE_LAST:LV_GRID_FR(1);
	grid_row[2]=LV_GRID_TEMPLATE_LAST;
	for(size_t i=0;i<cs;i++)grid_col[i]=LV_GRID_FR(1);
	grid_col[cs]=LV_GRID_TEMPLATE_LAST;
	lv_obj_set_content_height(btns,gui_font_size*(lb?2:4));
	lv_obj_set_grid_dsc_array(btns,grid_col,grid_row);
	for(size_t i=0;i<ARRLEN(fm_btns);i++){
		fm_btns[i].btn=lv_btn_create(btns);
		lv_obj_set_enabled(fm_btns[i].btn,fm_btns[i].enabled);
		lv_obj_add_event_cb(fm_btns[i].btn,btns_cb,LV_EVENT_CLICKED,fm_btns[i].name);
		fm_btns[i].lbl=lv_label_create(fm_btns[i].btn);
		lv_label_set_text(fm_btns[i].lbl,fm_btns[i].symbol);
		lv_obj_center(fm_btns[i].lbl);
		lv_obj_set_grid_cell(
			fm_btns[i].btn,
			LV_GRID_ALIGN_STRETCH,i%cs,1,
			LV_GRID_ALIGN_STRETCH,i/cs,1
		);
	}
	tabview_create();
	return 0;
}

struct gui_register guireg_filemgr={
	.name="file-manager",
	.title="File Manager",
	.icon="filemgr.svg",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_cleanup,
	.get_focus=filemgr_get_focus,
	.lost_focus=filemgr_lost_focus,
	.draw=filemgr_draw,
	.back=true
};
#endif

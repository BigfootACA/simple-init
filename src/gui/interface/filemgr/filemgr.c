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
#include"gui/msgbox.h"
#include"gui/filetab.h"
#include"gui/fileopen.h"
#include"gui/inputbox.h"
#include"gui/activity.h"
#define TAG "filemgr"
static lv_obj_t*btn_prev,*btn_refresh,*btn_edit,*btn_home,*btn_info,*btn_next;
static lv_obj_t*btn_paste,*btn_copy,*btn_delete,*btn_new,*btn_cut,*btn_back;
static lv_obj_t*tabview,*scr,*path;
static struct filetab*tabs[4],*active=NULL;

static int filemgr_get_focus(struct gui_activity*d __attribute__((unused))){
	if(active)filetab_add_group(active,gui_grp);
	lv_group_add_obj(gui_grp,btn_prev);
	lv_group_add_obj(gui_grp,btn_refresh);
	lv_group_add_obj(gui_grp,btn_edit);
	lv_group_add_obj(gui_grp,btn_home);
	lv_group_add_obj(gui_grp,btn_info);
	lv_group_add_obj(gui_grp,btn_next);
	lv_group_add_obj(gui_grp,btn_paste);
	lv_group_add_obj(gui_grp,btn_copy);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_new);
	lv_group_add_obj(gui_grp,btn_cut);
	lv_group_add_obj(gui_grp,btn_back);
	return 0;
}

static int filemgr_lost_focus(struct gui_activity*d __attribute__((unused))){
	if(active)filetab_remove_group(active);
	lv_group_remove_obj(btn_prev);
	lv_group_remove_obj(btn_refresh);
	lv_group_remove_obj(btn_edit);
	lv_group_remove_obj(btn_home);
	lv_group_remove_obj(btn_info);
	lv_group_remove_obj(btn_next);
	lv_group_remove_obj(btn_paste);
	lv_group_remove_obj(btn_copy);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_new);
	lv_group_remove_obj(btn_cut);
	lv_group_remove_obj(btn_back);
	return 0;
}

static void update_active(){
	char*np;
	struct filetab*old=active;
	filetab_remove_group(active);
	active=NULL;
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(!filetab_is_active(tabs[i]))continue;
		active=tabs[i];
		filetab_add_group(active,gui_grp);
		if((np=filetab_get_path(active))){
			lv_label_set_text(path,np);
			free(np);
		}
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

static void on_change_dir(struct filetab*fv,url*old __attribute__((unused)),url*new){
	char*str;
	size_t tab=0;
	if(!filetab_is_active(fv))return;
	for(tab=0;tab<ARRLEN(tabs);tab++)if(tabs[tab]==fv)break;
	if(new){
		if(!(str=url_generate_alloc(new)))return;
		lv_label_set_text(path,str);
		confd_set_string_array("gui.filemgr.tab",(int)tab,"dir",str);
		free(str);
	}else{
		lv_label_set_text(path,"/");
		confd_delete_array("gui.filemgr.tab",(int)tab,"dir");
	}
}

static bool on_item_click(struct filetab*fv,char*item,fs_type type){
	fsh*f=NULL,*nf=NULL;
	if(!filetab_is_active(fv))return true;
	if(!fs_has_type(type,FS_TYPE_FILE))return true;
	if(fs_has_type(type,FS_TYPE_FILE_FOLDER))return true;
	if(!(nf=filetab_get_fsh(fv)))return true;
	if(fs_open(nf,&f,item,FILE_FLAG_READ)!=0)return true;
	fileopen_open_fsh(f);
	fs_close(&f);
	return true;
}

static void on_item_select(
	struct filetab*fv,
	char*name __attribute__((unused)),
	fs_type type __attribute__((unused)),
	bool checked __attribute__((unused)),
	uint16_t cnt
){
	if(fv!=active)return;
	if(filetab_is_top(fv))return;
	if(cnt==1){
		lv_obj_set_enabled(btn_edit,true);
		lv_obj_set_enabled(btn_info,true);
	}else{
		lv_obj_set_enabled(btn_edit,false);
		lv_obj_set_enabled(btn_info,false);
	}
	if(cnt>0){
		lv_obj_set_enabled(btn_cut,true);
		lv_obj_set_enabled(btn_copy,true);
		lv_obj_set_enabled(btn_delete,true);
	}else{
		lv_obj_set_enabled(btn_cut,false);
		lv_obj_set_enabled(btn_copy,false);
		lv_obj_set_enabled(btn_delete,false);
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
	int r=0;
	fsh*f=NULL,*pf=NULL;
	fs_file_flag flag=FILE_FLAG_CREATE;
	if(!ok)return false;
	if(!(pf=filetab_get_fsh(active)))return false;
	switch(*(uint16_t*)user_data){
		case 0:flag|=0644;break;//file
		case 1:flag|=0755|FILE_FLAG_FOLDER;break;//folder
		default:return false;
	}
	r=fs_open(pf,&f,name,flag);
	if(f)fs_close(&f);
	filetab_set_path(active,NULL);
	if(r!=LV_FS_RES_OK)msgbox_alert(
		"Create '%s' failed: %s",
		name,strerror(r)
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
	fsh*f=filetab_get_fsh(active);
	if(!f||!ok||filetab_get_checked_count(active)!=1)return false;
	char**sel=filetab_get_checked(active);
	if(!sel||!sel[0]||sel[1]){
		if(sel)free(sel);
		msgbox_alert("File select invalid");
		return true;
	}
	int r=fs_rename_at(f,sel[0],name);
	filetab_set_path(active,NULL);
	if(r!=0)msgbox_alert(
		"Rename '%s' to '%s' failed: %s",
		sel[0],name,strerror(r)
	);
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
		if(filetab_is_top(active))return;
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
	lv_obj_set_style_bg_opa(tabview,LV_OPA_TRANSP,0);
	lv_obj_add_event_cb(tabview,tabview_cb,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_set_flex_grow(tabview,1);

	// current path
	path=lv_label_create(scr);
	lv_obj_set_style_text_align(path,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_long_mode(
		path,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_size(path,lv_pct(100),LV_SIZE_CONTENT);

	size_t cs=gui_sw>gui_sh?12:6;
	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,en,x)&(struct button_dsc){\
			&btn_##tgt,en,title,btns_cb,#tgt,x%cs,1,x/cs,1,NULL\
		}
		BTN(prev,    LV_SYMBOL_LEFT,      true,  0),
		BTN(refresh, LV_SYMBOL_REFRESH,   true,  1),
		BTN(edit,    LV_SYMBOL_EDIT,      false, 2),
		BTN(home,    LV_SYMBOL_HOME,      true,  3),
		BTN(info,    LV_SYMBOL_LIST,      false, 4),
		BTN(next,    LV_SYMBOL_RIGHT,     true,  5),
		BTN(paste,   LV_SYMBOL_PASTE,     false, 6),
		BTN(copy,    LV_SYMBOL_COPY,      false, 7),
		BTN(delete,  LV_SYMBOL_TRASH,     false, 8),
		BTN(new,     LV_SYMBOL_PLUS,      true,  9),
		BTN(cut,     LV_SYMBOL_CUT,       false, 10),
		BTN(back,    LV_SYMBOL_BACKSPACE, true,  11),
		#undef BTN
		NULL
	);
	tabview_create();
	return 0;
}

struct gui_register guireg_filemgr={
	.name="file-manager",
	.title="File Manager",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_cleanup,
	.get_focus=filemgr_get_focus,
	.lost_focus=filemgr_lost_focus,
	.draw=filemgr_draw,
	.back=true
};
#endif

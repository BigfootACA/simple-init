#ifdef ENABLE_GUI
#include<stdlib.h>
#include"str.h"
#include"gui.h"
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

static lv_obj_t*tabview,*scr,*path;
static lv_obj_t*btn_prev,*btn_refresh,*btn_edit,*btn_home,*btn_info,*btn_next;
static lv_obj_t*btn_paste,*btn_copy,*btn_delete,*btn_new,*btn_cut,*btn_back;
static struct filetab*tabs[4],*active=NULL;

static int filemgr_get_focus(struct gui_activity*d __attribute__((unused))){
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
	if(active)filetab_add_group(active,gui_grp);
	return 0;
}

static int filemgr_lost_focus(struct gui_activity*d __attribute__((unused))){
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
		if(old!=active)tlog_debug(
			"switch to tab %d",
			filetab_get_id(active)
		);
		break;
	}
}

int do_cleanup(struct gui_activity*d __attribute__((unused))){
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(tabs[i])filetab_free(tabs[i]);
		tabs[i]=NULL;
	}
	active=NULL;
	return 0;
}

static void tabview_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	update_active();
}

static void on_change_dir(struct filetab*fv,char*old __attribute__((unused)),char*new){
	if(filetab_is_active(fv))lv_label_set_text(path,new);
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
	struct filetab*fv __attribute__((unused)),
	char*name __attribute__((unused)),
	enum item_type type __attribute__((unused)),
	bool checked __attribute__((unused)),
	uint16_t cnt
){
	if(fsext_is_multi&&filetab_is_top(active))return;
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
		filetab_set_path(tabs[i],"/");
		update_active();
	}
}

static int do_back(struct gui_activity*d __attribute__((unused))){
	lv_obj_t*tab=filetab_get_tab(active);
	if(!tab)return 0;
	if(!lv_page_is_top(tab)){
		lv_page_go_top(tab);
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
	char oldname[256]={0};
	char oldpath[PATH_MAX]={0};
	char newpath[PATH_MAX]={0};
	char*fp=filetab_get_lvgl_path(active);
	char**sel=filetab_get_checked(active);
	if(!sel||!sel[0]||sel[1]){
		if(sel)free(sel);
		msgbox_alert("File select invalid");
		return true;
	}
	strncpy(oldname,sel[0],255);
	free(sel);
	snprintf(oldpath,PATH_MAX-1,"%s/%s",fp,oldname);
	snprintf(newpath,PATH_MAX-1,"%s/%s",fp,name);
	lv_res_t r=lv_fs_rename(oldpath,newpath);
	filetab_set_path(active,NULL);
	if(r!=LV_FS_RES_OK)msgbox_alert(
		"Rename '%s' to '%s' failed: %s",
		oldname,name,
		lv_fs_res_to_i18n_string(r)
	);
	return false;
}

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(strcmp(guiact_get_last()->name,"file-manager")!=0)return;
	tlog_info("click button %s",(char*)lv_obj_get_user_data(obj));
	if(obj==btn_prev){
		uint16_t act=lv_tabview_get_tab_act(tabview);
		if(act<=0)act=lv_tabview_get_tab_count(tabview);
		lv_tabview_set_tab_act(tabview,act-1,LV_ANIM_ON);
		update_active();
	}else if(obj==btn_refresh){
		filetab_set_path(active,NULL);
	}else if(obj==btn_edit){
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
	}else if(obj==btn_home){
		filetab_set_path(active,"/");
	}else if(obj==btn_info){
		msgbox_alert("This function does not implemented");
	}else if(obj==btn_next){
		uint16_t act=lv_tabview_get_tab_act(tabview);
		if(act>=lv_tabview_get_tab_count(tabview)-1)act=-1;
		lv_tabview_set_tab_act(tabview,act+1,LV_ANIM_ON);
		update_active();
	}else if(obj==btn_paste){
		msgbox_alert("This function does not implemented");
	}else if(obj==btn_copy){
		msgbox_alert("This function does not implemented");
	}else if(obj==btn_delete){
		msgbox_alert("This function does not implemented");
	}else if(obj==btn_new){
		static const char*types[]={
			LV_SYMBOL_FILE,
			LV_SYMBOL_DIRECTORY,
			""
		};
		if(fsext_is_multi&&filetab_is_top(active))return;
		msgbox_create_custom(create_cb,types,"Choose type to create");
	}else if(obj==btn_cut){
		msgbox_alert("This function does not implemented");
	}else if(obj==btn_back){
		if(!filetab_is_top(active)){
			lv_obj_t*tab=filetab_get_tab(active);
			if(tab&&!lv_page_is_top(tab))lv_page_go_top(tab);
			filetab_go_back(active);
		}
	}
}

static int filemgr_draw(struct gui_activity*act){
	lv_coord_t btx=gui_font_size,btm=btx/2,btw=(gui_sw-btm)/6-btm,bth=btx*2;
	scr=act->page;

	static lv_style_t s;
	lv_style_init(&s);
	lv_style_set_pad_all(&s,LV_STATE_DEFAULT,gui_font_size/2);

	// file view
	tabview=lv_tabview_create(scr,NULL);
	lv_obj_set_event_cb(tabview,tabview_cb);
	lv_obj_add_style(tabview,LV_TABVIEW_PART_TAB_BTN,&s);

	static lv_style_t btn;
	lv_style_init(&btn);
	lv_style_set_radius(&btn,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&btn,LV_STATE_DEFAULT,0);

	// current path
	path=lv_label_create(scr,NULL);
	lv_label_set_align(path,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(path,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_size(path,gui_sw,gui_dpi/7);
	lv_obj_align(path,NULL,LV_ALIGN_IN_BOTTOM_LEFT,0,-bth*2-btm*3);
	lv_obj_set_size(tabview,gui_sw,lv_obj_get_y(path));

	lv_obj_t*line=lv_obj_create(act->page,NULL);
	lv_obj_set_size(line,gui_sw,gui_dpi/100);
	lv_obj_align(line,path,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_theme_apply(line,LV_THEME_SCR);
	lv_obj_set_style_local_bg_color(
		line,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,
		lv_color_darken(lv_obj_get_style_bg_color(line,LV_OBJ_PART_MAIN),LV_OPA_20)
	);

	btn_prev=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_prev,btw,bth);
	lv_obj_set_event_cb(btn_prev,btns_cb);
	lv_obj_set_user_data(btn_prev,"prev");
	lv_obj_align(btn_prev,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm*2-bth);
	lv_obj_set_style_local_radius(btn_prev,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_prev,NULL),LV_SYMBOL_LEFT);

	btn_refresh=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_refresh,btw,bth);
	lv_obj_set_event_cb(btn_refresh,btns_cb);
	lv_obj_set_user_data(btn_refresh,"refresh");
	lv_obj_align(btn_refresh,btn_prev,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_refresh,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),LV_SYMBOL_REFRESH);

	btn_edit=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_edit,false);
	lv_obj_set_size(btn_edit,btw,bth);
	lv_obj_set_event_cb(btn_edit,btns_cb);
	lv_obj_set_user_data(btn_edit,"edit");
	lv_obj_align(btn_edit,btn_refresh,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_edit,NULL),LV_SYMBOL_EDIT);

	btn_home=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_home,btw,bth);
	lv_obj_set_event_cb(btn_home,btns_cb);
	lv_obj_set_user_data(btn_home,"home");
	lv_obj_align(btn_home,btn_edit,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_home,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_home,NULL),LV_SYMBOL_HOME);

	btn_info=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_info,false);
	lv_obj_set_size(btn_info,btw,bth);
	lv_obj_set_event_cb(btn_info,btns_cb);
	lv_obj_set_user_data(btn_info,"info");
	lv_obj_align(btn_info,btn_home,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_info,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_info,NULL),LV_SYMBOL_LIST);

	btn_next=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_next,btw,bth);
	lv_obj_set_event_cb(btn_next,btns_cb);
	lv_obj_set_user_data(btn_next,"next");
	lv_obj_align(btn_next,btn_info,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_next,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_next,NULL),LV_SYMBOL_RIGHT);

	btn_paste=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_paste,false);
	lv_obj_set_size(btn_paste,btw,bth);
	lv_obj_set_event_cb(btn_paste,btns_cb);
	lv_obj_set_user_data(btn_paste,"paste");
	lv_obj_align(btn_paste,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(btn_paste,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_paste,NULL),LV_SYMBOL_PASTE);

	btn_copy=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_copy,false);
	lv_obj_set_size(btn_copy,btw,bth);
	lv_obj_set_event_cb(btn_copy,btns_cb);
	lv_obj_set_user_data(btn_copy,"copy");
	lv_obj_align(btn_copy,btn_paste,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_copy,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_copy,NULL),LV_SYMBOL_COPY);

	btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_delete,false);
	lv_obj_set_size(btn_delete,btw,bth);
	lv_obj_set_event_cb(btn_delete,btns_cb);
	lv_obj_set_user_data(btn_delete,"delete");
	lv_obj_align(btn_delete,btn_copy,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_delete,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_delete,NULL),LV_SYMBOL_TRASH);

	btn_new=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_new,btw,bth);
	lv_obj_set_event_cb(btn_new,btns_cb);
	lv_obj_set_user_data(btn_new,"new");
	lv_obj_align(btn_new,btn_delete,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_new,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_new,NULL),LV_SYMBOL_PLUS);

	btn_cut=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_cut,false);
	lv_obj_set_size(btn_cut,btw,bth);
	lv_obj_set_event_cb(btn_cut,btns_cb);
	lv_obj_set_user_data(btn_cut,"cut");
	lv_obj_align(btn_cut,btn_new,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_cut,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_cut,NULL),LV_SYMBOL_CUT);

	btn_back=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_back,btw,bth);
	lv_obj_set_event_cb(btn_back,btns_cb);
	lv_obj_set_user_data(btn_back,"back");
	lv_obj_align(btn_back,btn_cut,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_back,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_back,NULL),LV_SYMBOL_BACKSPACE);

	tabview_create();
	return 0;
}

struct gui_register guireg_filemgr={
	.name="file-manager",
	.title="File Manager",
	.icon="filemgr.png",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_cleanup,
	.get_focus=filemgr_get_focus,
	.lost_focus=filemgr_lost_focus,
	.draw=filemgr_draw,
	.back=true
};
#endif

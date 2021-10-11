#ifdef ENABLE_GUI
#include<unistd.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"init_internal.h"
#define TAG "gadget"

static lv_obj_t*view,*info,*last;
static lv_obj_t*btn_add,*btn_edit;
static lv_obj_t*btn_delete,*btn_restart;
static lv_obj_t*btn_reload,*btn_base;
static char*base="gadget.func";

struct func_item{
	bool enable;
	lv_obj_t*btn,*chk;
	char id[64];
}items[64],*selected;

static void btns_toggle(bool state){
	lv_obj_set_enabled(btn_add,state);
	lv_obj_set_enabled(btn_edit,state);
	lv_obj_set_enabled(btn_delete,state);
}

static void clean_items(){
	if(info)lv_obj_del(info);
	info=NULL,selected=NULL,last=NULL;
	for(int i=0;i<64;i++){
		if(!items[i].enable)continue;
		lv_obj_del(items[i].btn);
		memset(&items[i],0,sizeof(struct func_item));
	}
}

static void set_info(char*text){
	btns_toggle(false);
	clean_items();
	info=lv_label_create(view,NULL);
	lv_label_set_long_mode(info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(info,lv_page_get_scrl_width(view),gui_sh/16);
	lv_label_set_align(info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(info,text);
}


static void item_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected){
		if(obj==selected->chk)return;
		else{
			lv_checkbox_set_checked(selected->chk,false);
			lv_obj_set_checked(selected->btn,false);
		}
	}
	selected=NULL;
	for(int i=0;i<64&&!selected;i++)
		if(items[i].enable&&items[i].chk==obj)
			selected=&items[i];
	if(!selected)return;
	lv_obj_set_checked(selected->btn,true);
	btns_toggle(true);
	tlog_debug("selected function %s",selected->id);
}

static void view_add_item(struct func_item*k){
	lv_coord_t bw=lv_page_get_scrl_width(view),m=gui_dpi/20;

	// option select button
	k->btn=lv_btn_create(view,NULL);
	lv_obj_set_size(k->btn,bw,gui_dpi/2);
	lv_obj_align(
		k->btn,last,
		last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
		0,last?gui_dpi/8:0
	);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);
	last=k->btn;

	// line for button text
	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	// function name and checkbox
	k->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(k->chk);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(e->label,bw-gui_dpi/5*2);
	lv_checkbox_set_text(k->chk,confd_get_string_dict(base,k->id,"name",_("(unknown)")));
	lv_obj_set_event_cb(k->chk,item_click);
	lv_style_set_focus_checkbox(k->chk);
	lv_obj_align(k->chk,k->btn,LV_ALIGN_IN_TOP_LEFT,m,m);
	lv_group_add_obj(gui_grp,k->chk);

	// function type
	lv_obj_t*type=lv_label_create(line,NULL);
	lv_label_set_long_mode(type,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(type,bw/2-m*2);
	lv_label_set_text(type,confd_get_string_dict(base,k->id,"func",_("(unknown)")));
	lv_obj_align(type,k->btn,LV_ALIGN_IN_BOTTOM_LEFT,m,-m);

	// function mode
	lv_obj_t*mode=lv_label_create(line,NULL);
	lv_label_set_long_mode(mode,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(mode,bw/2-m*2);
	lv_label_set_align(mode,LV_LABEL_ALIGN_RIGHT);
	lv_label_set_text(mode,confd_get_string_dict(base,k->id,"mode","(unknown)"));
	lv_obj_align(mode,k->btn,LV_ALIGN_IN_BOTTOM_RIGHT,-m,-m);
}

static void view_reload(){
	btns_toggle(false);
	clean_items();
	int i=0;
	char**list=confd_ls(base),*item;
	if(list)for(;(item=list[i]);i++){
		struct func_item*k=&items[i];
		k->enable=true;
		strcpy(k->id,item);
		view_add_item(k);
		if(i>=64){
			tlog_warn("functions too many, only show 64 functions");
			break;
		}
	}
	if(i==0)set_info(_("(none)"));
	tlog_info("found %d functions",i);
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	view_reload();
	lv_group_add_obj(gui_grp,btn_add);
	lv_group_add_obj(gui_grp,btn_edit);
	lv_group_add_obj(gui_grp,btn_delete);
	lv_group_add_obj(gui_grp,btn_restart);
	lv_group_add_obj(gui_grp,btn_reload);
	lv_group_add_obj(gui_grp,btn_base);
}

static int gadget_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int gadget_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<64;i++){
		if(!items[i].enable)continue;
		lv_group_remove_obj(items[i].chk);
	}
	lv_group_remove_obj(btn_add);
	lv_group_remove_obj(btn_edit);
	lv_group_remove_obj(btn_delete);
	lv_group_remove_obj(btn_restart);
	lv_group_remove_obj(btn_reload);
	lv_group_remove_obj(btn_base);
	return 0;
}

static bool delete_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0&&selected){
		char key[PATH_MAX]={0};
		snprintf(key,PATH_MAX-1,"%s.%s",base,selected->id);
		tlog_info("delete %s",key);
		confd_delete(key);
	}
	return false;
}

static bool restart_cb(uint16_t id,const char*text __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0){
		tlog_info("try restart gadget service");
		struct init_msg msg,response;
		init_initialize_msg(&msg,ACTION_SVC_RESTART);
		strcpy(msg.data.data,"usb-gadget");
		errno=0;
		init_send(&msg,&response);
		if(errno!=0||response.data.status.ret!=0){
			if(errno==0)errno=response.data.status.ret;
			telog_warn("restart service failed");
			msgbox_alert("Restart gadget service failed: %m");
		}
	}
	return false;
}

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(strcmp(guiact_get_last()->name,"usb-gadget")!=0)return;
	tlog_info("click button %s",(char*)lv_obj_get_user_data(obj));
	if(obj==btn_restart){
		msgbox_create_yesno(restart_cb,"Are you sure to restart gadget service?");
	}else if(obj==btn_reload){
		view_reload();
	}else if(obj==btn_base){
		guiact_start_activity_by_name("usb-gadget-base-info",NULL);
	}else if(!selected)return;
	else if(obj==btn_add){

	}else if(obj==btn_edit){

	}else if(obj==btn_delete){
		msgbox_create_yesno(delete_cb,"Are you sure to delete function %s?",selected->id);
	}
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	clean_items();
	return 0;
}

static int gadget_draw(struct gui_activity*act){
	lv_coord_t btx=gui_font_size,bts=gui_sw/6-btx,btm=btx/2;

	// app title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_sh/32);
	lv_obj_set_size(title,gui_sw,gui_sh/16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("USB Gadget"));

	// function view
	view=lv_page_create(act->page,NULL);
	lv_obj_set_width(view,gui_sw-gui_font_size);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(view,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_align(view,title,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_set_height(view,gui_sh-gui_sw/6-lv_obj_get_y(view));

	btn_add=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_add,bts,bts);
	lv_obj_set_event_cb(btn_add,btns_cb);
	lv_obj_set_user_data(btn_add,"add");
	lv_obj_align(btn_add,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(btn_add,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_add,NULL),LV_SYMBOL_PLUS);

	btn_edit=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_edit,bts,bts);
	lv_obj_set_event_cb(btn_edit,btns_cb);
	lv_obj_set_user_data(btn_edit,"edit");
	lv_obj_align(btn_edit,btn_add,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_edit,NULL),LV_SYMBOL_EDIT);

	btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_delete,bts,bts);
	lv_obj_set_event_cb(btn_delete,btns_cb);
	lv_obj_set_user_data(btn_delete,"delete");
	lv_obj_align(btn_delete,btn_edit,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_delete,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_delete,NULL),LV_SYMBOL_TRASH);

	btn_restart=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_restart,bts,bts);
	lv_obj_set_event_cb(btn_restart,btns_cb);
	lv_obj_set_user_data(btn_restart,"restart");
	lv_obj_align(btn_restart,btn_delete,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_restart,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_restart,NULL),LV_SYMBOL_OK);

	btn_reload=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_reload,bts,bts);
	lv_obj_set_event_cb(btn_reload,btns_cb);
	lv_obj_set_user_data(btn_reload,"reload");
	lv_obj_align(btn_reload,btn_restart,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_reload,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_reload,NULL),LV_SYMBOL_REFRESH);

	btn_base=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_base,bts,bts);
	lv_obj_set_event_cb(btn_base,btns_cb);
	lv_obj_set_user_data(btn_base,"settings");
	lv_obj_align(btn_base,btn_reload,LV_ALIGN_OUT_RIGHT_MID,btx,0);
	lv_obj_set_style_local_radius(btn_base,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_base,NULL),LV_SYMBOL_SETTINGS);

	return 0;
}

struct gui_register guireg_gadget={
	.name="usb-gadget",
	.title="USB Gadget",
	.icon="usb.png",
	.show_app=true,
	.quiet_exit=do_clean,
	.draw=gadget_draw,
	.get_focus=gadget_get_focus,
	.lost_focus=gadget_lost_focus,
	.back=true
};
#endif

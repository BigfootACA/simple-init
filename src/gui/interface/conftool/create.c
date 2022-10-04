/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<string.h>
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "conftool"

struct create_box{
	lv_obj_t*mask,*box,*label;
	lv_obj_t*key,*lbl_key;
	lv_obj_t*dropdown;
	lv_obj_t*val,*clr_val;
	lv_obj_t*ok,*cancel;
};

static int createbox_get_focus(struct gui_activity*d){
	struct create_box*box=(struct create_box*)d->data;
	if(!box)return 0;
	lv_group_add_obj(gui_grp,box->key);
	lv_group_add_obj(gui_grp,box->dropdown);
	lv_group_add_obj(gui_grp,box->clr_val);
	lv_group_add_obj(gui_grp,box->val);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int createbox_lost_focus(struct gui_activity*d){
	struct create_box*box=(struct create_box*)d->data;
	if(!box)return 0;
	lv_group_remove_obj(box->key);
	lv_group_remove_obj(box->dropdown);
	lv_group_remove_obj(box->clr_val);
	lv_group_remove_obj(box->val);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

static void dropdown_cb(lv_event_t*e){
	struct create_box*box=e->user_data;
	if(!box||e->target!=box->dropdown)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	switch(lv_dropdown_get_selected(box->dropdown)){
		case 0: // String
			lv_textarea_set_accepted_chars(box->val,NULL);
			lv_textarea_set_max_length(box->val,0);
			lv_textarea_set_text(box->val,"");
			lv_textarea_set_one_line(box->val,false);
		break;
		case 1: // Integer
			lv_textarea_set_accepted_chars(box->val,NUMBER"-");
			lv_textarea_set_max_length(box->val,20);
			lv_textarea_set_text(box->val,"0");
			lv_textarea_set_one_line(box->val,true);
			break;
		case 2: // Boolean
			lv_textarea_set_accepted_chars(box->val,"01");
			lv_textarea_set_max_length(box->val,1);
			lv_textarea_set_text(box->val,"0");
			lv_textarea_set_one_line(box->val,true);
		break;
	}
}

static void input_cb(lv_event_t*e){
	struct create_box*box=e->user_data;
	if(!box||guiact_get_last()->data!=box)return;
	const char*buf=lv_textarea_get_text(box->key);
	lv_obj_set_enabled(box->ok,buf[0]!=0);
}

static void do_save(const char*key,const char*val,struct create_box*box){
	size_t s=0;
	errno=0;
	confd_delete(key);
	switch(lv_dropdown_get_selected(box->dropdown)){
		case 0:{// String
			s=strlen(val);
			confd_set_string(key,(char*)val);
		}break;
		case 1:{// Integer
			s=sizeof(int64_t);
			char*end;
			int64_t num;
			errno=0,num=strtol(val,&end,10);
			if(strcmp(val,"0")||(errno==0&&end!=val&&*end==0))
				confd_set_integer(key,num);
			else if(errno==0)errno=EINVAL;
		}break;
		case 2:{ // Boolean
			s=sizeof(bool);
			if(strcmp(val,"0")==0)
				confd_set_boolean(key,false);
			else if(strcmp(val,"1")==0)
				confd_set_boolean(key,true);
			else errno=EINVAL;
		}break;
		default:return;
	}
	if(errno==0){
		tlog_debug("save %zu bytes to config item '%s'",s,key);
		guiact_do_back();
	}else{
		tlog_debug("save config item '%s' failed: %m",key);
		msgbox_alert(_("save config item '%s' failed: %m"),key);
	}
}

static bool overwrite_cb(uint16_t id,const char*text __attribute__((unused)),void*data){
	struct create_box*box=(struct create_box*)data;
	if(id==0)do_save(
		lv_textarea_get_text(box->key),
		lv_textarea_get_text(box->val),
		box
	);
	return false;
}

static void btn_cb(lv_event_t*e){
	struct create_box*box=e->user_data;
	if(!box||guiact_get_last()->data!=box)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	if(e->target==box->ok){
		const char*key=lv_textarea_get_text(box->key);
		const char*val=lv_textarea_get_text(box->val);
		if(confd_get_type(key)!=TYPE_KEY)do_save(key,val,box);
		else msgbox_set_user_data(msgbox_create_yesno(
			overwrite_cb,
			"'%s' is an exists key, do you want to overwrite it?",
			key
		),box);
	}else if(e->target==box->cancel)guiact_do_back();
}

static int createbox_init(struct gui_activity*act){
	struct create_box*box=malloc(sizeof(struct create_box));
	if(!box)ERET(ENOMEM);
	memset(box,0,sizeof(struct create_box));
	act->data=box;
	return 0;
}

static int createbox_clean(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int createbox_draw(struct gui_activity*act){
	struct create_box*box=act->data;
	if(!act->args||!box)return -1;
	char*p=act->args+1;
	if(strcmp(p,"/")==0)p="";

	box->box=lv_draw_dialog_box(
		act->page,&box->label,
		((char*)act->args)[0]?
		_("Edit config item"):
		_("Create config item")
	);

	box->lbl_key=lv_label_create(box->box);
	lv_label_set_text(box->lbl_key,_("Config item path:"));

	box->key=lv_textarea_create(box->box);
	lv_obj_set_width(box->key,lv_pct(100));
	lv_textarea_set_text(box->key,p);
	lv_textarea_set_one_line(box->key,true);
	lv_textarea_set_accepted_chars(box->key,VALID"-.");
	lv_obj_add_event_cb(box->key,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_add_event_cb(box->key,input_cb,LV_EVENT_VALUE_CHANGED,box);

	lv_draw_dropdown(box->box,"Type:",&box->dropdown);
	lv_obj_add_event_cb(box->dropdown,dropdown_cb,LV_EVENT_VALUE_CHANGED,box);
	lv_dropdown_clear_options(box->dropdown);
	lv_dropdown_add_option(box->dropdown,_("String"),0);
	lv_dropdown_add_option(box->dropdown,_("Integer"),1);
	lv_dropdown_add_option(box->dropdown,_("Boolean"),2);

	lv_draw_input(box->box,"Value:",NULL,&box->clr_val,&box->val,NULL);
	lv_draw_btns_ok_cancel(box->box,&box->ok,&box->cancel,btn_cb,box);
	lv_textarea_set_one_line(box->val,false);

	if(((char*)act->args)[0]){
		char val[128]={0},*s;
		switch(confd_get_type(p)){
			case TYPE_KEY:break;
			case TYPE_STRING:
				lv_dropdown_set_selected(box->dropdown,0);
				lv_event_send(box->dropdown,LV_EVENT_VALUE_CHANGED,NULL);
				s=confd_get_string(p,"");
				lv_textarea_set_text(box->val,s?s:"");
				if(s)free(s);
			break;
			case TYPE_INTEGER:
				lv_dropdown_set_selected(box->dropdown,1);
				lv_event_send(box->dropdown,LV_EVENT_VALUE_CHANGED,NULL);
				snprintf(val,127,"%lld",(long long int)confd_get_integer(p,0));
				lv_textarea_set_text(box->val,val);
			break;
			case TYPE_BOOLEAN:
				lv_dropdown_set_selected(box->dropdown,2);
				lv_event_send(box->dropdown,LV_EVENT_VALUE_CHANGED,NULL);
				snprintf(val,127,"%d",confd_get_boolean(p,false));
				lv_textarea_set_text(box->val,val);
			break;
		}
	}

	return 0;
}

struct gui_register guireg_conftool_create={
	.name="config-manager-create",
	.title="Config Manager Create",
	.show_app=false,
	.init=createbox_init,
	.quiet_exit=createbox_clean,
	.get_focus=createbox_get_focus,
	.lost_focus=createbox_lost_focus,
	.draw=createbox_draw,
	.back=true,
	.mask=true
};
#endif

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
	lv_obj_t*chk_string;
	lv_obj_t*chk_integer;
	lv_obj_t*chk_boolean;
	lv_obj_t*val,*lbl_val;
	lv_obj_t*ok,*cancel;
};

static int createbox_get_focus(struct gui_activity*d){
	struct create_box*box=(struct create_box*)d->data;
	if(!box)return 0;
	lv_group_add_obj(gui_grp,box->key);
	lv_group_add_obj(gui_grp,box->chk_string);
	lv_group_add_obj(gui_grp,box->chk_integer);
	lv_group_add_obj(gui_grp,box->chk_boolean);
	lv_group_add_obj(gui_grp,box->val);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int createbox_lost_focus(struct gui_activity*d){
	struct create_box*box=(struct create_box*)d->data;
	if(!box)return 0;
	lv_group_remove_obj(box->key);
	lv_group_remove_obj(box->chk_string);
	lv_group_remove_obj(box->chk_integer);
	lv_group_remove_obj(box->chk_boolean);
	lv_group_remove_obj(box->val);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

static void radio_cb(lv_event_t*e){
	struct create_box*box=e->user_data;
	if(!box)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	if(e->target==box->chk_string){
		lv_obj_set_checked(box->chk_integer,false);
		lv_obj_set_checked(box->chk_boolean,false);
		lv_textarea_set_accepted_chars(box->val,NULL);
		lv_textarea_set_max_length(box->val,0);
		lv_textarea_set_text(box->val,"");
		lv_textarea_set_one_line(box->val,false);
	}else if(e->target==box->chk_integer){
		lv_obj_set_checked(box->chk_string,false);
		lv_obj_set_checked(box->chk_boolean,false);
		lv_textarea_set_accepted_chars(box->val,NUMBER"-");
		lv_textarea_set_max_length(box->val,20);
		lv_textarea_set_text(box->val,"0");
		lv_textarea_set_one_line(box->val,true);
	}else if(e->target==box->chk_boolean){
		lv_obj_set_checked(box->chk_string,false);
		lv_obj_set_checked(box->chk_integer,false);
		lv_textarea_set_accepted_chars(box->val,"01");
		lv_textarea_set_max_length(box->val,1);
		lv_textarea_set_text(box->val,"0");
		lv_textarea_set_one_line(box->val,true);
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
	if(lv_obj_is_checked(box->chk_string)){
		s=strlen(val);
		confd_set_string(key,(char*)val);
	}else if(lv_obj_is_checked(box->chk_integer)){
		s=sizeof(int64_t);
		char*end;
		int64_t num;
		errno=0,num=strtol(val,&end,10);
		if(errno==0&&end!=val&&*end==0)confd_set_integer(key,num);
		else if(errno==0)errno=EINVAL;
	}else if(lv_obj_is_checked(box->chk_boolean)){
		s=sizeof(bool);
		if(strcmp(val,"0")==0)confd_set_boolean(key,false);
		else if(strcmp(val,"1")==0)confd_set_boolean(key,true);
		else errno=EINVAL;
	}else return;
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
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_DPI_DEF,
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	struct create_box*box=act->data;
	if(!act->args||!box)return -1;
	char*p=act->args+1;
	if(strcmp(p,"/")==0)p="";

	box->box=lv_obj_create(act->page);
	lv_obj_set_style_max_width(box->box,lv_pct(85),0);
	lv_obj_set_style_max_height(box->box,lv_pct(85),0);
	lv_obj_set_style_min_width(box->box,gui_dpi*2,0);
	lv_obj_set_height(box->box,LV_SIZE_CONTENT);
	lv_obj_set_grid_dsc_array(box->box,grid_col,grid_row);
	lv_obj_set_style_pad_row(box->box,gui_font_size/2,0);
	lv_obj_center(box->box);

	box->label=lv_label_create(box->box);
	lv_label_set_text(
		box->label,((char*)act->args)[0]?
		_("Edit config item"):
		_("Create config item")
	);
	lv_obj_set_grid_cell(
		box->label,
		LV_GRID_ALIGN_CENTER,0,2,
		LV_GRID_ALIGN_CENTER,0,1
	);

	box->lbl_key=lv_label_create(box->box);
	lv_obj_set_small_text_font(box->lbl_key,LV_PART_MAIN);
	lv_label_set_text(box->lbl_key,_("Config item path:"));
	lv_obj_set_grid_cell(
		box->lbl_key,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,1,1
	);

	box->key=lv_textarea_create(box->box);
	lv_textarea_set_text(box->key,p);
	lv_textarea_set_one_line(box->key,true);
	lv_textarea_set_accepted_chars(box->key,VALID"-.");
	lv_obj_add_event_cb(box->key,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_add_event_cb(box->key,input_cb,LV_EVENT_VALUE_CHANGED,box);
	lv_obj_set_grid_cell(
		box->key,
		LV_GRID_ALIGN_STRETCH,0,2,
		LV_GRID_ALIGN_STRETCH,2,1
	);

	box->chk_string=lv_checkbox_create(box->box);
	lv_checkbox_set_text(box->chk_string,_("String"));
	lv_obj_set_checked(box->chk_string,true);
	lv_obj_add_event_cb(box->chk_string,radio_cb,LV_EVENT_VALUE_CHANGED,box);
	lv_obj_set_grid_cell(
		box->chk_string,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,3,1
	);

	box->chk_integer=lv_checkbox_create(box->box);
	lv_checkbox_set_text(box->chk_integer,_("Integer"));
	lv_obj_add_event_cb(box->chk_integer,radio_cb,LV_EVENT_VALUE_CHANGED,box);
	lv_obj_set_user_data(box->chk_integer,box);
	lv_obj_set_grid_cell(
		box->chk_integer,
		LV_GRID_ALIGN_START,1,1,
		LV_GRID_ALIGN_CENTER,3,1
	);

	box->chk_boolean=lv_checkbox_create(box->box);
	lv_checkbox_set_text(box->chk_boolean,_("Boolean"));
	lv_obj_add_event_cb(box->chk_boolean,radio_cb,LV_EVENT_VALUE_CHANGED,box);
	lv_obj_set_user_data(box->chk_boolean,box);
	lv_obj_set_grid_cell(
		box->chk_boolean,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,4,1
	);

	box->lbl_val=lv_label_create(box->box);
	lv_obj_set_small_text_font(box->lbl_val,LV_PART_MAIN);
	lv_label_set_text(box->lbl_val,_("Value:"));
	lv_obj_set_grid_cell(
		box->lbl_val,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,5,1
	);

	box->val=lv_textarea_create(box->box);
	lv_textarea_set_text(box->val,"");
	lv_obj_add_event_cb(box->val,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(
		box->val,
		LV_GRID_ALIGN_STRETCH,0,2,
		LV_GRID_ALIGN_STRETCH,6,1
	);

	box->ok=lv_btn_create(box->box);
	lv_obj_set_enabled(box->ok,*p);
	lv_obj_t*lbl_ok=lv_label_create(box->ok);
	lv_label_set_text(lbl_ok,LV_SYMBOL_OK);
	lv_obj_center(lbl_ok);
	lv_obj_add_event_cb(box->ok,btn_cb,LV_EVENT_CLICKED,box);
	lv_obj_set_grid_cell(
		box->ok,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_CENTER,7,1
	);

	box->cancel=lv_btn_create(box->box);
	lv_obj_t*lbl_cancel=lv_label_create(box->cancel);
	lv_label_set_text(lbl_cancel,LV_SYMBOL_CLOSE);
	lv_obj_center(lbl_cancel);
	lv_obj_add_event_cb(box->cancel,btn_cb,LV_EVENT_CLICKED,box);
	lv_obj_set_grid_cell(
		box->cancel,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_CENTER,7,1
	);

	lv_event_send(box->chk_string,LV_EVENT_VALUE_CHANGED,NULL);
	if(((char*)act->args)[0]){
		char val[128]={0},*s;
		switch(confd_get_type(p)){
			case TYPE_KEY:break;
			case TYPE_STRING:
				s=confd_get_string(p,"");
				lv_textarea_set_text(box->val,s?s:"");
				if(s)free(s);
			break;
			case TYPE_INTEGER:
				lv_obj_set_checked(box->chk_integer,true);
				lv_event_send(box->chk_integer,LV_EVENT_VALUE_CHANGED,NULL);
				snprintf(val,127,"%lld",(long long int)confd_get_integer(p,0));
				lv_textarea_set_text(box->val,val);
			break;
			case TYPE_BOOLEAN:
				lv_obj_set_checked(box->chk_boolean,true);
				lv_event_send(box->chk_boolean,LV_EVENT_VALUE_CHANGED,NULL);
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
	.icon="conftool.svg",
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

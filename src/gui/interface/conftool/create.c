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

static void createbox_repos(struct create_box*box){
	lv_coord_t px=gui_sw,py=gui_sh,bs;
	bs=lv_obj_get_y(box->val)+lv_obj_get_height(box->val);
	bs+=gui_font_size/2;
	lv_obj_set_y(box->ok,bs);
	lv_obj_set_y(box->cancel,bs);
	bs+=lv_obj_get_height(box->ok);
	bs+=gui_font_size/2;
	lv_obj_set_height(box->box,bs);
	px-=lv_obj_get_width(box->box),py-=bs;
	if(sysbar.keyboard)py-=lv_obj_get_height(sysbar.keyboard);
	px/=2,py/=2;
	lv_obj_set_pos(box->box,px,py);
}

static int createbox_get_focus(struct gui_activity*d){
	struct create_box*box=(struct create_box*)d->data;
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
	lv_group_remove_obj(box->key);
	lv_group_remove_obj(box->chk_string);
	lv_group_remove_obj(box->chk_integer);
	lv_group_remove_obj(box->chk_boolean);
	lv_group_remove_obj(box->val);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

static void radio_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	struct create_box*box=(struct create_box*)lv_obj_get_user_data(obj);
	if(!box)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	if(obj==box->chk_string){
		lv_checkbox_set_checked(box->chk_integer,false);
		lv_checkbox_set_checked(box->chk_boolean,false);
		lv_textarea_set_accepted_chars(box->val,NULL);
		lv_textarea_set_max_length(box->val,0);
		lv_textarea_set_text(box->val,"");
		lv_textarea_set_one_line(box->val,false);
	}else if(obj==box->chk_integer){
		lv_checkbox_set_checked(box->chk_string,false);
		lv_checkbox_set_checked(box->chk_boolean,false);
		lv_textarea_set_accepted_chars(box->val,NUMBER"-");
		lv_textarea_set_max_length(box->val,20);
		lv_textarea_set_text(box->val,"0");
		lv_textarea_set_one_line(box->val,true);
	}else if(obj==box->chk_boolean){
		lv_checkbox_set_checked(box->chk_string,false);
		lv_checkbox_set_checked(box->chk_integer,false);
		lv_textarea_set_accepted_chars(box->val,"01");
		lv_textarea_set_max_length(box->val,1);
		lv_textarea_set_text(box->val,"0");
		lv_textarea_set_one_line(box->val,true);
	}
	createbox_repos(box);
}

static void input_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj)return;
	struct create_box*box=(struct create_box*)lv_obj_get_user_data(obj);
	if(!box||guiact_get_last()->data!=box)return;
	const char*buf=lv_textarea_get_text(box->key);
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
			sysbar_keyboard_open();
			//fallthrough
		case LV_EVENT_DEFOCUSED:
		case LV_EVENT_FOCUSED:
			createbox_repos(box);
		break;
		case LV_EVENT_VALUE_CHANGED:
			if(obj!=box->key)break;
			lv_obj_set_enabled(box->ok,buf[0]!=0);
		break;
	}
}
static void do_save(const char*key,const char*val,struct create_box*box){
	size_t s=0;
	errno=0;
	confd_delete(key);
	if(lv_checkbox_is_checked(box->chk_string)){
		s=strlen(val);
		confd_set_string(key,(char*)val);
	}else if(lv_checkbox_is_checked(box->chk_integer)){
		s=sizeof(int64_t);
		char*end;
		int64_t num;
		errno=0,num=strtol(val,&end,10);
		if(errno==0&&end!=val&&*end==0)confd_set_integer(key,num);
		else if(errno==0)errno=EINVAL;
	}else if(lv_checkbox_is_checked(box->chk_boolean)){
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

static void btn_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct create_box*box=(struct create_box*)lv_obj_get_user_data(obj);
	if(!box||guiact_get_last()->data!=box)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	createbox_repos(box);
	if(obj==box->ok){
		const char*key=lv_textarea_get_text(box->key);
		const char*val=lv_textarea_get_text(box->val);
		if(confd_get_type(key)!=TYPE_KEY)do_save(key,val,box);
		else msgbox_set_user_data(msgbox_create_yesno(
			overwrite_cb,
			"'%s' is an exists key, do you want to overwrite it?",
			key
		),box);
	}else if(obj==box->cancel)guiact_do_back();
}

static int createbox_draw(struct gui_activity*act){
	if(!act->args)return -1;
	char*p=act->args+1;
	lv_coord_t box_h=0;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3,xw=MIN(max_w,cur_w);
	static struct create_box box;
	memset(&box,0,sizeof(struct create_box));
	act->data=&box;
	if(strcmp(p,"/")==0)p="";

	box.box=lv_obj_create(act->page,NULL);
	lv_obj_set_style_local_border_width(box.box,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(box.box,LV_OBJ_PART_MAIN,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_border_width(box.box,LV_OBJ_PART_MAIN,LV_STATE_FOCUSED,0);
	lv_obj_set_width(box.box,xw+gui_font_size);

	box_h+=gui_font_size/2;
	box.label=lv_label_create(box.box,NULL);
	lv_label_set_align(box.label,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(box.label,LV_LABEL_LONG_BREAK);
	lv_obj_set_pos(box.label,gui_font_size/2,box_h);
	lv_obj_set_width(box.label,xw);
	lv_label_set_text(
		box.label,((char*)act->args)[0]?
		_("Edit config item"):
		_("Create config item")
	);
	box_h+=lv_obj_get_height(box.label);

	lv_coord_t
		btn_m=gui_font_size/2,
		xm=btn_m/2+gui_font_size/2,btn_w=xw/2,
		btn_h=gui_font_size+(gui_dpi/8);
	box_h+=btn_m;

	box.lbl_key=lv_label_create(box.box,NULL);
	lv_obj_set_small_text_font(box.lbl_key,LV_LABEL_PART_MAIN);
	lv_label_set_text(box.lbl_key,_("Config item path:"));
	lv_obj_set_pos(box.lbl_key,xm,box_h);
	lv_obj_set_width(box.lbl_key,xw-btn_m);
	box_h+=lv_obj_get_height(box.lbl_key)+gui_dpi/20;

	box.key=lv_textarea_create(box.box,NULL);
	lv_textarea_set_text(box.key,p);
	lv_textarea_set_cursor_hidden(box.key,true);
	lv_textarea_set_one_line(box.key,true);
	lv_textarea_set_accepted_chars(box.key,VALID"-.");
	lv_obj_set_style_local_margin_bottom(box.key,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_pos(box.key,xm,box_h);
	lv_obj_set_user_data(box.key,&box);
	lv_obj_set_width(box.key,xw-btn_m);
	lv_obj_set_event_cb(box.key,input_cb);
	lv_obj_set_user_data(box.key,&box);
	box_h+=lv_obj_get_height(box.key)+btn_m;

	box.chk_string=lv_checkbox_create(box.box,NULL);
	lv_checkbox_set_text(box.chk_string,_("String"));
	lv_checkbox_set_checked(box.chk_string,true);
	lv_obj_set_event_cb(box.chk_string,radio_cb);
	lv_obj_set_user_data(box.chk_string,&box);
	lv_obj_set_pos(box.chk_string,xm,box_h);
	lv_style_set_focus_radiobox(box.chk_string);

	box.chk_integer=lv_checkbox_create(box.box,NULL);
	lv_checkbox_set_text(box.chk_integer,_("Integer"));
	lv_obj_set_event_cb(box.chk_integer,radio_cb);
	lv_obj_set_user_data(box.chk_integer,&box);
	lv_style_set_focus_radiobox(box.chk_integer);
	lv_obj_set_pos(
		box.chk_integer,
		xw-btn_m/2-
		lv_obj_get_width(box.chk_integer),
		box_h
	);
	box_h+=lv_obj_get_height(box.chk_integer)+btn_m;

	box.chk_boolean=lv_checkbox_create(box.box,NULL);
	lv_checkbox_set_text(box.chk_boolean,_("Boolean"));
	lv_obj_set_event_cb(box.chk_boolean,radio_cb);
	lv_obj_set_user_data(box.chk_boolean,&box);
	lv_obj_set_pos(box.chk_boolean,xm,box_h);
	lv_style_set_focus_radiobox(box.chk_boolean);
	box_h+=lv_obj_get_height(box.chk_boolean)+btn_m;

	box.lbl_val=lv_label_create(box.box,NULL);
	lv_obj_set_small_text_font(box.lbl_val,LV_LABEL_PART_MAIN);
	lv_label_set_text(box.lbl_val,_("Value:"));
	lv_obj_set_pos(box.lbl_val,xm,box_h);
	lv_obj_set_width(box.lbl_val,xw-btn_m);
	box_h+=lv_obj_get_height(box.lbl_val)+gui_dpi/20;

	box.val=lv_textarea_create(box.box,NULL);
	lv_textarea_set_text(box.val,"");
	lv_textarea_set_cursor_hidden(box.val,true);
	lv_obj_set_style_local_margin_bottom(box.val,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_pos(box.val,xm,box_h);
	lv_obj_set_event_cb(box.val,input_cb);
	lv_obj_set_user_data(box.val,&box);
	lv_obj_set_width(box.val,xw-btn_m);
	box_h+=lv_obj_get_height(box.val)+btn_m;

	box.ok=lv_btn_create(box.box,NULL);
	lv_obj_set_enabled(box.ok,*p);
	lv_label_set_text(lv_label_create(box.ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_style_local_margin_bottom(box.ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_style_local_radius(box.ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(box.ok,btn_w-btn_m,btn_h);
	lv_obj_set_user_data(box.ok,&box);
	lv_obj_set_event_cb(box.ok,btn_cb);
	lv_obj_set_x(box.ok,xm);

	box.cancel=lv_btn_create(box.box,NULL);
	lv_label_set_text(lv_label_create(box.cancel,NULL),LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_margin_bottom(box.cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_style_local_radius(box.cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(box.cancel,btn_w-btn_m,btn_h);
	lv_obj_set_user_data(box.cancel,&box);
	lv_obj_set_event_cb(box.cancel,btn_cb);
	lv_obj_set_x(box.cancel,xm+btn_w);

	lv_event_send(box.chk_string,LV_EVENT_VALUE_CHANGED,NULL);
	if(((char*)act->args)[0]){
		char val[128]={0},*s;
		switch(confd_get_type(p)){
			case TYPE_KEY:break;
			case TYPE_STRING:
				s=confd_get_string(p,"");
				lv_textarea_set_text(box.val,s?s:"");
				if(s)free(s);
			break;
			case TYPE_INTEGER:
				lv_checkbox_set_checked(box.chk_integer,true);
				lv_event_send(box.chk_integer,LV_EVENT_VALUE_CHANGED,NULL);
				snprintf(val,127,"%lld",(long long int)confd_get_integer(p,0));
				lv_textarea_set_text(box.val,val);
			break;
			case TYPE_BOOLEAN:
				lv_checkbox_set_checked(box.chk_boolean,true);
				lv_event_send(box.chk_boolean,LV_EVENT_VALUE_CHANGED,NULL);
				snprintf(val,127,"%d",confd_get_boolean(p,false));
				lv_textarea_set_text(box.val,val);
			break;
		}
	}

	createbox_repos(&box);
	return 0;
}

struct gui_register guireg_conftool_create={
	.name="config-manager-create",
	.title="Config Manager Create",
	.icon="conftool.png",
	.show_app=false,
	.get_focus=createbox_get_focus,
	.lost_focus=createbox_lost_focus,
	.draw=createbox_draw,
	.back=true,
	.mask=true
};
#endif

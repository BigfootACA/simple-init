/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<ctype.h>
#include<iconv.h>
#include<endian.h>
#include<string.h>
#include"str.h"
#include"gui.h"
#include"regedit.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "conftool"

enum digital_base{
	BASE_OCT=0,
	BASE_DEC=1,
	BASE_HEX=2,
};

struct edit_value{
	struct gui_activity*act;
	lv_obj_t*mask,*box,*label;
	lv_obj_t*key,*lbl_key;
	lv_obj_t*base,*lbl_base;
	lv_obj_t*val,*lbl_val;
	lv_obj_t*ok,*cancel;
	enum digital_base cur_base;
	bool loaded,is_number;
	struct regedit*reg;
	hive_h*hive;
	hive_node_h node;
	hive_value_h value;
	hive_type type;
};

static void load_string(struct edit_value*box){
	char*str;
	if(!(str=hivex_value_string(
		box->hive,
		box->value
	)))return;
	lv_textarea_set_text(box->val,str);
	lv_textarea_set_one_line(box->val,false);
	lv_textarea_set_accepted_chars(box->val,NULL);
	free(str);
}

static void load_multi_string(struct edit_value*box){
	char**strings;
	if(!(strings=hivex_value_multiple_strings(
		box->hive,box->value
	)))return;
	lv_textarea_set_text(box->val,"");
	for(int i=0;strings[i];i++){
		if(i>0)lv_textarea_add_char(box->val,'\n');
		lv_textarea_add_text(box->val,strings[i]);
		free(strings[i]);
	}
	lv_textarea_set_one_line(box->val,false);
	lv_textarea_set_accepted_chars(box->val,NULL);
	free(strings);
}

static void load_binary(struct edit_value*box){
	size_t len=0;
	char buf[4],*data;
	if(!(data=hivex_value_value(
		box->hive,box->value,NULL,&len
	)))return;
	lv_textarea_set_text(box->val,"");
	for(size_t i=0;i<len;i++){
		if(i>0&&!(i%12))lv_textarea_add_char(box->val,'\n');
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf)-1,"%02X",(uint8_t)data[i]);
		lv_textarea_add_text(box->val,buf);
	}
	lv_textarea_set_one_line(box->val,false);
	lv_textarea_set_accepted_chars(box->val,HEX" \n");
	free(data);
}

static void load_dword(struct edit_value*box){
	char buf[64],*fmt;
	int32_t v=hivex_value_dword(
		box->hive,box->value
	);
	switch(box->cur_base){
		case BASE_OCT:fmt="%o";break;
		case BASE_DEC:fmt="%d";break;
		case BASE_HEX:fmt="%x";break;
		default:return;
	}
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,fmt,v);
	lv_textarea_set_text(box->val,buf);
	lv_textarea_set_one_line(box->val,true);
	lv_textarea_set_accepted_chars(box->val,HEX);
	box->is_number=true;
}

static void load_qword(struct edit_value*box){
	char buf[128],*fmt;
	int64_t v=hivex_value_qword(
		box->hive,box->value
	);
	switch(box->cur_base){
		case BASE_OCT:fmt="%lo";break;
		case BASE_DEC:fmt="%ld";break;
		case BASE_HEX:fmt="%lx";break;
		default:return;
	}
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,fmt,v);
	lv_textarea_set_text(box->val,buf);
	lv_textarea_set_one_line(box->val,true);
	lv_textarea_set_accepted_chars(box->val,HEX);
	box->is_number=true;
}

static bool save_string(const char*val,struct hive_set_value*set){
	iconv_t cd=NULL;
	char*value=NULL,*rv=NULL;
	size_t len=strlen(val),size=(len+1)*2,os=size;
	if(!(value=rv=malloc(size)))goto fail;
	if(!(cd=iconv_open("UTF-16LE","UTF-8")))goto fail;
	memset(value,0,size);
	size_t r=iconv(cd,(char**)&val,&len,&value,&os);
	if(r==(size_t)-1)goto fail;
	set->value=rv,set->len=size;
	iconv_close(cd);
	return true;
	fail:
	if(cd)iconv_close(cd);
	if(rv)free(rv);
	return false;
}

static bool save_multi_string(const char*val,struct hive_set_value*set){
	wchar_t*wv;
	iconv_t cd=NULL;
	char*value=NULL,*rv=NULL;
	size_t len=strlen(val),size=(len+1)*2,os=size;
	if(!(value=rv=malloc(size)))goto fail;
	if(!(cd=iconv_open("UTF-16LE","UTF-8")))goto fail;
	memset(value,0,size);
	size_t r=iconv(cd,(char**)&val,&len,&value,&os);
	if(r==(size_t)-1)goto fail;
	wv=(wchar_t*)value;
	for(size_t i=0;wv[i];i++)if(wv[i]=='\n')wv[i]=0;
	set->value=rv,set->len=size;
	iconv_close(cd);
	return true;
	fail:
	if(cd)iconv_close(cd);
	if(rv)free(rv);
	return false;
}

static bool save_number(struct edit_value*box,const char*val,struct hive_set_value*set){
	int b;
	char*end=NULL;
	static int32_t i32;
	static int64_t i64;
	switch(box->cur_base){
		case BASE_OCT:b=8;break;
		case BASE_DEC:b=10;break;
		case BASE_HEX:b=16;break;
		default:tlog_warn("unsupported base");return false;
	}
	errno=0;
	int64_t value=strtoll(val,&end,b);
	if(*end||val==end||errno!=0){
		telog_warn("parse number failed");
		return false;
	}
	switch(box->type){
		case hive_t_REG_DWORD:{
			if(value<INT32_MIN||value>INT32_MAX)
				return false;
			i32=htole32((int32_t)value);
			set->len=sizeof(i32);
			set->value=memdup(&i32,set->len);
		}break;
		case hive_t_REG_DWORD_BIG_ENDIAN:{
			if(value<INT32_MIN||value>INT32_MAX)
				return false;
			i32=htobe32((int32_t)value);
			set->len=sizeof(i32);
			set->value=memdup(&i32,set->len);
		}break;
		case hive_t_REG_QWORD:{
			i64=htole64((int64_t)value);
			set->len=sizeof(i64);
			set->value=memdup(&i64,set->len);
		}break;
		default:return false;
	}
	return set->value!=NULL;
}

static bool save_binary(const char*val,struct hive_set_value*set){
	size_t len=strlen(val),size=len/2,real_size=0;
	if(!(set->value=malloc(size)))return false;
	memset(set->value,0,size);
	for(size_t i=0;i<len&&real_size<len;i++){
		if(isspace(val[i]))continue;
		uint8_t v=0;
		if(val[i]>='0'&&val[i]<='9')v=val[i]-'0';
		else if(val[i]>='a'&&val[i]<='f')v=val[i]-'a'+0xA;
		else if(val[i]>='A'&&val[i]<='F')v=val[i]-'A'+0xA;
		else goto fail;
		set->value[real_size/2]|=v;
		if(!(real_size%2))set->value[real_size/2]<<=4;
		real_size++;
	}
	set->len=real_size/2;
	return true;
	fail:
	free(set->value);
	set->value=NULL;
	return false;
}

static int save(struct edit_value*box){
	bool ret=false;
	struct hive_set_value set;
	const char*key=lv_textarea_get_text(box->key);
	const char*val=lv_textarea_get_text(box->val);
	set.key=(char*)key,set.t=box->type;
	switch(box->type){
		case hive_t_REG_EXPAND_SZ:
		case hive_t_REG_SZ:ret=save_string(val,&set);break;
		case hive_t_REG_BINARY:ret=save_binary(val,&set);break;
		case hive_t_REG_DWORD:
		case hive_t_REG_DWORD_BIG_ENDIAN:
		case hive_t_REG_QWORD:ret=save_number(box,val,&set);break;
		case hive_t_REG_MULTI_SZ:ret=save_multi_string(val,&set);break;
		default:ret=save_binary(val,&set);break;
	}
	if(!ret){
		if(set.value)free(set.value);
		msgbox_alert(
			"Parse value failed: %s",
			_(errno>0?strerror(errno):"Unknown")
		);
		return -1;
	}
	int r=hivex_node_set_value(box->hive,box->node,&set,0);
	if(set.value)free(set.value);
	if(r!=0){
		msgbox_alert(
			"Set value failed: %s",
			_(errno>0?strerror(errno):"Unknown")
		);
		return -1;
	}
	box->reg->changed=true;
	guiact_do_back();
	return 0;
}

static void edit_value_repos(struct edit_value*box){
	lv_coord_t px=gui_sw,py=gui_sh,bs;
	bs=lv_obj_get_y(box->val)+lv_obj_get_height(box->val);
	bs+=gui_font_size/2;
	lv_obj_set_y(box->ok,bs);
	lv_obj_set_y(box->cancel,bs);
	bs+=lv_obj_get_height(box->ok);
	bs+=gui_font_size/2;
	lv_obj_set_height(box->box,bs);
	px-=lv_obj_get_width(box->box),py-=bs;
	px/=2,py/=2;
	lv_obj_set_pos(box->box,px,py);
}

static int edit_value_get_focus(struct gui_activity*d){
	struct edit_value*box=(struct edit_value*)d->data;
	if(!box)return 0;
	lv_group_add_obj(gui_grp,box->key);
	lv_group_add_obj(gui_grp,box->val);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int edit_value_lost_focus(struct gui_activity*d){
	struct edit_value*box=(struct edit_value*)d->data;
	if(!box)return 0;
	lv_group_remove_obj(box->key);
	lv_group_remove_obj(box->val);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

static void input_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e==LV_EVENT_DELETE)return;
	struct edit_value*box=(struct edit_value*)lv_obj_get_user_data(obj);
	if(!box||guiact_get_last()->data!=box)return;
	const char*buf=lv_textarea_get_text(box->key);
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
			sysbar_keyboard_open();
			//fallthrough
		case LV_EVENT_DEFOCUSED:
		case LV_EVENT_FOCUSED:
			edit_value_repos(box);
		break;
		case LV_EVENT_VALUE_CHANGED:
			if(obj!=box->key)break;
			lv_obj_set_enabled(box->ok,buf[0]!=0);
		break;
	}
}

static void btn_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct edit_value*box=lv_obj_get_user_data(obj);
	if(!box||guiact_get_last()->data!=box)return;
	sysbar_focus_input(NULL);
	sysbar_keyboard_close();
	edit_value_repos(box);
	if(obj==box->cancel)guiact_do_back();
	else if(obj==box->ok)save(box);
}

static void dropdown_cb(lv_obj_t*obj,lv_event_t e){
	int b;
	const char*text,*fmt;
	char buf[128],*end=NULL;
	if(e==LV_EVENT_DELETE)return;
	lv_default_dropdown_cb(obj,e);
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct edit_value*box=lv_obj_get_user_data(obj);
	uint16_t base=lv_dropdown_get_selected(box->base);
	if(base==box->cur_base)return;
	text=lv_textarea_get_text(box->val);
	if(!text||!*text)return;
	switch(box->cur_base){
		case BASE_OCT:b=8;break;
		case BASE_DEC:b=10;break;
		case BASE_HEX:b=16;break;
		default:return;
	}
	switch(base){
		case BASE_OCT:fmt="%lo";break;
		case BASE_DEC:fmt="%ld";break;
		case BASE_HEX:fmt="%lx";break;
		default:return;
	}
	box->cur_base=base,errno=0;
	int64_t value=strtoll(text,&end,b);
	if(*end||text==end||errno!=0)return;
	memset(buf,0,sizeof(buf));
	snprintf(buf,sizeof(buf)-1,fmt,value);
	lv_textarea_set_text(box->val,buf);
}

static int edit_value_init(struct gui_activity*act){
	struct edit_value*box=malloc(sizeof(struct edit_value));
	if(!box)ERET(ENOMEM);
	memset(box,0,sizeof(struct edit_value));
	act->data=box,box->act=act;
	if(act->args){
		struct regedit_value*val=act->args;
		box->value=val->value;
		box->hive=val->hive;
		box->node=val->node;
		box->reg=val->reg;
	}
	return 0;
}

static int edit_value_clean(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int edit_value_resize(struct gui_activity*act){
	struct edit_value*box=act->data;
	lv_coord_t
		box_h=0,
		max_w=gui_dpi*4,
		cur_w=act->w/4*3,
		xw=MIN(max_w,cur_w),
		btn_m=gui_font_size/2,
		xm=btn_m/2+gui_font_size/2,
		btn_w=xw/2,
		btn_h=gui_font_size+(gui_dpi/8);

	lv_obj_set_width(box->box,xw+gui_font_size);
	box_h+=gui_font_size/2;
	lv_obj_set_pos(box->label,gui_font_size/2,box_h);
	lv_obj_set_width(box->label,xw);
	box_h+=lv_obj_get_height(box->label)+btn_m;
	lv_obj_set_pos(box->lbl_key,xm,box_h);
	lv_obj_set_width(box->lbl_key,xw-btn_m);
	box_h+=lv_obj_get_height(box->lbl_key)+gui_dpi/20;
	lv_obj_set_style_local_margin_bottom(
		box->key,
		LV_TEXTAREA_PART_BG,
		LV_STATE_DEFAULT,
		btn_m
	);
	lv_obj_set_pos(box->key,xm,box_h);
	lv_obj_set_width(box->key,xw-btn_m);
	box_h+=lv_obj_get_height(box->key)+btn_m;
	lv_obj_set_hidden(box->base,!box->is_number);
	lv_obj_set_hidden(box->lbl_base,!box->is_number);
	if(box->is_number){
		lv_obj_set_pos(box->lbl_base,xm,box_h);
		lv_obj_set_width(box->lbl_base,xw-btn_m);
		box_h+=lv_obj_get_height(box->lbl_base)+gui_dpi/20;
		lv_obj_set_style_local_margin_bottom(
			box->base,
			LV_TEXTAREA_PART_BG,
			LV_STATE_DEFAULT,
			btn_m
		);
		lv_obj_set_pos(box->base,xm,box_h);
		lv_obj_set_width(box->base,xw-btn_m);
		box_h+=lv_obj_get_height(box->base)+btn_m;
	}
	lv_obj_set_pos(box->lbl_val,xm,box_h);
	lv_obj_set_width(box->lbl_val,xw-btn_m);
	box_h+=lv_obj_get_height(box->lbl_val)+gui_dpi/20;
	lv_obj_set_style_local_margin_bottom(
		box->val,
		LV_TEXTAREA_PART_BG,
		LV_STATE_DEFAULT,
		btn_m
	);
	lv_obj_set_pos(box->val,xm,box_h);
	lv_obj_set_width(box->val,xw-btn_m);
	box_h+=lv_obj_get_height(box->val)+btn_m;
	lv_obj_set_style_local_margin_bottom(
		box->ok,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		btn_m
	);
	lv_obj_set_style_local_radius(
		box->ok,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_dpi/15
	);
	lv_obj_set_size(box->ok,btn_w-btn_m,btn_h);
	lv_obj_set_x(box->ok,xm);
	lv_obj_set_style_local_margin_bottom(
		box->cancel,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		btn_m
	);
	lv_obj_set_style_local_radius(
		box->cancel,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_dpi/15
	);
	lv_obj_set_size(box->cancel,btn_w-btn_m,btn_h);
	lv_obj_set_x(box->cancel,xm+btn_w);
	edit_value_repos(box);
	return 0;
}

static int edit_value_load_data(struct gui_activity*act){
	struct edit_value*box=act->data;
	if(box->loaded)return 0;
	if(!box->hive)return -1;
	char*name;
	if((name=hivex_value_key(box->hive,box->value))){
		lv_textarea_set_text(box->key,name);
		free(name);
	}
	if(hivex_value_type(
		box->hive,box->value,
		&box->type,NULL
	)==0)switch(box->type){
		case hive_t_REG_DWORD_BIG_ENDIAN:
		case hive_t_REG_DWORD:load_dword(box);break;
		case hive_t_REG_QWORD:load_qword(box);break;
		case hive_t_REG_SZ:
		case hive_t_REG_EXPAND_SZ:load_string(box);break;
		case hive_t_REG_BINARY:load_binary(box);break;
		case hive_t_REG_MULTI_SZ:load_multi_string(box);break;
		default:
			tlog_warn(
				"unsupported value type %s, use binary",
				hivex_type_to_string(box->type)
			);
			load_multi_string(box);
		break;
	}
	box->loaded=true;
	return edit_value_resize(act);
}

static int edit_value_draw(struct gui_activity*act){
	struct edit_value*box=act->data;
	if(!box)return -1;

	box->box=lv_obj_create(act->page,NULL);
	lv_obj_set_style_local_border_width(box->box,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(box->box,LV_OBJ_PART_MAIN,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_border_width(box->box,LV_OBJ_PART_MAIN,LV_STATE_FOCUSED,0);

	box->label=lv_label_create(box->box,NULL);
	lv_label_set_align(box->label,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(box->label,LV_LABEL_LONG_BREAK);
	lv_label_set_text(box->label,_("Edit registry value"));

	box->lbl_key=lv_label_create(box->box,NULL);
	lv_obj_set_small_text_font(box->lbl_key,LV_LABEL_PART_MAIN);
	lv_label_set_text(box->lbl_key,_("Value name:"));

	box->key=lv_textarea_create(box->box,NULL);
	lv_textarea_set_cursor_hidden(box->key,true);
	lv_textarea_set_one_line(box->key,true);
	lv_obj_set_event_cb(box->key,input_cb);
	lv_obj_set_user_data(box->key,box);
	lv_textarea_set_text(box->key,"");

	box->lbl_base=lv_label_create(box->box,NULL);
	lv_obj_set_small_text_font(box->lbl_base,LV_LABEL_PART_MAIN);
	lv_label_set_text(box->lbl_base,_("Digital base:"));

	box->base=lv_dropdown_create(box->box,NULL);
	lv_obj_set_event_cb(box->base,dropdown_cb);
	lv_obj_set_user_data(box->base,box);
	lv_dropdown_clear_options(box->base);
	lv_dropdown_add_option(box->base,_("Octal (8)"),BASE_OCT);
	lv_dropdown_add_option(box->base,_("Decimal (10)"),BASE_DEC);
	lv_dropdown_add_option(box->base,_("Hexadecimal (16)"),BASE_HEX);
	lv_dropdown_set_selected(box->base,BASE_DEC);
	box->cur_base=BASE_DEC;

	box->lbl_val=lv_label_create(box->box,NULL);
	lv_obj_set_small_text_font(box->lbl_val,LV_LABEL_PART_MAIN);
	lv_label_set_text(box->lbl_val,_("Value:"));

	box->val=lv_textarea_create(box->box,NULL);
	lv_textarea_set_text(box->val,"");
	lv_textarea_set_cursor_hidden(box->val,true);
	lv_obj_set_event_cb(box->val,input_cb);
	lv_obj_set_user_data(box->val,box);

	box->ok=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_user_data(box->ok,box);
	lv_obj_set_event_cb(box->ok,btn_cb);

	box->cancel=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->cancel,NULL),LV_SYMBOL_CLOSE);

	lv_obj_set_user_data(box->cancel,box);
	lv_obj_set_event_cb(box->cancel,btn_cb);

	return 0;
}

struct gui_register guireg_regedit_value={
	.name="regedit-edit-value",
	.title="Edit Registry Value",
	.icon="regedit.svg",
	.show_app=false,
	.init=edit_value_init,
	.quiet_exit=edit_value_clean,
	.resize=edit_value_resize,
	.get_focus=edit_value_get_focus,
	.lost_focus=edit_value_lost_focus,
	.data_load=edit_value_load_data,
	.draw=edit_value_draw,
	.back=true,
	.mask=true
};
#endif
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<string.h>
#include"gui.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/clipboard.h"
#include"gui/filepicker.h"
#define TAG "editor"

static char*cur_path=NULL;
static char*last_search=NULL;
static bool changed=false;
static lv_obj_t*text;
static lv_obj_t*btn_open,*btn_new,*btn_save,*btn_copy,*btn_paste,*btn_menu;
static lv_coord_t btx,btm,btw,bth;

static const char*adv_menu[]={
	"Copy selected",
	"Cut selected",
	"Paste",
	"Open file",
	"New file",
	"Save",
	"Save as",
	"Search",
	""
};

static int editor_get_focus(struct gui_activity*act __attribute__((unused))){
	lv_group_add_obj(gui_grp,text);
	lv_group_add_obj(gui_grp,btn_open);
	lv_group_add_obj(gui_grp,btn_new);
	lv_group_add_obj(gui_grp,btn_save);
	lv_group_add_obj(gui_grp,btn_copy);
	lv_group_add_obj(gui_grp,btn_paste);
	lv_group_add_obj(gui_grp,btn_menu);
	lv_obj_set_enabled(btn_paste,clipboard_get_type()==CLIP_TEXT);
	sysbar_focus_input(text);
	ctrl_pad_set_target(text);
	return 0;
}

static int editor_lost_focus(struct gui_activity*act __attribute__((unused))){
	lv_group_remove_obj(text);
	lv_group_remove_obj(btn_open);
	lv_group_remove_obj(btn_new);
	lv_group_remove_obj(btn_save);
	lv_group_remove_obj(btn_copy);
	lv_group_remove_obj(btn_paste);
	lv_group_remove_obj(btn_menu);
	sysbar_focus_input(NULL);
	ctrl_pad_set_target(NULL);
	return 0;
}

static void set_changed(bool v){
	changed=v;
	lv_obj_set_enabled(btn_save,v);
}

static void do_copy(){
	const char*cont=lv_textarea_get_text(text);
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
	uint32_t i=ext->sel_end-ext->sel_start;
	if(i==0)return;
	clipboard_set(CLIP_TEXT,cont+ext->sel_start,i);
	lv_obj_set_enabled(btn_paste,true);
}

static void do_cut(){
	do_copy();
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
	uint32_t ss=ext->sel_start,sl=ext->sel_end-ss;
	lv_textarea_remove_text(text,ss,sl);
	lv_obj_set_enabled(btn_copy,false);
	lv_textarea_set_cursor_pos(text,ss);
}

static void do_paste(){
	if(clipboard_get_type()!=CLIP_TEXT)return;
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	if(sl>0){
		lv_textarea_remove_text(text,ss,sl);
		lv_obj_set_enabled(btn_copy,false);
		lv_textarea_set_cursor_pos(text,ss);
	}
	lv_textarea_add_text(text,clipboard_get_content());
}

struct open_data{
	char*path;
	uint32_t size;
	lv_fs_file_t file;
};

static bool open_read_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data){
	struct open_data*od=user_data;
	char*buf;
	uint32_t rs=0;
	lv_fs_res_t res;
	if(id==0){
		if(cur_path)free(cur_path);
		cur_path=od->path;
		set_changed(false);
		lv_textarea_clear_selection(text);
		lv_textarea_set_text(text,"");
		lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
		ext->sel_start=ext->sel_end=0;
		lv_obj_set_enabled(btn_copy,false);
		if(od->size==0){
			size_t bs=BUFSIZ,cs=bs;
			if(!(buf=malloc(bs)))goto e_buf;
			char*xb=buf,*x;
			for(;;){
				if(cs>0x800000){
					msgbox_alert("File too large; size limit is 8MiB, file is %d bytes",(int)cs);
					goto fail;
				}
				memset(xb,0,bs);
				rs=bs;
				if((res=lv_fs_read(&od->file,xb,rs,&rs))!=LV_FS_RES_OK)goto e_read;
				if(rs<bs)break;
				cs+=bs;
				if(!(x=realloc(buf,cs)))goto e_buf;
				buf=x,xb=buf+cs-bs;
			}
		}else{
			if(!(buf=malloc(od->size+1)))goto e_buf;
			memset(buf,0,od->size+1);
			if((res=lv_fs_read(&od->file,buf,od->size,&rs))!=LV_FS_RES_OK)goto e_read;
		}
		lv_textarea_set_text(text,buf);
		lv_textarea_set_cursor_pos(text,0);
		free(buf);
		set_changed(false);
		tlog_debug("read %d bytes",rs);
	}
	end:
	lv_fs_close(&od->file);
	free(od);
	return false;
	fail:
	free(od->path);
	cur_path=NULL;
	goto end;
	e_buf:
	msgbox_alert("Allocate file buffer failed");
	goto fail;
	e_read:
	msgbox_alert("Read file failed: %s",lv_fs_res_to_i18n_string(res));
	goto fail;
}

static void open_start_read(lv_task_t*t){
	open_read_cb(0,NULL,t->user_data);
}

static bool read_file(const char*path){
	lv_res_t res;
	struct open_data*od=malloc(sizeof(struct open_data));
	memset(od,0,sizeof(struct open_data));
	if(!(od->path=strdup(path)))return true;
	if((res=lv_fs_open(&od->file,od->path,LV_FS_MODE_RD))!=LV_FS_RES_OK){
		msgbox_alert("Open file failed: %s",lv_fs_res_to_i18n_string(res));
		free(od->path);
		return false;
	}
	if((res=lv_fs_size(&od->file,&od->size))!=LV_FS_RES_OK){
		msgbox_alert("Get file size failed: %s",lv_fs_res_to_i18n_string(res));
		lv_fs_close(&od->file);
		free(od->path);
		return false;
	}
	if(od->size>0x800000){
		msgbox_alert("File too large; size limit is 8MiB, file is %d bytes",od->size);
		lv_fs_close(&od->file);
		free(od->path);
		return false;
	}
	if(od->size<0x10000)lv_task_once(lv_task_create(open_start_read,20,LV_TASK_PRIO_LOWEST,od));
	else msgbox_set_user_data(msgbox_create_yesno(
		open_read_cb,
		"This file is bigger than 64 KiB (%d bytes), "
		"are you sure you want to open this file?",od->size
	),od);
	return true;
}

static bool open_select_cb(bool ok,const char**path,uint16_t cnt,void*user_data __attribute__((unused))){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	return !read_file(path[0]);
}

static bool open_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0)filepicker_set_max_item(filepicker_create(open_select_cb,"Open file"),1);
	return false;
}

static void do_open(){
	if(!changed)open_cb(0,NULL,NULL);
	else msgbox_create_yesno(
		open_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	);
}

static bool new_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0){
		if(cur_path)free(cur_path);
		cur_path=NULL;
		lv_textarea_set_text(text,"");
		lv_textarea_clear_selection(text);
		lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
		ext->sel_start=ext->sel_end=0;
		lv_obj_set_enabled(btn_copy,false);
		set_changed(false);
	}
	return false;
}

static void do_new(){
	if(!changed)new_cb(0,NULL,NULL);
	else msgbox_create_yesno(
		new_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	);
}

static void write_save(){
	lv_fs_file_t file;
	lv_res_t res;
	const char*cont=lv_textarea_get_text(text);
	uint32_t s=(uint32_t)strlen(cont),bs=0;
	lv_fs_remove(cur_path);
	if((res=lv_fs_open(&file,cur_path,LV_FS_MODE_WR))!=LV_FS_RES_OK){
		msgbox_alert("Open file failed: %s",lv_fs_res_to_i18n_string(res));
		return;
	}
	if((res=lv_fs_write(&file,cont,s,&bs))!=LV_FS_RES_OK){
		msgbox_alert("Write file failed: %s",lv_fs_res_to_i18n_string(res));
		lv_fs_close(&file);
		return;
	}
	lv_fs_close(&file);
	tlog_debug("wrote %d of %d bytes",bs,s);
	if(bs!=s){
		msgbox_alert("Write file maybe failed, wrote %d of %d bytes",bs,s);
		return;
	}
	set_changed(false);
}

static bool save_select_cb(bool ok,const char**path,uint16_t cnt,void*user_data __attribute__((unused))){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	if(cur_path)free(cur_path);
	if(!(cur_path=strdup(path[0])))return true;
	write_save();
	return false;
}

static void do_save_as(){
	filepicker_set_max_item(filepicker_create(save_select_cb,"Save file"),1);
}

static void do_save(){
	if(!cur_path)do_save_as();
	else write_save();
}

static bool search_rev_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data __attribute__((unused))){
	if(id==0){
		const char*cont=lv_textarea_get_text(text);
		char*ret=strstr(cont,last_search);
		if(ret)lv_textarea_set_cursor_pos(text,ret-cont);
		else msgbox_create_yesno(NULL,"No matching string found");
	}
	return false;
}

static bool search_cb(bool ok,const char*content,void*user_data __attribute__((unused))){
	if(!ok)return false;
	if(last_search)free(last_search);
	if(!(last_search=strdup(content)))return true;
	uint32_t pos=lv_textarea_get_cursor_pos(text);
	const char*cont=lv_textarea_get_text(text);
	char*ret=strstr(cont+pos+1,last_search);
	if(ret){
		uint32_t match=ret-cont;
		tlog_debug("found match string at %d",match);
		lv_textarea_set_cursor_pos(text,match);
	}else msgbox_create_yesno(
		search_rev_cb,
		"Searched to the end of the file, do you want "
		"to go back to the head and continue searching?"
	);
	return false;
}

static void do_search(){
	if(!*(lv_textarea_get_text(text)))return;
	struct inputbox*in=inputbox_create(search_cb,"Search text");
	if(last_search)inputbox_set_content(in,"%s",last_search);
}

static bool menu_cb(uint16_t id,const char*name __attribute__((unused)),void*data __attribute__((unused))){
	switch(id){
		case 0:do_copy();break;
		case 1:do_cut();break;
		case 2:do_paste();break;
		case 3:do_open();break;
		case 4:do_new();break;
		case 5:do_save();break;
		case 6:do_save_as();break;
		case 7:do_search();break;
	}
	return false;
}

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(strcmp(guiact_get_last()->name,"text-editor")!=0)return;
	tlog_info("click button %s",(char*)lv_obj_get_user_data(obj));
	if(obj==btn_open)do_open();
	else if(obj==btn_new)do_new();
	else if(obj==btn_save)do_save();
	else if(obj==btn_copy)do_copy();
	else if(obj==btn_paste)do_paste();
	else if(obj==btn_menu)msgbox_create_custom(menu_cb,adv_menu,"Menu");
}

static void input_cb(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE)return;
	if(obj!=text||strcmp(guiact_get_last()->name,"text-editor")!=0)return;
	lv_coord_t fh=gui_sh-bth-btm*2,sh=fh;
	if(sysbar.keyboard)sh=gui_sh-lv_obj_get_height(sysbar.keyboard);
	lv_textarea_ext_t*ext=lv_obj_get_ext_attr(text);
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	lv_obj_set_enabled(btn_copy,sl>0);
	lv_textarea_set_cursor_hidden(text,false);
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
		break;
		case LV_EVENT_VALUE_CHANGED:
			set_changed(true);
		break;
		case LV_EVENT_FOCUSED:
		case LV_EVENT_DEFOCUSED:
			lv_obj_set_height(text,MIN(sh,fh));
			lv_obj_set_y(lv_page_get_scrl(sysbar.content),0);
		break;
		case LV_EVENT_INSERT:
			if(*(char*)lv_event_get_data()==127&&sl>0){
				lv_textarea_set_insert_replace(text,"");
				lv_textarea_remove_text(text,ss,sl);
				lv_obj_set_enabled(btn_copy,false);
				lv_textarea_set_cursor_pos(text,ss);
			}
		break;
	}
}

static bool back_cb(uint16_t id,const char*name __attribute__((unused)),void*data __attribute__((unused))){
	if(id==0){
		changed=false;
		guiact_do_back();
		guiact_do_back();
		return true;
	}
	return false;
}

static int do_back(struct gui_activity*act __attribute__((unused))){
	if(!changed)return 0;
	msgbox_create_yesno(
		back_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	);
	return 1;
}

static int do_clean(struct gui_activity*act __attribute__((unused))){
	if(cur_path)free(cur_path);
	cur_path=NULL,changed=false;
	return 0;
}

static int editor_draw(struct gui_activity*act){
	btx=gui_font_size,btm=btx/2,btw=(gui_sw-btm)/6-btm,bth=btx*2;

	text=lv_textarea_create(act->page,NULL);
	lv_textarea_clear_selection(text);
	lv_textarea_set_text(text,"");
	lv_textarea_set_one_line(text,false);
	lv_textarea_set_text_sel(text,true);
	lv_obj_set_size(text,gui_sw,gui_sh-bth-btm*2);
	lv_obj_set_event_cb(text,input_cb);
	lv_obj_set_style_local_border_width(text,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_radius(text,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_small_text_font(text,LV_TEXTAREA_PART_BG);

	btn_open=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_open,btw,bth);
	lv_obj_set_event_cb(btn_open,btns_cb);
	lv_obj_set_user_data(btn_open,"open");
	lv_obj_align(btn_open,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(btn_open,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_open,NULL),LV_SYMBOL_UPLOAD);

	btn_new=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_new,btw,bth);
	lv_obj_set_event_cb(btn_new,btns_cb);
	lv_obj_set_user_data(btn_new,"new");
	lv_obj_align(btn_new,btn_open,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_new,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_new,NULL),LV_SYMBOL_PLUS);

	btn_save=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_save,false);
	lv_obj_set_size(btn_save,btw,bth);
	lv_obj_set_event_cb(btn_save,btns_cb);
	lv_obj_set_user_data(btn_save,"save");
	lv_obj_align(btn_save,btn_new,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_save,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_save,NULL),LV_SYMBOL_SAVE);

	btn_copy=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_copy,false);
	lv_obj_set_size(btn_copy,btw,bth);
	lv_obj_set_event_cb(btn_copy,btns_cb);
	lv_obj_set_user_data(btn_copy,"copy");
	lv_obj_align(btn_copy,btn_save,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_copy,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_copy,NULL),LV_SYMBOL_COPY);

	btn_paste=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(btn_paste,clipboard_get_type()==CLIP_TEXT);
	lv_obj_set_size(btn_paste,btw,bth);
	lv_obj_set_event_cb(btn_paste,btns_cb);
	lv_obj_set_user_data(btn_paste,"paste");
	lv_obj_align(btn_paste,btn_copy,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_paste,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_paste,NULL),LV_SYMBOL_PASTE);

	btn_menu=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_menu,btw,bth);
	lv_obj_set_event_cb(btn_menu,btns_cb);
	lv_obj_set_user_data(btn_menu,"menu");
	lv_obj_align(btn_menu,btn_paste,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(btn_menu,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(btn_menu,NULL),LV_SYMBOL_LIST);

	set_changed(false);
	if(act->args){
		char*path=(char*)act->args;
		if(!*path)return -1;
		if(strlen(path)<3||path[1]!=':')
			return trlog_warn(-1,"invalid file %s",path);
		read_file(path);
	}
	return 0;
}

struct gui_register guireg_text_edit={
	.name="text-editor",
	.title="Text Editor",
	.icon="text-editor.svg",
	.show_app=true,
	.open_file=true,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.draw=editor_draw,
	.lost_focus=editor_lost_focus,
	.get_focus=editor_get_focus,
	.back=true
};
#endif

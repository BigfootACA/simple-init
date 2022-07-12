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
#include"array.h"
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
static lv_obj_t*text,*btns;
static struct te_button{
	char*name;
	char*symbol;
	bool enabled;
	lv_obj_t*btn;
	lv_obj_t*lbl;
}te_btns[]={
	{ .name="open",  .symbol=LV_SYMBOL_UPLOAD, .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="new",   .symbol=LV_SYMBOL_PLUS,   .enabled=true,  .btn=NULL, .lbl=NULL },
	{ .name="save",  .symbol=LV_SYMBOL_SAVE,   .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="copy",  .symbol=LV_SYMBOL_COPY,   .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="paste", .symbol=LV_SYMBOL_PASTE,  .enabled=false, .btn=NULL, .lbl=NULL },
	{ .name="menu",  .symbol=LV_SYMBOL_LIST,   .enabled=true,  .btn=NULL, .lbl=NULL },
};

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

static void set_btn_enabled(char*name,bool enabled){
	for(size_t i=0;i<ARRLEN(te_btns);i++)
		if(strcmp(te_btns[i].name,name)==0)
			lv_obj_set_enabled(te_btns[i].btn,enabled);
}

static int editor_get_focus(struct gui_activity*act __attribute__((unused))){
	lv_group_add_obj(gui_grp,text);
	for(size_t i=0;i<ARRLEN(te_btns);i++)
		lv_group_add_obj(gui_grp,te_btns[i].btn);
	set_btn_enabled("paste",clipboard_get_type()==CLIP_TEXT);
	sysbar_focus_input(text);
	ctrl_pad_set_target(text);
	return 0;
}

static int editor_lost_focus(struct gui_activity*act __attribute__((unused))){
	lv_group_remove_obj(text);
	for(size_t i=0;i<ARRLEN(te_btns);i++)
		lv_group_remove_obj(te_btns[i].btn);
	sysbar_focus_input(NULL);
	ctrl_pad_set_target(NULL);
	return 0;
}

static void set_changed(bool v){
	changed=v;
	set_btn_enabled("save",v);
}

static void do_copy(){
	const char*cont=lv_textarea_get_text(text);
	lv_textarea_t*ext=(lv_textarea_t*)text;
	uint32_t i=ext->sel_end-ext->sel_start;
	if(i==0)return;
	clipboard_set(CLIP_TEXT,cont+ext->sel_start,i);
	set_btn_enabled("paste",true);
}

static void do_cut(){
	do_copy();
	lv_textarea_t*ext=(lv_textarea_t*)text;
	uint32_t ss=ext->sel_start,sl=ext->sel_end-ss;
	lv_textarea_remove_text(text,ss,sl);
	set_btn_enabled("copy",false);
	lv_textarea_set_cursor_pos(text,ss);
}

static void do_paste(){
	if(clipboard_get_type()!=CLIP_TEXT)return;
	lv_textarea_t*ext=(lv_textarea_t*)text;
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	if(sl>0){
		lv_textarea_remove_text(text,ss,sl);
		set_btn_enabled("copy",false);
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
	char*buf=NULL;
	uint32_t rs=0;
	lv_fs_res_t res;
	if(id==0){
		if(cur_path)free(cur_path);
		cur_path=od->path;
		set_changed(false);
		lv_textarea_clear_selection(text);
		lv_textarea_set_text(text,"");
		lv_textarea_t*ext=(lv_textarea_t*)text;
		ext->sel_start=ext->sel_end=0;
		set_btn_enabled("copy",false);
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
				res=lv_fs_read(&od->file,xb,rs,&rs);
				if(res!=LV_FS_RES_OK)goto e_read;
				if(rs<bs)break;
				cs+=bs;
				if(!(x=realloc(buf,cs)))goto e_buf;
				buf=x,xb=buf+cs-bs;
			}
		}else{
			if(!(buf=malloc(od->size+1)))goto e_buf;
			memset(buf,0,od->size+1);
			res=lv_fs_read(&od->file,buf,od->size,&rs);
			if(res!=LV_FS_RES_OK)goto e_read;
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

static void open_start_read(lv_timer_t*t){
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
	if(od->size<0x10000)lv_timer_set_repeat_count(lv_timer_create(open_start_read,20,od),1);
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
		lv_textarea_t*ext=(lv_textarea_t*)text;
		ext->sel_start=ext->sel_end=0;
		set_btn_enabled("copy",false);
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

static void btns_cb(lv_event_t*e){
	if(strcmp(guiact_get_last()->name,"text-editor")!=0)return;
	char*btn=(char*)e->user_data;
	tlog_info("click button %s",btn);
	if(strcmp(btn,"open")==0)do_open();
	else if(strcmp(btn,"new")==0)do_new();
	else if(strcmp(btn,"save")==0)do_save();
	else if(strcmp(btn,"copy")==0)do_copy();
	else if(strcmp(btn,"paste")==0)do_paste();
	else if(strcmp(btn,"menu")==0)msgbox_create_custom(menu_cb,adv_menu,"Menu");
}

static void text_changed_cb(lv_event_t*e __attribute__((unused))){
	lv_textarea_t*ext=(lv_textarea_t*)text;
	set_btn_enabled("copy",ext->sel_end-ext->sel_start>0);
	set_changed(true);
}

static void click_input_cb(lv_event_t*e){
	sysbar_focus_input(e->target);
}

static void insert_cb(lv_event_t*e){
	lv_textarea_t*ext=(lv_textarea_t*)text;
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	if(*(char*)e->param==127&&sl>0){
		lv_textarea_set_insert_replace(text,"");
		lv_textarea_remove_text(text,ss,sl);
		set_btn_enabled("copy",false);
		lv_textarea_set_cursor_pos(text,ss);
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

static int editor_resize(struct gui_activity*act __attribute__((unused))){
	if(sysbar.keyboard)lv_obj_add_flag(btns,LV_OBJ_FLAG_HIDDEN);
	else lv_obj_clear_flag(btns,LV_OBJ_FLAG_HIDDEN);
	return 0;
}

static int editor_draw(struct gui_activity*act){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	lv_obj_set_style_pad_all(act->page,0,0);
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	text=lv_textarea_create(act->page);
	lv_textarea_clear_selection(text);
	lv_textarea_set_text(text,"");
	lv_textarea_set_one_line(text,false);
	lv_textarea_set_text_selection(text,true);
	lv_obj_add_event_cb(text,insert_cb,LV_EVENT_INSERT,NULL);
	lv_obj_add_event_cb(text,text_changed_cb,LV_EVENT_VALUE_CHANGED,NULL);
	lv_obj_add_event_cb(text,click_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_width(text,lv_pct(100));
	lv_obj_set_flex_grow(text,1);
	lv_obj_set_style_border_width(text,0,0);
	lv_obj_set_style_radius(text,0,0);
	lv_obj_set_small_text_font(text,0);

	btns=lv_obj_create(act->page);
	lv_obj_set_style_radius(btns,0,0);
	lv_obj_set_scroll_dir(btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(btns,0,0);
	lv_obj_set_style_bg_opa(btns,LV_OPA_0,0);
	lv_obj_clear_flag(btns,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(btns,lv_pct(100));
	lv_obj_set_content_height(btns,gui_font_size*2);
	lv_obj_set_grid_dsc_array(btns,grid_col,grid_row);
	for(size_t i=0;i<ARRLEN(te_btns);i++){
		te_btns[i].btn=lv_btn_create(btns);
		lv_obj_set_enabled(te_btns[i].btn,te_btns[i].enabled);
		lv_obj_add_event_cb(te_btns[i].btn,btns_cb,LV_EVENT_CLICKED,te_btns[i].name);
		te_btns[i].lbl=lv_label_create(te_btns[i].btn);
		lv_label_set_text(te_btns[i].lbl,te_btns[i].symbol);
		lv_obj_center(te_btns[i].lbl);
		lv_obj_set_grid_cell(
			te_btns[i].btn,
			LV_GRID_ALIGN_STRETCH,i,1,
			LV_GRID_ALIGN_STRETCH,0,1
		);
	}

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
	.resize=editor_resize,
	.draw=editor_draw,
	.lost_focus=editor_lost_focus,
	.get_focus=editor_get_focus,
	.back=true
};
#endif

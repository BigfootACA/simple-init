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
#include"filesystem.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/sysbar.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/clipboard.h"
#include"gui/filepicker.h"
#define TAG "editor"

struct text_editor{
	char*cur_path,*last_search;
	bool changed;
	lv_obj_t*text,*btns;
	lv_obj_t*btn_open;
	lv_obj_t*btn_new;
	lv_obj_t*btn_save;
	lv_obj_t*btn_copy;
	lv_obj_t*btn_paste;
	lv_obj_t*btn_menu;
};
struct open_data{
	struct text_editor*te;
	char*path;
	size_t size;
	fsh*file;
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

static int editor_get_focus(struct gui_activity*act){
	struct text_editor*te=act->data;
	if(!te)return 0;
	lv_group_add_obj(gui_grp,te->text);
	lv_group_add_obj(gui_grp,te->btn_open);
	lv_group_add_obj(gui_grp,te->btn_new);
	lv_group_add_obj(gui_grp,te->btn_save);
	lv_group_add_obj(gui_grp,te->btn_copy);
	lv_group_add_obj(gui_grp,te->btn_paste);
	lv_group_add_obj(gui_grp,te->btn_menu);
	lv_obj_set_enabled(te->btn_paste,clipboard_get_type()==CLIP_TEXT);
	sysbar_focus_input(te->text);
	ctrl_pad_set_target(te->text);
	return 0;
}

static int editor_lost_focus(struct gui_activity*act){
	struct text_editor*te=act->data;
	if(!te)return 0;
	lv_group_remove_obj(te->text);
	lv_group_remove_obj(te->btn_open);
	lv_group_remove_obj(te->btn_new);
	lv_group_remove_obj(te->btn_save);
	lv_group_remove_obj(te->btn_copy);
	lv_group_remove_obj(te->btn_paste);
	lv_group_remove_obj(te->btn_menu);
	sysbar_focus_input(NULL);
	ctrl_pad_set_target(NULL);
	return 0;
}

static void set_changed(struct text_editor*te,bool v){
	te->changed=v;
	lv_obj_set_enabled(te->btn_save,v);
}

static void do_copy(struct text_editor*te){
	const char*cont=lv_textarea_get_text(te->text);
	lv_textarea_t*ext=(lv_textarea_t*)te->text;
	uint32_t i=ext->sel_end-ext->sel_start;
	if(i==0)return;
	clipboard_set(CLIP_TEXT,cont+ext->sel_start,i);
	lv_obj_set_enabled(te->btn_paste,true);
}

static void do_cut(struct text_editor*te){
	do_copy(te);
	lv_textarea_t*ext=(lv_textarea_t*)te->text;
	uint32_t ss=ext->sel_start,sl=ext->sel_end-ss;
	lv_textarea_remove_text(te->text,ss,sl);
	lv_obj_set_enabled(te->btn_copy,false);
	lv_textarea_set_cursor_pos(te->text,ss);
}

static void do_paste(struct text_editor*te){
	if(clipboard_get_type()!=CLIP_TEXT)return;
	lv_textarea_t*ext=(lv_textarea_t*)te->text;
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	if(sl>0){
		lv_textarea_remove_text(te->text,ss,sl);
		lv_obj_set_enabled(te->btn_copy,false);
		lv_textarea_set_cursor_pos(te->text,ss);
	}
	lv_textarea_add_text(te->text,clipboard_get_content());
}

static bool open_read_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data){
	struct open_data*od=user_data;
	char*buf=NULL;
	size_t rs=0;
	int res=0;
	if(id==0){
		if(od->te->cur_path)free(od->te->cur_path);
		od->te->cur_path=od->path;
		set_changed(od->te,false);
		lv_textarea_clear_selection(od->te->text);
		lv_textarea_set_text(od->te->text,"");
		lv_textarea_t*ext=(lv_textarea_t*)od->te->text;
		ext->sel_start=ext->sel_end=0;
		lv_obj_set_enabled(od->te->btn_copy,false);
		res=fs_read_all(od->file,(void**)&buf,&rs);
		if(res!=0)goto e_read;
		lv_textarea_set_text(od->te->text,buf);
		lv_textarea_set_cursor_pos(od->te->text,0);
		free(buf);
		set_changed(od->te,false);
		tlog_debug("read %zu bytes",rs);
	}
	end:
	fs_close(&od->file);
	free(od);
	return false;
	fail:
	free(od->path);
	od->te->cur_path=NULL;
	goto end;
	e_read:
	msgbox_alert("Read file failed: %s",strerror(res));
	goto fail;
}

static void open_start_read(void*d){
	open_read_cb(0,NULL,d);
}

static bool read_file(struct text_editor*te,const char*path){
	bool open=false;
	lv_res_t res;
	struct open_data*od=malloc(sizeof(struct open_data));
	memset(od,0,sizeof(struct open_data));
	if(!(od->path=strdup(path)))EDONE();
	od->te=te;
	if((res=fs_open(NULL,&od->file,od->path,FILE_FLAG_READ))!=LV_FS_RES_OK)
		EDONE(msgbox_alert("Open file failed: %s",strerror(res)));
	if((res=fs_get_size(od->file,&od->size))!=LV_FS_RES_OK)
		EDONE(msgbox_alert("Get file size failed: %s",strerror(res)));
	if(od->size>0x800000)EDONE(msgbox_alert("File too large; size limit is 8MiB, file is %zu bytes",od->size));
	if(od->size<0x10000)lv_async_call(open_start_read,od);
	else msgbox_set_user_data(msgbox_create_yesno(
		open_read_cb,
		"This file is bigger than 64 KiB (%zu bytes), "
		"are you sure you want to open this file?",od->size
	),od);
	return true;
	done:
	if(od){
		if(open)fs_close(&od->file);
		if(od->path)free(od->path);
		free(od);
	}
	return false;
}

static bool open_select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	return !read_file(user_data,path[0]);
}

static bool open_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data){
	if(id==0){
		struct filepicker*fp=filepicker_create(open_select_cb,"Open file");
		filepicker_set_user_data(fp,user_data);
		filepicker_set_max_item(fp,1);
	}
	return false;
}

static void do_open(struct text_editor*te){
	if(!te->changed)open_cb(0,NULL,te);
	else msgbox_set_user_data(msgbox_create_yesno(
		open_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	),te);
}

static bool new_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data){
	struct text_editor*te=user_data;
	if(id==0){
		if(te->cur_path)free(te->cur_path);
		te->cur_path=NULL;
		lv_textarea_set_text(te->text,"");
		lv_textarea_clear_selection(te->text);
		lv_textarea_t*ext=(lv_textarea_t*)te->text;
		ext->sel_start=ext->sel_end=0;
		lv_obj_set_enabled(te->btn_copy,false);
		set_changed(te,false);
	}
	return false;
}

static void do_new(struct text_editor*te){
	if(!te->changed)new_cb(0,NULL,te);
	else msgbox_set_user_data(msgbox_create_yesno(
		new_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	),te);
}

static void write_save(struct text_editor*te){
	int res;
	fsh*file=NULL;
	const char*cont=lv_textarea_get_text(te->text);
	size_t s=(uint32_t)strlen(cont),bs=0;
	if((res=fs_open(NULL,&file,te->cur_path,FILE_FLAG_WRITE))!=0){
		msgbox_alert("Open file failed: %s",strerror(res));
		return;
	}
	res=fs_write(file,(void*)cont,s,&bs);
	fs_close(&file);
	if(res!=0){
		msgbox_alert("Write file failed: %s",strerror(res));
		return;
	}
	tlog_debug("wrote %zu of %zu bytes",bs,s);
	if(bs!=s){
		msgbox_alert("Write file maybe failed, wrote %zu of %zu bytes",bs,s);
		return;
	}
	set_changed(te,false);
}

static bool save_select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	struct text_editor*te=user_data;
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	if(te->cur_path)free(te->cur_path);
	if(!(te->cur_path=strdup(path[0])))return true;
	write_save(te);
	return false;
}

static void do_save_as(struct text_editor*te){
	struct filepicker*fp=filepicker_create(save_select_cb,"Save file");
	filepicker_set_user_data(fp,te);
	filepicker_set_max_item(fp,1);
}

static void do_save(struct text_editor*te){
	if(!te->cur_path)do_save_as(te);
	else write_save(te);
}

static bool search_rev_cb(uint16_t id,const char*name __attribute__((unused)),void*user_data){
	struct text_editor*te=user_data;
	if(id==0){
		const char*cont=lv_textarea_get_text(te->text);
		char*ret=strstr(cont,te->last_search);
		if(ret)lv_textarea_set_cursor_pos(te->text,ret-cont);
		else msgbox_alert("No matching string found");
	}
	return false;
}

static bool search_cb(bool ok,const char*content,void*user_data __attribute__((unused))){
	struct text_editor*te=user_data;
	if(!ok)return false;
	if(te->last_search)free(te->last_search);
	if(!(te->last_search=strdup(content)))return true;
	uint32_t pos=lv_textarea_get_cursor_pos(te->text);
	const char*cont=lv_textarea_get_text(te->text);
	char*ret=strstr(cont+pos+1,te->last_search);
	if(ret){
		uint32_t match=ret-cont;
		tlog_debug("found match string at %d",match);
		lv_textarea_set_cursor_pos(te->text,match);
	}else msgbox_set_user_data(msgbox_create_yesno(
		search_rev_cb,
		"Searched to the end of the file, do you want "
		"to go back to the head and continue searching?"
	),te);
	return false;
}

static void do_search(struct text_editor*te){
	if(!*(lv_textarea_get_text(te->text)))return;
	struct inputbox*in=inputbox_create(search_cb,"Search text");
	if(te->last_search)inputbox_set_content(in,"%s",te->last_search);
	inputbox_set_user_data(in,te);
}

static bool menu_cb(uint16_t id,const char*name __attribute__((unused)),void*data){
	switch(id){
		case 0:do_copy(data);break;
		case 1:do_cut(data);break;
		case 2:do_paste(data);break;
		case 3:do_open(data);break;
		case 4:do_new(data);break;
		case 5:do_save(data);break;
		case 6:do_save_as(data);break;
		case 7:do_search(data);break;
	}
	return false;
}

static void btns_cb(lv_event_t*e){
	if(strcmp(guiact_get_last()->name,"text-editor")!=0)return;
	struct text_editor*te=e->user_data;
	if(e->target==te->btn_open)do_open(te);
	else if(e->target==te->btn_new)do_new(te);
	else if(e->target==te->btn_save)do_save(te);
	else if(e->target==te->btn_copy)do_copy(te);
	else if(e->target==te->btn_paste)do_paste(te);
	else if(e->target==te->btn_menu)msgbox_set_user_data(
		msgbox_create_custom(menu_cb,adv_menu,"Menu"),te
	);
}

static void text_changed_cb(lv_event_t*e){
	struct text_editor*te=e->user_data;
	lv_textarea_t*ext=(lv_textarea_t*)te->text;
	lv_obj_set_enabled(te->btn_copy,ext->sel_end-ext->sel_start>0);
	set_changed(te,true);
}

static void click_input_cb(lv_event_t*e){
	sysbar_focus_input(e->target);
}

static void insert_cb(lv_event_t*e){
	struct text_editor*te=e->user_data;
	lv_textarea_t*ext=(lv_textarea_t*)te->text;
	uint32_t ss=ext->sel_start,se=ext->sel_end,sl=se-ss;
	if(*(char*)e->param==127&&sl>0){
		lv_textarea_set_insert_replace(te->text,"");
		lv_textarea_remove_text(te->text,ss,sl);
		lv_obj_set_enabled(te->btn_copy,false);
		lv_textarea_set_cursor_pos(te->text,ss);
	}
}

static bool back_cb(uint16_t id,const char*name __attribute__((unused)),void*data){
	struct text_editor*te=data;
	if(id==0){
		te->changed=false;
		guiact_do_back();
		guiact_do_back();
		return true;
	}
	return false;
}

static int do_back(struct gui_activity*act){
	struct text_editor*te=act->data;
	if(!te||!te->changed)return 0;
	msgbox_set_user_data(msgbox_create_yesno(
		back_cb,
		"The buffer has changed, "
		"do you want to discard any changes?"
	),te);
	return 1;
}

static int do_clean(struct gui_activity*act){
	struct text_editor*te=act->data;
	if(!te)return 0;
	if(te->cur_path)free(te->cur_path);
	te->cur_path=NULL,te->changed=false;
	free(te);
	act->data=NULL;
	return 0;
}

static int do_init(struct gui_activity*act){
	struct text_editor*te;
	if(!(te=malloc(sizeof(struct text_editor))))return -1;
	memset(te,0,sizeof(struct text_editor));
	act->data=te;
	return 0;
}

static int editor_resize(struct gui_activity*act){
	struct text_editor*te=act->data;
	lv_obj_set_hidden(te->btns,sysbar.keyboard!=NULL);
	return 0;
}

static int editor_draw(struct gui_activity*act){
	struct text_editor*te=act->data;
	lv_obj_set_style_pad_all(act->page,0,0);
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	te->text=lv_textarea_create(act->page);
	lv_textarea_clear_selection(te->text);
	lv_textarea_set_text(te->text,"");
	lv_textarea_set_one_line(te->text,false);
	lv_textarea_set_text_selection(te->text,true);
	lv_obj_add_event_cb(te->text,insert_cb,LV_EVENT_INSERT,te);
	lv_obj_add_event_cb(te->text,text_changed_cb,LV_EVENT_VALUE_CHANGED,te);
	lv_obj_add_event_cb(te->text,click_input_cb,LV_EVENT_CLICKED,te);
	lv_obj_set_width(te->text,lv_pct(100));
	lv_obj_set_flex_grow(te->text,1);
	lv_obj_set_style_border_width(te->text,0,0);
	lv_obj_set_style_radius(te->text,0,0);
	lv_obj_set_small_text_font(te->text,0);
	te->btns=lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,en,x)&(struct button_dsc){\
			&tgt,en,title,btns_cb,te,x,1,0,1,NULL\
		}
		BTN(te->btn_open,  LV_SYMBOL_UPLOAD, true,  0),
		BTN(te->btn_new,   LV_SYMBOL_PLUS,   true,  1),
		BTN(te->btn_save,  LV_SYMBOL_SAVE,   false, 2),
		BTN(te->btn_copy,  LV_SYMBOL_COPY,   false, 3),
		BTN(te->btn_paste, LV_SYMBOL_PASTE,  false, 4),
		BTN(te->btn_menu,  LV_SYMBOL_LIST,   true,  5),
		#undef BTN
		NULL
	);
	set_changed(te,false);
	if(act->args){
		char*path=(char*)act->args;
		if(!*path)return -1;
		read_file(te,path);
	}
	return 0;
}

struct gui_register guireg_text_edit={
	.name="text-editor",
	.title="Text Editor",
	.show_app=true,
	.open_file=true,
	.ask_exit=do_back,
	.init=do_init,
	.quiet_exit=do_clean,
	.resize=editor_resize,
	.draw=editor_draw,
	.lost_focus=editor_lost_focus,
	.get_focus=editor_get_focus,
	.back=true
};
#endif

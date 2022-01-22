/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<ctype.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/stat.h>
#include<blkid/blkid.h>
#include<libmount/libmount.h>
#include"str.h"
#include"gui.h"
#include"logger.h"
#include"system.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "mount"

struct add_mount{
	lv_obj_t*box,*ok,*cancel;
	lv_obj_t*btn_source,*btn_target;
	lv_obj_t*source,*target,*type,*options;
	bool type_lock;
	bool target_def;
	list*types;
};

static int add_mount_get_focus(struct gui_activity*d){
	struct add_mount*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->source);
	lv_group_add_obj(gui_grp,am->btn_source);
	lv_group_add_obj(gui_grp,am->target);
	lv_group_add_obj(gui_grp,am->btn_target);
	lv_group_add_obj(gui_grp,am->type);
	lv_group_add_obj(gui_grp,am->options);
	lv_group_add_obj(gui_grp,am->ok);
	lv_group_add_obj(gui_grp,am->cancel);
	return 0;
}

static int add_mount_lost_focus(struct gui_activity*d){
	struct add_mount*am=d->data;
	if(!am)return 0;
	lv_group_remove_obj(am->source);
	lv_group_remove_obj(am->btn_source);
	lv_group_remove_obj(am->target);
	lv_group_remove_obj(am->btn_target);
	lv_group_remove_obj(am->type);
	lv_group_remove_obj(am->options);
	lv_group_remove_obj(am->ok);
	lv_group_remove_obj(am->cancel);
	return 0;
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	lv_textarea_set_text(user_data,path[0]+2);
	return false;
}

static void sel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct filepicker*fp=filepicker_create(select_cb,"Select item");
	filepicker_set_user_data(fp,lv_obj_get_user_data(obj));
	filepicker_set_max_item(fp,1);
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct add_mount*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->ok)return;
	int r=0;
	struct libmnt_context*cxt;
	char buf[BUFSIZ]={0},type[64]={0};
	const char*source=lv_textarea_get_text(am->source);
	const char*target=lv_textarea_get_text(am->target);
	const char*options=lv_textarea_get_text(am->options);
	if(lv_dropdown_get_selected(am->type)!=0)lv_dropdown_get_selected_str(am->type,type,63);
	if(source&&!source[0])source=NULL;
	if(target&&!target[0])target=NULL;
	if(options&&!options[0])options=NULL;
	if(!source){
		msgbox_alert("Invalid mount source");
		return;
	}
	if(!target||target[0]!='/'){
		msgbox_alert("Invalid mount target");
		return;
	}
	mkdir(target,0755);
	if(!(cxt=mnt_new_context())){
		telog_error("failed to init libmount context");
		msgbox_alert("Init libmount context failed");
		goto fail;
	}
	errno=0;
	if(options&&mnt_context_set_options(cxt,options)!=0){
		telog_error("failed to set mount options");
		msgbox_alert("Failed to set mount options");
		goto fail;
	}
	errno=0;
	if(type[0]&&mnt_context_set_fstype(cxt,type)!=0){
		telog_error("failed to set mount filesystem type");
		msgbox_alert("Failed to set mount filesystem type");
		goto fail;
	}
	errno=0;
	if(mnt_context_set_source(cxt,source)!=0){
		telog_error("failed to set mount source");
		msgbox_alert("Failed to set mount source");
		goto fail;
	}
	errno=0;
	if(mnt_context_set_target(cxt,target)!=0){
		telog_error("failed to set mount target");
		msgbox_alert("Failed to set mount target");
		goto fail;
	}
	tlog_debug(
		"mount %s(%s) to %s with %s",
		source,
		type[0]?type:"none",
		target,
		options?options:"(null)"
	);
	int ec=mnt_context_mount(cxt);
	if((r=mnt_context_get_excode(cxt,ec,buf,sizeof(buf)))!=0){
		telog_error("mount failed: %s",buf);
		msgbox_alert("Mount failed: %s",buf);
		goto fail;
	}
	tlog_debug("mount success");
	guiact_do_back();
	fail:
	if(cxt)mnt_free_context(cxt);
	if(r!=0&&am->target_def)rmdir(target);
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct add_mount*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->cancel)return;
	guiact_do_back();
}

static int init(struct gui_activity*act){
	struct add_mount*am=malloc(sizeof(struct add_mount));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct add_mount));
	act->data=am;
	char*p=act->args;
	if(p&&p[0]!='/'){
		if(p[1]!=':')return -EINVAL;
		act->args+=2;
	}
	return 0;
}

static void inp_cb(lv_obj_t*obj __attribute__((unused)),lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	sysbar_focus_input(obj);
	sysbar_keyboard_open();
}

static void reload_type(struct add_mount*am){
	uint16_t i=0;
	size_t s=0,o=0;
	char buf[BUFSIZ],name[64]={0},c;
	uint16_t cur=lv_dropdown_get_selected(am->type);
	lv_dropdown_clear_options(am->type);
	lv_dropdown_add_option(am->type,_("(none)"),i++);
	list_free_all_def(am->types);
	am->types=NULL;
	if(read_file(buf,BUFSIZ,true,_PATH_PROC_FILESYSTEMS)<=0)return;
	for(;(c=buf[s]);s++)if(isspace(c)){
		if(name[0]&&strcmp(name,"nodev")!=0){
			lv_dropdown_add_option(am->type,name,i);
			if(i==cur)lv_dropdown_set_selected(am->type,i);
			list_obj_add_new_strdup(&am->types,name);
			i++;
		}
		memset(name,0,sizeof(name));
		o=0;
	}else if(o<64)name[o++]=c;
}

static char*get_fs_type(const char*blk){
	static blkid_cache cache=NULL;
	if(!cache)blkid_get_cache(&cache,NULL);
	return blkid_get_tag_value(cache,"TYPE",blk);
}

static bool focus_type(struct add_mount*am,char*type){
	uint16_t id=1;
	list*o=list_first(am->types);
	if(o)do{
		LIST_DATA_DECLARE(name,o,char*);
		if(strcmp(type,name)==0){
			lv_dropdown_set_selected(am->type,id);
			return true;
		}
		id++;
	}while((o=o->next));
	return false;
}

static void source_cb(lv_obj_t*obj,lv_event_t e){
	inp_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	struct add_mount*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->source||am->type_lock)return;
	char*type;
	const char*value=lv_textarea_get_text(am->source);
	if(!value||value[0]!='/'||!(type=get_fs_type(value)))return;
	if(!focus_type(am,type)){
		char mod[64]={0};
		snprintf(mod,63,"fs-%s",type);
		insmod(mod,false);
		reload_type(am);
		focus_type(am,type);
	}
}

static void target_cb(lv_obj_t*obj,lv_event_t e){
	inp_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	struct add_mount*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->target)return;
	am->target_def=false;
}

static void type_cb(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE)return;
	struct add_mount*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->type)return;
	lv_default_dropdown_cb(obj,e);
	if(e==LV_EVENT_DEFOCUSED)am->type_lock=true;
}

static int do_cleanup(struct gui_activity*act){
	struct add_mount*am=act->data;
	if(!am)return 0;
	if(am->target_def)rmdir(lv_textarea_get_text(am->target));
	list_free_all_def(am->types);
	free(am);
	act->data=NULL;
	return 0;
}

static int draw_add_mount(struct gui_activity*act){
	struct add_mount*am=act->data;

	am->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(am->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(am->box,gui_sw/8*7);
	lv_obj_set_click(am->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(am->box);

	// Title
	lv_obj_t*title=lv_label_create(am->box,NULL);
	lv_label_set_text(title,_("Add Mount"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	// Source
	h+=gui_font_size;
	lv_obj_t*source=lv_label_create(am->box,NULL);
	lv_label_set_text(source,_("Device:"));
	lv_obj_set_y(source,h);

	am->source=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_textarea_set_one_line(am->source,true);
	lv_textarea_set_cursor_hidden(am->source,true);
	lv_obj_set_user_data(am->source,am);
	lv_obj_set_event_cb(am->source,source_cb);
	lv_obj_align(am->source,source,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(am->source,h);
	lv_obj_align(source,am->source,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);

	am->btn_source=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->btn_source,true);
	lv_obj_set_user_data(am->btn_source,am->source);
	lv_obj_set_event_cb(am->btn_source,sel_cb);
	lv_obj_set_style_local_radius(am->btn_source,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size/2);
	lv_obj_set_size(am->btn_source,gui_font_size*3,lv_obj_get_height(am->source));
	lv_obj_set_width(am->source,w-lv_obj_get_width(am->btn_source)-lv_obj_get_width(source)-gui_font_size);
	lv_obj_align(am->btn_source,am->source,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/4,0);
	lv_label_set_text(lv_label_create(am->btn_source,NULL),"...");
	h+=lv_obj_get_height(am->btn_source);

	// Target
	h+=gui_font_size;
	lv_obj_t*target=lv_label_create(am->box,NULL);
	lv_label_set_text(target,_("Target:"));
	lv_obj_set_y(target,h);

	char point[256];
	auto_mountpoint(point,256);
	am->target_def=true;
	am->target=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->target,point);
	lv_textarea_set_one_line(am->target,true);
	lv_textarea_set_cursor_hidden(am->target,true);
	lv_obj_set_user_data(am->target,am);
	lv_obj_set_event_cb(am->target,target_cb);
	lv_obj_align(am->target,target,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(am->target,h);
	lv_obj_align(target,am->target,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);

	am->btn_target=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->btn_target,true);
	lv_obj_set_user_data(am->btn_target,am->target);
	lv_obj_set_event_cb(am->btn_target,sel_cb);
	lv_obj_set_style_local_radius(am->btn_target,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_font_size/2);
	lv_obj_set_size(am->btn_target,gui_font_size*3,lv_obj_get_height(am->target));
	lv_obj_set_width(am->target,w-lv_obj_get_width(am->btn_target)-lv_obj_get_width(target)-gui_font_size);
	lv_obj_align(am->btn_target,am->target,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/4,0);
	lv_label_set_text(lv_label_create(am->btn_target,NULL),"...");
	h+=lv_obj_get_height(am->btn_target);

	// Type
	h+=gui_font_size;
	lv_obj_t*type=lv_label_create(am->box,NULL);
	lv_label_set_text(type,_("Type:"));
	lv_obj_set_y(type,h);

	am->type=lv_dropdown_create(am->box,NULL);
	lv_obj_set_user_data(am->type,am);
	lv_obj_set_event_cb(am->type,type_cb);
	lv_obj_set_width(am->type,w-lv_obj_get_width(type)-gui_font_size);
	lv_obj_align(am->type,type,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(am->type,h);
	lv_obj_align(type,am->type,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);
	h+=lv_obj_get_height(am->type);

	// Options
	h+=gui_font_size;
	lv_obj_t*options=lv_label_create(am->box,NULL);
	lv_label_set_text(options,_("Options:"));
	lv_obj_set_y(options,h);

	am->options=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->options,"rw,noatime");
	lv_textarea_set_one_line(am->options,true);
	lv_textarea_set_cursor_hidden(am->options,true);
	lv_obj_set_user_data(am->options,am);
	lv_obj_set_event_cb(am->options,inp_cb);
	lv_obj_set_user_data(am->options,am);
	lv_obj_set_width(am->options,w-lv_obj_get_width(options)-gui_font_size);
	lv_obj_align(am->options,options,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(am->options,h);
	lv_obj_align(options,am->options,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);
	h+=lv_obj_get_height(am->options);

	// OK Button
	h+=gui_font_size;
	am->ok=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->ok,true);
	lv_obj_set_size(am->ok,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(am->ok,NULL,LV_ALIGN_IN_TOP_LEFT,(gui_font_size/2),h);
	lv_obj_set_user_data(am->ok,am);
	lv_obj_set_event_cb(am->ok,ok_cb);
	lv_label_set_text(lv_label_create(am->ok,NULL),_("OK"));

	// Cancel Button
	am->cancel=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->cancel,true);
	lv_obj_set_size(am->cancel,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(am->cancel,NULL,LV_ALIGN_IN_TOP_RIGHT,-(gui_font_size/2),h);
	lv_obj_set_user_data(am->cancel,am);
	lv_obj_set_event_cb(am->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(am->cancel,NULL),_("Cancel"));
	h+=lv_obj_get_height(am->cancel);

	h+=gui_font_size*3;
	lv_obj_set_height(am->box,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(am->box,NULL,LV_ALIGN_CENTER,0,0);

	reload_type(am);
	if(act->args)source_cb(am->source,LV_EVENT_DEFOCUSED);
	return 0;
}

struct gui_register guireg_add_mount={
	.name="add-mount",
	.title="Add Mount",
	.icon="mount.svg",
	.show_app=false,
	.open_file=true,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=add_mount_get_focus,
	.lost_focus=add_mount_lost_focus,
	.draw=draw_add_mount,
	.back=true,
	.mask=true,
};
#endif

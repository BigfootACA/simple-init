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
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "mount"

struct add_mount{
	lv_obj_t*box,*fields,*btns,*ok,*cancel;
	lv_obj_t*btn_source,*btn_target;
	lv_obj_t*lbl_title,*lbl_source;
	lv_obj_t*lbl_target,*lbl_type,*lbl_options;
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

static void sel_cb(lv_event_t*e){
	struct filepicker*fp=filepicker_create(select_cb,"Select item");
	filepicker_set_user_data(fp,e->user_data);
	filepicker_set_max_item(fp,1);
}

static void ok_cb(lv_event_t*e){
	int r=0;
	struct libmnt_context*cxt;
	char buf[BUFSIZ]={0},type[64]={0};
	struct add_mount*am=e->user_data;
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

static void cancel_cb(lv_event_t*e __attribute__((unused))){
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

static void source_cb(lv_event_t*e){
	struct add_mount*am=e->user_data;
	if(am->type_lock)return;
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

static void target_cb(lv_event_t*e){
	struct add_mount*am=e->user_data;
	am->target_def=false;
}

static void type_cb(lv_event_t*e){
	struct add_mount*am=e->user_data;
	am->target_def=false;
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
	static lv_coord_t grid_col[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	char point[256];
	struct add_mount*am=act->data;

	am->box=lv_obj_create(act->page);
	lv_obj_set_style_max_width(am->box,lv_pct(80),0);
	lv_obj_set_style_max_height(am->box,lv_pct(80),0);
	lv_obj_set_style_min_width(am->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(am->box,gui_dpi,0);
	lv_obj_set_height(am->box,LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(am->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_center(am->box);

	// Title
	am->lbl_title=lv_label_create(am->box);
	lv_obj_set_width(am->lbl_title,lv_pct(100));
	lv_label_set_text(am->lbl_title,_("Add Mount"));
	lv_label_set_long_mode(am->lbl_title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(am->lbl_title,LV_TEXT_ALIGN_CENTER,0);

	am->fields=lv_obj_create(am->box);
	lv_obj_set_style_radius(am->fields,0,0);
	lv_obj_set_scroll_dir(am->fields,LV_DIR_NONE);
	lv_obj_set_style_border_width(am->fields,0,0);
	lv_obj_set_style_bg_opa(am->fields,LV_OPA_0,0);
	lv_obj_set_style_pad_all(am->fields,gui_dpi/50,0);
	lv_obj_set_grid_dsc_array(am->fields,grid_col,grid_row);
	lv_obj_clear_flag(am->fields,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_row(am->fields,gui_font_size/2,0);
	lv_obj_set_style_pad_column(am->fields,gui_font_size/2,0);
	lv_obj_set_size(am->fields,lv_pct(100),LV_SIZE_CONTENT);

	// Source
	am->lbl_source=lv_label_create(am->fields);
	lv_label_set_text(am->lbl_source,_("Device:"));
	lv_obj_set_grid_cell(am->lbl_source,LV_GRID_ALIGN_START,0,3,LV_GRID_ALIGN_CENTER,0,1);

	am->source=lv_textarea_create(am->fields);
	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_textarea_set_one_line(am->source,true);
	lv_obj_add_event_cb(am->source,source_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_set_grid_cell(am->source,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_CENTER,1,1);

	am->btn_source=lv_btn_create(am->fields);
	lv_obj_set_enabled(am->btn_source,true);
	lv_obj_add_event_cb(am->btn_source,sel_cb,LV_EVENT_CLICKED,am->source);
	lv_label_set_text(lv_label_create(am->btn_source),"...");
	lv_obj_set_grid_cell(am->btn_source,LV_GRID_ALIGN_STRETCH,2,1,LV_GRID_ALIGN_CENTER,1,1);

	// Target
	am->lbl_target=lv_label_create(am->fields);
	lv_label_set_text(am->lbl_target,_("Target:"));
	lv_obj_set_grid_cell(am->lbl_target,LV_GRID_ALIGN_START,0,3,LV_GRID_ALIGN_CENTER,2,1);

	auto_mountpoint(point,256);
	am->target_def=true;
	am->target=lv_textarea_create(am->fields);
	lv_textarea_set_text(am->target,point);
	lv_textarea_set_one_line(am->target,true);
	lv_obj_add_event_cb(am->target,target_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_set_grid_cell(am->target,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_CENTER,3,1);

	am->btn_target=lv_btn_create(am->fields);
	lv_obj_set_enabled(am->btn_target,true);
	lv_obj_add_event_cb(am->btn_target,sel_cb,LV_EVENT_CLICKED,am->target);
	lv_label_set_text(lv_label_create(am->btn_target),"...");
	lv_obj_set_grid_cell(am->btn_target,LV_GRID_ALIGN_STRETCH,2,1,LV_GRID_ALIGN_CENTER,3,1);

	// Type
	am->lbl_type=lv_label_create(am->fields);
	lv_label_set_text(am->lbl_type,_("Type:"));
	lv_obj_set_grid_cell(am->lbl_type,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,4,1);

	am->type=lv_dropdown_create(am->fields);
	lv_obj_add_event_cb(am->type,lv_default_dropdown_cb,LV_EVENT_ALL,am);
	lv_obj_add_event_cb(am->type,type_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_set_grid_cell(am->type,LV_GRID_ALIGN_STRETCH,1,2,LV_GRID_ALIGN_CENTER,4,1);

	// Options
	am->lbl_options=lv_label_create(am->fields);
	lv_label_set_text(am->lbl_options,_("Options:"));
	lv_obj_set_grid_cell(am->lbl_options,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,5,1);

	am->options=lv_textarea_create(am->fields);
	lv_textarea_set_text(am->options,"rw,noatime");
	lv_textarea_set_one_line(am->options,true);
	lv_obj_add_event_cb(am->options,lv_input_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_set_grid_cell(am->options,LV_GRID_ALIGN_STRETCH,1,2,LV_GRID_ALIGN_CENTER,5,1);

	am->btns=lv_obj_create(am->box);
	lv_obj_set_style_radius(am->btns,0,0);
	lv_obj_set_scroll_dir(am->btns,LV_DIR_NONE);
	lv_obj_set_style_border_width(am->btns,0,0);
	lv_obj_set_style_bg_opa(am->btns,LV_OPA_0,0);
	lv_obj_set_style_pad_all(am->btns,gui_dpi/50,0);
	lv_obj_set_flex_flow(am->btns,LV_FLEX_FLOW_ROW);
	lv_obj_clear_flag(am->btns,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_row(am->btns,gui_font_size/2,0);
	lv_obj_set_style_pad_column(am->btns,gui_font_size/2,0);
	lv_obj_set_size(am->btns,lv_pct(100),LV_SIZE_CONTENT);

	// OK Button
	am->ok=lv_btn_create(am->btns);
	lv_obj_add_event_cb(am->ok,ok_cb,LV_EVENT_CLICKED,am);
	lv_obj_t*lbl_ok=lv_label_create(am->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);
	lv_obj_set_flex_grow(am->ok,1);

	// Cancel Button
	am->cancel=lv_btn_create(am->btns);
	lv_obj_add_event_cb(am->cancel,cancel_cb,LV_EVENT_CLICKED,NULL);
	lv_obj_t*lbl_cancel=lv_label_create(am->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);
	lv_obj_set_flex_grow(am->cancel,1);

	reload_type(am);
	if(act->args)source_cb(&(lv_event_t){.user_data=am});
	return 0;
}

struct gui_register guireg_add_mount={
	.name="add-mount",
	.title="Add Mount",
	.icon="mount.svg",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"^[A-Z]?:/dev/.+",
		".*\\.img$",
		".*\\.raw$",
		".*\\.iso$",
		NULL
	},
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=add_mount_get_focus,
	.lost_focus=add_mount_lost_focus,
	.draw=draw_add_mount,
	.back=true,
	.mask=true,
};
#endif

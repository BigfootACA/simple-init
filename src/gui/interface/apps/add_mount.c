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

static void ok_cb(lv_event_t*e){
	int r=0;
	struct libmnt_context*cxt;
	char buf[BUFSIZ]={0},type[64]={0},*x;
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
	if((x=strstr(source,"://"))){
		if(strncmp(source,"file",x-source)!=0){
			msgbox_alert("Only file:// path supported");
			return;
		}
		source=x+3;
	}
	if(target){
		if((x=strstr(target,"://"))){
			if(strncmp(target,"file",x-target)!=0){
				msgbox_alert("Only file:// path supported");
				return;
			}
			target=x+3;
		}
		if(target[0]!='/'){
			msgbox_alert("Invalid mount target");
			return;
		}
		mkdir(target,0755);
	}
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

static int init(struct gui_activity*act){
	struct add_mount*am=malloc(sizeof(struct add_mount));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct add_mount));
	act->data=am;
	char*p=act->args,*x;
	if(p&&(x=strstr(p,"://"))){
		if(strncmp(p,"file",x-p)!=0){
			msgbox_alert("Only file:// path supported");
			return -1;
		}
		act->args=x+3;
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
	free(type);
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
	char point[256];
	struct add_mount*am=act->data;
	am->box=lv_draw_dialog_box(act->page,NULL,"Add Mount");
	lv_draw_input(am->box,"Device:",NULL,NULL,&am->source,&am->btn_source);
	lv_draw_input(am->box,"Target:",NULL,NULL,&am->target,&am->btn_target);
	lv_draw_dropdown(am->box,"Type:",&am->type);
	lv_draw_input(am->box,"Options:",NULL,NULL,&am->options,NULL);
	lv_draw_btns_ok_cancel(am->box,&am->ok,&am->cancel,ok_cb,am);
	am->target_def=true;
	auto_mountpoint(point,256);
	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_textarea_set_text(am->target,point);
	lv_textarea_set_text(am->options,"rw,noatime");
	lv_obj_add_event_cb(am->source,source_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_add_event_cb(am->target,target_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_add_event_cb(am->type,type_cb,LV_EVENT_DEFOCUSED,am);
	reload_type(am);
	if(act->args)source_cb(&(lv_event_t){.user_data=am});
	return 0;
}

struct gui_register guireg_add_mount={
	.name="add-mount",
	.title="Add Mount",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"file:///dev/.+",
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

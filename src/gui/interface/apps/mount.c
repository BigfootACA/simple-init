/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include<sys/mount.h>
#include<blkid/blkid.h>
#include"str.h"
#include"gui.h"
#include"logger.h"
#include"system.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "mounts"

static blkid_cache cache=NULL;
static lv_obj_t*lst=NULL,*show_all;
static lv_obj_t*info,*btn_mount,*btn_umount,*btn_refresh;
struct mount_info{
	bool enable;
	lv_obj_t*btn;
	lv_obj_t*source,*type;
	lv_obj_t*target,*partlabel;
	struct mount_item*item;
};
static bool is_show_all=false;
static list*mounts=NULL;
static struct mount_info*selected=NULL;

static void item_check(lv_event_t*e){
	lv_obj_set_checked(e->target,true);
	if(selected&&e->target!=selected->btn)
		lv_obj_set_checked(selected->btn,false);
	selected=e->user_data;
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_umount,true);
}

static void free_mount_info(struct mount_info*m){
	if(!m)return;
	if(m->item)free_mount_item(m->item);
	memset(m,0,sizeof(struct mount_info));
	free(m);
}

static int item_free(void*d){
	struct mount_info*mi=d;
	free_mount_info(mi);
	return 0;
}

static int item_free_del(void*d){
	struct mount_info*mi=d;
	lv_obj_del(mi->btn);
	free_mount_info(mi);
	return 0;
}

static void mount_clear(){
	selected=NULL;
	lv_obj_set_enabled(btn_umount,false);
	if(info)lv_obj_del(info);
	list_free_all(mounts,item_free_del);
	info=NULL,mounts=NULL;
}

static void mount_set_info(char*text){
	mount_clear();
	info=lv_label_create(lst);
	lv_label_set_long_mode(info,LV_LABEL_LONG_WRAP);
	lv_obj_set_size(info,lv_obj_get_width(lst),gui_sh/16);
	lv_label_set_text(info,text);
}

static void mount_add_item(struct mount_info*k){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};

	// mount button
	k->btn=lv_btn_create(lst);
	lv_obj_set_size(k->btn,lv_pct(100),gui_dpi/2);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_grid_dsc_array(k->btn,grid_col,grid_row);
	lv_obj_add_event_cb(k->btn,item_check,LV_EVENT_CLICKED,k);
	lv_group_add_obj(gui_grp,k->btn);

	// source
	k->source=lv_label_create(k->btn);
	lv_label_set_long_mode(k->source,LV_LABEL_LONG_DOT);
	lv_label_set_text(k->source,k->item->source);
	lv_obj_set_grid_cell(k->source,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,0,1);

	// type
	k->type=lv_label_create(k->btn);
	lv_label_set_text(k->type,k->item->type);
	lv_obj_set_small_text_font(k->type,LV_PART_MAIN);
	lv_obj_set_grid_cell(k->type,LV_GRID_ALIGN_END,1,1,LV_GRID_ALIGN_CENTER,0,1);

	// target
	k->target=lv_label_create(k->btn);
	lv_label_set_long_mode(k->target,LV_LABEL_LONG_DOT);
	lv_obj_set_small_text_font(k->target,LV_PART_MAIN);
	lv_label_set_text(k->target,k->item->target);
	lv_obj_set_grid_cell(k->target,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);

	char*partlabel=NULL;
	if(!cache)blkid_get_cache(&cache,NULL);
	if(strncmp(
		k->item->source,
		_PATH_DEV,
		strlen(_PATH_DEV)
	)==0)partlabel=blkid_get_tag_value(
		cache,"PARTLABEL",
		k->item->source
	);
	if(partlabel){
		// part label
		k->partlabel=lv_label_create(k->btn);
		lv_obj_set_style_text_align(k->partlabel,LV_TEXT_ALIGN_RIGHT,0);
		lv_label_set_text(k->partlabel,partlabel);
		lv_obj_set_small_text_font(k->partlabel,LV_PART_MAIN);
		lv_obj_set_grid_cell(k->partlabel,LV_GRID_ALIGN_END,1,1,LV_GRID_ALIGN_CENTER,1,1);
		free(partlabel);
	}
}

static void mount_reload(){
	mount_clear();
	struct mount_item**ms=read_proc_mounts();
	if(!ms){
		mount_set_info(_("Read mount table failed"));
		return;
	}
	for(size_t i=0;ms[i];i++){
		if(strncmp(
			ms[i]->source,
			_PATH_DEV,
			strlen(_PATH_DEV)
		)!=0&&!is_show_all){
			free_mount_item(ms[i]);
			continue;
		}
		struct mount_info*m=malloc(sizeof(struct mount_info));
		if(!m){
			free_mount_item(ms[i]);
			continue;
		}
		memset(m,0,sizeof(struct mount_info));
		m->item=ms[i];
		mount_add_item(m);
		list_obj_add_new_notnull(&mounts,m);
	}
	if(!mounts)mount_set_info(_("No mount points found"));
	free(ms);
}

static void refresh_click(lv_event_t*e __attribute__((unused))){
	tlog_debug("request refresh");
	mount_reload();
}

static void mount_click(lv_event_t*e __attribute__((unused))){
	guiact_start_activity_by_name("add-mount",NULL);
}

static void show_all_click(lv_event_t*e){
	is_show_all=lv_obj_is_checked(e->target);
	tlog_debug("request show all %s",BOOL2STR(is_show_all));
	mount_reload();
}

static bool umount_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct mount_info*mi=user_data;
	if(!mi)return true;
	if(id==0){
		errno=0;
		umount(mi->item->target);
		if(errno!=0)msgbox_alert("Unmount failed: %m");
	}
	return false;
}

static void umount_click(lv_event_t*e __attribute__((unused))){
	if(!selected)return;
	msgbox_set_user_data(msgbox_create_yesno(
		umount_cb,
		"Are you sure you want to unmount '%s'?",
		selected->item->source
	),selected);
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	list_free_all(mounts,item_free);
	if(cache)blkid_put_cache(cache);
	info=NULL,mounts=NULL,cache=NULL;
	return 0;
}

static int do_load(struct gui_activity*d __attribute__((unused))){
	mount_reload();
	return 0;
}

static int mount_get_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(mounts);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct mount_info*);
		lv_group_add_obj(gui_grp,item->btn);
	}while((o=o->next));
	lv_group_add_obj(gui_grp,show_all);
	lv_group_add_obj(gui_grp,btn_mount);
	lv_group_add_obj(gui_grp,btn_umount);
	lv_group_add_obj(gui_grp,btn_refresh);
	return 0;
}

static int mount_lost_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(mounts);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct mount_info*);
		lv_group_remove_obj(item->btn);
	}while((o=o->next));
	lv_group_remove_obj(show_all);
	lv_group_remove_obj(btn_mount);
	lv_group_remove_obj(btn_umount);
	lv_group_remove_obj(btn_refresh);
	return 0;
}

static int mount_draw(struct gui_activity*act){
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// function title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("Mount Manager"));

	// options list
	lst=lv_obj_create(act->page);
	lv_obj_set_flex_flow(lst,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(lst,lv_pct(100));
	lv_obj_set_flex_grow(lst,1);

	// show all checkbox
	show_all=lv_draw_checkbox(
		act->page,_("Show all mountpoints"),
		false,show_all_click,NULL
	);

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,x)&(struct button_dsc){\
			&btn_##tgt,true,_(title),                   \
			tgt##_click,NULL,x,1,0,1,NULL\
		}
		BTN(mount,  "Mount",   0),
		BTN(umount, "Unmount", 1),
		BTN(refresh,"Refresh", 2),
		NULL
		#undef BTN
	);
	return 0;
}

struct gui_register guireg_mount={
	.name="mount-manager",
	.title="Mount Manager",
	.show_app=true,
	.draw=mount_draw,
	.quiet_exit=do_cleanup,
	.get_focus=mount_get_focus,
	.lost_focus=mount_lost_focus,
	.data_load=do_load,
	.back=true,
};
#endif

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
#ifdef ENABLE_BLKID
#include<blkid/blkid.h>
#endif
#include"str.h"
#include"gui.h"
#include"logger.h"
#include"system.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "mounts"

#ifdef ENABLE_BLKID
blkid_cache cache=NULL;
#endif
static lv_obj_t*lst=NULL,*last=NULL,*show_all;
static lv_obj_t*info,*btn_mount,*btn_umount,*btn_refresh;
struct mount_info{
	bool enable;
	lv_obj_t*btn,*chk;
	lv_obj_t*source,*type;
	lv_obj_t*target,*partlabel;
	struct mount_item*item;
};
static bool is_show_all=false;
static list*mounts=NULL;
static struct mount_info*selected=NULL;

static void item_check(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected&&obj!=selected->chk){
		lv_checkbox_set_checked(selected->chk,false);
		lv_obj_set_checked(selected->btn,false);
	}
	selected=lv_obj_get_user_data(obj);
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_umount,true);
}

static void free_mount_item(struct mount_item*m){
	if(!m)return;
	if(m->source)free(m->source);
	if(m->options){
		if(m->options[0])free(m->options[0]);
		free(m->options);
	}
	memset(m,0,sizeof(struct mount_item));
	free(m);
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
	info=NULL,last=NULL,mounts=NULL;
}

static void mount_set_info(char*text){
	mount_clear();
	info=lv_label_create(lst,NULL);
	lv_label_set_long_mode(info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(info,lv_page_get_scrl_width(lst),gui_sh/16);
	lv_label_set_align(info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(info,text);
}

static void mount_add_item(struct mount_info*k){
	lv_coord_t bw,bm;
	bw=lv_page_get_scrl_width(lst);
	bm=gui_dpi/20;

	// mount button
	k->btn=lv_btn_create(lst,NULL);
	lv_obj_set_size(k->btn,bw,gui_dpi/2);
	lv_obj_align(
		k->btn,last,
		last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
		0,last?gui_dpi/8:0
	);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);
	last=k->btn;

	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	// checkbox
	k->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(k->chk,"");
	lv_obj_set_event_cb(k->chk,item_check);
	lv_obj_set_user_data(k->chk,k);
	lv_style_set_focus_checkbox(k->chk);
	lv_obj_align(k->chk,k->btn,LV_ALIGN_IN_TOP_LEFT,bm,bm);
	lv_group_add_obj(gui_grp,k->chk);

	// source
	k->source=lv_label_create(line,NULL);
	lv_obj_align(k->source,k->chk,LV_ALIGN_OUT_RIGHT_MID,0,0);
	lv_label_set_long_mode(k->source,LV_LABEL_LONG_DOT);
	lv_label_set_text(k->source,k->item->source);

	// type
	k->type=lv_label_create(line,NULL);
	lv_label_set_text(k->type,k->item->type);
	lv_obj_set_small_text_font(k->type,LV_LABEL_PART_MAIN);
	if(lv_obj_get_width(k->type)>bw/3){
		lv_label_set_long_mode(k->type,LV_LABEL_LONG_DOT);
		lv_obj_set_width(k->type,bw/3);
	}
	lv_obj_align(k->type,k->btn,LV_ALIGN_IN_TOP_RIGHT,-bm,gui_dpi/15);
	lv_obj_set_width(k->source,lv_obj_get_x(k->type)-lv_obj_get_x(k->source)-bm);

	// target
	k->target=lv_label_create(line,NULL);
	lv_label_set_long_mode(k->target,LV_LABEL_LONG_DOT);
	lv_obj_set_small_text_font(k->target,LV_LABEL_PART_MAIN);
	lv_label_set_text(k->target,k->item->target);
	lv_obj_align(k->target,k->btn,LV_ALIGN_IN_BOTTOM_LEFT,bm,-bm);
	lv_obj_set_width(k->target,bw-(bm*2));

	#ifdef ENABLE_BLKID
	char*partlabel=NULL;
	if(!cache)blkid_get_cache(&cache,NULL);
	if(strncmp(k->item->source,"/dev/",5)==0)
		partlabel=blkid_get_tag_value(cache,"PARTLABEL",k->item->source);
	if(partlabel){
		// part label
		k->partlabel=lv_label_create(line,NULL);
		lv_label_set_align(k->partlabel,LV_LABEL_ALIGN_RIGHT);
		lv_label_set_text(k->partlabel,partlabel);
		lv_obj_set_small_text_font(k->partlabel,LV_LABEL_PART_MAIN);
		if(lv_obj_get_width(k->partlabel)>bw/4-(bm*3)){
			lv_label_set_long_mode(k->partlabel,LV_LABEL_LONG_DOT);
			lv_obj_set_width(k->partlabel,bw/4-(bm*3));
		}
		lv_obj_align(k->partlabel,k->btn,LV_ALIGN_IN_BOTTOM_RIGHT,-bm,-bm);
		lv_obj_set_width(k->target,bw-lv_obj_get_width(k->partlabel)-(bm*3));
		free(partlabel);
	}
	#endif
}

static void mount_reload(){
	mount_clear();
	struct mount_item**ms=read_proc_mounts();
	if(!ms){
		mount_set_info(_("Read mount table failed"));
		return;
	}
	for(size_t i=0;ms[i];i++){
		if(strncmp(ms[i]->source,"/dev/",5)!=0&&!is_show_all)continue;
		struct mount_info*m=malloc(sizeof(struct mount_info));
		if(!m)continue;
		memset(m,0,sizeof(struct mount_info));
		m->item=ms[i];
		mount_add_item(m);
		list_obj_add_new_notnull(&mounts,m);
	}
	if(!mounts)mount_set_info(_("No mount points found"));
	free(ms);
}

static void refresh_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_refresh)return;
	tlog_debug("request refresh");
	mount_reload();
}

static void mount_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_mount)return;
	msgbox_alert("This function does not implemented");
}

static void show_all_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED||obj!=show_all)return;
	is_show_all=lv_checkbox_is_checked(obj);
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
static void umount_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_umount||!selected)return;
	msgbox_set_user_data(msgbox_create_yesno(
		umount_cb,
		"Are you sure you want to unmount '%s'?",
		selected->item->source
	),selected);
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	list_free_all(mounts,item_free);
	info=NULL,last=NULL,mounts=NULL;
	return 0;
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	mount_reload();
	lv_group_add_obj(gui_grp,btn_mount);
	lv_group_add_obj(gui_grp,btn_umount);
	lv_group_add_obj(gui_grp,btn_refresh);
}

static int mount_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int mount_lost_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(mounts);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct mount_info*);
		lv_group_remove_obj(item->chk);
	}while((o=o->next));
	lv_group_remove_obj(btn_mount);
	lv_group_remove_obj(btn_umount);
	lv_group_remove_obj(btn_refresh);
	return 0;
}

static int mount_draw(struct gui_activity*act){

	lv_coord_t btm=gui_dpi/10;
	lv_coord_t btw=gui_sw/3-btm;
	lv_coord_t bth=gui_font_size+gui_dpi/10;

	// function title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_sh/16);
	lv_obj_set_size(title,gui_sw,gui_sh/16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("Mount Manager"));

	// options list
	static lv_style_t lst_style;
	lv_style_init(&lst_style);
	lv_style_set_border_width(&lst_style,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&lst_style,LV_STATE_FOCUSED,0);
	lv_style_set_border_width(&lst_style,LV_STATE_PRESSED,0);
	lst=lv_page_create(act->page,NULL);
	lv_obj_add_style(lst,LV_PAGE_PART_BG,&lst_style);
	lv_obj_set_pos(lst,gui_dpi/20,gui_sh/8);
	lv_obj_set_size(lst,gui_sw-gui_dpi/10,gui_sh-lv_obj_get_y(lst)-bth*2-btm*3);

	// show all checkbox
	show_all=lv_checkbox_create(act->page,NULL);
	lv_obj_set_size(show_all,bth,bth);
	lv_obj_align(show_all,lst,LV_ALIGN_OUT_BOTTOM_LEFT,btm,btm);
	lv_obj_set_event_cb(show_all,show_all_click);
	lv_style_set_focus_checkbox(show_all);
	lv_checkbox_set_text(show_all,_("Show all mountpoints"));

	// button style
	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
	lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);

	// mount button
	btn_mount=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_mount,btw,bth);
	lv_obj_align(btn_mount,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_event_cb(btn_mount,mount_click);
	lv_style_set_action_button(btn_mount,true);
	lv_label_set_text(lv_label_create(btn_mount,NULL),_("Mount"));

	// umount button
	btn_umount=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_umount,btw,bth);
	lv_obj_align(btn_umount,NULL,LV_ALIGN_IN_BOTTOM_MID,0,-btm);
	lv_obj_set_event_cb(btn_umount,umount_click);
	lv_style_set_action_button(btn_umount,false);
	lv_label_set_text(lv_label_create(btn_umount,NULL),_("Unmount"));

	// refresh button
	btn_refresh=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_refresh,btw,bth);
	lv_obj_align(btn_refresh,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-btm);
	lv_obj_set_event_cb(btn_refresh,refresh_click);
	lv_style_set_action_button(btn_refresh,true);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),_("Refresh"));

	return 0;
}

struct gui_register guireg_mount={
	.name="mount-manager",
	.title="Mount Manager",
	.icon="mount.png",
	.show_app=true,
	.draw=mount_draw,
	.quiet_exit=do_cleanup,
	.get_focus=mount_get_focus,
	.lost_focus=mount_lost_focus,
	.back=true,
};
#endif

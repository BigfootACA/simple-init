/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_FDISK
#define _GNU_SOURCE
#include<stdlib.h>
#include<libfdisk/libfdisk.h>
#include"gui.h"
#include"str.h"
#include"confd.h"
#include"guipm.h"
#include"system.h"
#include"logger.h"
#include"gui/activity.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#define TAG "guipm"

static void partition_clear(struct part_disk_info*di,bool ui){
	di->selected=NULL;
	if(ui){
		lv_obj_set_enabled(di->btn_part,false);
		lv_obj_set_enabled(di->btn_new,false);
		if(di->disk_info)lv_obj_del(di->disk_info);
	}
	if(di->table)fdisk_unref_table(di->table);
	memset(di->size_str,0,sizeof(di->size_str));
	memset(di->type,0,sizeof(di->type));
	di->size=0,di->secs=0,di->lsec_size=0,di->psec_size=0;
	di->disk_info=NULL,di->label=NULL,di->table=NULL;
	for(int i=0;i<1024;i++){
		struct part_partition_info*p=di->partitions[i];
		if(!p)continue;
		if(ui&&p->btn)lv_obj_del(p->btn);
		free(p);
		di->partitions[i]=NULL;
	}
	di->label=fdisk_get_label(di->ctx,NULL);
	if(!fdisk_label_is_changed(di->label))fdisk_reassign_device(di->ctx);
}

static void fill_disk_info(struct part_disk_info*di){
	di->secs=fdisk_get_nsectors(di->ctx);
	di->lsec_size=fdisk_get_sector_size(di->ctx);
	di->psec_size=fdisk_get_physector_size(di->ctx);
	di->size=di->secs*di->lsec_size;
	make_readable_str_buf(di->size_str,sizeof(di->size_str),di->size,1,0);
	strcpy(di->type,fdisk_label_get_name(di->label));
	tlog_debug("disk label: %s",di->type);
	tlog_debug(
		"disk size: %s (%lu bytes, %lu sectors)",
		di->size_str,di->size,di->secs
	);
	tlog_debug(
		"disk sector size: %lu bytes logical, %lu bytes physical",
		di->lsec_size,di->psec_size
	);
}

static int fill_partition_info(struct part_disk_info*di,struct part_partition_info*part,struct fdisk_iter*itr){
	memset(part,0,sizeof(struct part_partition_info));
	if(fdisk_table_next_partition(di->table,itr,&part->part)!=0)return -1;

	part->di=di;
	part->start_sec=fdisk_partition_get_start(part->part);
	part->end_sec=fdisk_partition_get_end(part->part);
	part->size_sec=fdisk_partition_get_size(part->part);

	part->start=part->start_sec*di->lsec_size;
	part->end=part->end_sec*di->lsec_size;
	part->size=part->size_sec*di->lsec_size;

	make_readable_str_buf(part->start_str,sizeof(part->start_str),part->start,1,0);
	make_readable_str_buf(part->end_str,sizeof(part->end_str),part->end,1,0);
	make_readable_str_buf(part->size_str,sizeof(part->size_str),part->size,1,0);
	part->free=fdisk_partition_is_freespace(part->part);
	if(part->free){
		tlog_debug(
			"free space start %s(%lu), end %s(%lu), size %s(%lu)",
			part->start_str,part->start_sec,
			part->end_str,part->end_sec,
			part->size_str,part->size_sec
		);
	}else{
		part->no=fdisk_partition_get_partno(part->part);

		part->type=fdisk_partition_get_type(part->part);
		char*pname=(char*)fdisk_partition_get_name(part->part);
		if(pname&&pname[0])strcpy(part->name,pname);
		strcpy(part->partname,fdisk_partname(di->target,part->no+1));
		strcpy(part->type_str,fdisk_parttype_get_name(part->type));
		tlog_debug(
			"%s start %s(%lu), end %s(%lu), size %s(%lu), name %s, type %s",
			part->partname,
			part->start_str,part->start_sec,
			part->end_str,part->end_sec,
			part->size_str,part->size_sec,
			part->name[0]?part->name:"(none)",
			part->type_str
		);
	}
	return 0;
}

static void partition_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(di->selected){
		lv_obj_set_checked(di->selected->btn,false);
		lv_obj_set_enabled(di->btn_part,false);
		lv_obj_set_enabled(di->btn_new,false);
		if(obj==di->selected->chk){
			tlog_debug("clear selected");
			di->selected=NULL;
			return;
		}else lv_checkbox_set_checked(di->selected->chk,false);
	}
	di->selected=NULL;
	int i;
	for(i=0;i<1024&&!di->selected;i++)
		if(di->partitions[i]&&di->partitions[i]->chk==obj)
			di->selected=di->partitions[i];
	if(!di->selected)return;
	lv_obj_set_checked(di->selected->btn,true);
	if(di->selected->free){
		tlog_debug("selected free space %d",i);
		lv_obj_set_enabled(di->btn_new,true);
	}else{
		tlog_debug("selected partition %s",di->selected->partname);
		lv_obj_set_enabled(di->btn_part,true);
	}
}

static void partitions_add_item(int i,struct part_partition_info*p){
	struct part_disk_info*di=p->di;
	lv_coord_t bw=lv_page_get_scrl_width(di->page),m;

	// disk select button
	p->btn=lv_btn_create(di->page,NULL);
	if(i>0)lv_obj_align(p->btn,di->partitions[i-1]->btn,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_dpi/10);
	lv_obj_set_size(p->btn,bw,gui_dpi/3*2);
	lv_style_set_btn_item(p->btn);
	if(p->free)lv_obj_set_gray240_text_color(p->btn,LV_BTN_PART_MAIN);
	lv_obj_set_click(p->btn,false);

	// line for button text
	lv_obj_t*line=lv_line_create(p->btn,NULL);
	lv_obj_set_width(line,bw);

	// partition name and checkbox
	m=gui_dpi/20;
	p->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(p->chk,p->free?_("Free Space"):p->partname);
	if(p->free)lv_obj_set_gray160_text_color(p->chk,LV_LABEL_PART_MAIN);
	lv_style_set_focus_checkbox(p->chk);
	lv_obj_set_user_data(p->chk,di);
	lv_obj_set_event_cb(p->chk,partition_click);
	lv_group_add_obj(gui_grp,p->chk);
	lv_obj_align(p->chk,NULL,LV_ALIGN_IN_TOP_LEFT,m,lv_obj_get_height(p->chk)+m);

	// partition size
	m=gui_dpi/10;
	lv_obj_t*size=lv_label_create(line,NULL);
	lv_label_set_text_fmt(size,"%s(%lu)",p->size_str,p->size_sec);
	lv_label_set_align(size,LV_LABEL_ALIGN_RIGHT);
	lv_obj_align(size,NULL,LV_ALIGN_IN_TOP_RIGHT,-m,lv_obj_get_height(size)+m);

	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT;

	// partition position
	lv_obj_t*pos=lv_label_create(line,NULL);
	lv_label_set_long_mode(pos,lm);
	lv_label_set_text_fmt(
		pos,_("Start: %s(%lu) End %s(%lu)"),
		p->start_str,p->start_sec,
		p->end_str,p->end_sec
	);
	lv_obj_set_width(pos,bw-gui_dpi/5);
	lv_label_set_align(pos,LV_LABEL_ALIGN_LEFT);
	lv_obj_set_small_text_font(pos,LV_LABEL_PART_MAIN);
	lv_obj_align(pos,NULL,LV_ALIGN_IN_LEFT_MID,gui_dpi/10,gui_dpi/20);

	if(!p->free){
		// partition name
		lv_obj_t*name=lv_label_create(line,NULL);
		lv_label_set_long_mode(name,lm);
		lv_obj_set_small_text_font(name,LV_LABEL_PART_MAIN);
		if(!p->name[0])lv_obj_set_gray160_text_color(name,LV_LABEL_PART_MAIN);
		lv_label_set_text(name,p->name[0]?p->name:_("(none)"));
		lv_obj_set_width(name,bw/8*3-gui_dpi/10);
		lv_label_set_align(name,LV_LABEL_ALIGN_LEFT);
		lv_obj_align(
			name,NULL,
			LV_ALIGN_IN_BOTTOM_LEFT,
			gui_dpi/10,
			-lv_obj_get_height(p->chk)-gui_dpi/25
		);

		// partition name
		lv_obj_t*type=lv_label_create(line,NULL);
		lv_label_set_long_mode(type,lm);
		lv_obj_set_small_text_font(type,LV_LABEL_PART_MAIN);
		if(!p->type_str[0])lv_obj_set_gray160_text_color(type,LV_LABEL_PART_MAIN);
		lv_label_set_text(type,p->type_str[0]?p->type_str:_("(unknown)"));
		lv_obj_set_width(type,bw/8*5-gui_dpi/5);
		lv_label_set_align(type,LV_LABEL_ALIGN_LEFT);
		lv_obj_align(
			type,name,
			LV_ALIGN_OUT_RIGHT_MID,
			gui_dpi/10,
			0
		);
	}
}

static void set_disks_info(struct part_disk_info*di,char*text){
	if(di->disk_info)lv_obj_del(di->disk_info);
	di->disk_info=lv_label_create(di->page,NULL);
	lv_label_set_long_mode(di->disk_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(di->disk_info,lv_page_get_scrl_width(di->page),gui_sh/16);
	lv_label_set_align(di->disk_info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(di->disk_info,text);
}

static int reload_partitions(struct part_disk_info*di){
	partition_clear(di,true);
	if(!fdisk_has_label(di->ctx)){
		set_disks_info(di,_("This disk has no label"));
		return 0;
	}
	fill_disk_info(di);
	if(fdisk_get_partitions(di->ctx,&di->table)!=0)
		return tlog_warn("no any partitions found");
	if(fdisk_get_freespaces(di->ctx,&di->table)!=0)
		return tlog_warn("no any free spaces found");
	if(fdisk_table_get_nents(di->table)<=0)return 0;
	fdisk_list_disklabel(di->ctx);
	struct fdisk_iter*itr=NULL;
	if(!(itr=fdisk_new_iter(FDISK_ITER_FORWARD)))
		return terlog_warn(-1,"create fdisk iterable failed");
	for(size_t i=0;i<1024;i++){
		struct part_partition_info*p=malloc(sizeof(struct part_partition_info));
		if(fill_partition_info(di,p,itr)!=0){
			free(p);
			break;
		}
		di->partitions[i]=p;
		partitions_add_item(i,p);
	}
	fdisk_free_iter(itr);
	return 0;
}

static int init_disk(struct part_disk_info*di){
	errno=0;
	if(!(di->ctx=fdisk_new_context()))
		return terlog_error(-1,"failed to initialize fdisk context");
	size_t s=strlen(di->target)+16;
	if(di->path)free(di->path);
	if(!(di->path=malloc(s)))
		return terlog_error(-1,"malloc path failed");
	if(di->target[0]=='/'){
		strcpy(di->path,di->target);
		struct stat st;
		if(stat(di->path,&st)!=0){
			telog_error("cannot stat %s",di->path);
			goto fail;
		}
		if(!S_ISREG(st.st_mode)&&!S_ISBLK(st.st_mode)){
			telog_error("unknown type %s",di->path);
			goto fail;
		}
		if(S_ISREG(st.st_mode)&&st.st_size==0){
			telog_error("empty file %s",di->path);
			goto fail;
		}
	}else{
		snprintf(di->path,s-1,_PATH_DEV"/%s",di->target);
		if(!is_block(di->path)){
			snprintf(di->path,s-1,_PATH_DEV"/block/%s",di->target);
			if(!is_block(di->path)){
				telog_error("cannot found device %s real path",di->target);
				goto fail;
			}
		}
	}
	tlog_debug("found disk %s",di->path);
	errno=0;
	if(fdisk_assign_device(di->ctx,di->path,false)!=0){
		telog_error("failed assign block %s to fdisk context",di->target);
		goto fail;
	}
	return 0;
	fail:
	if(di->ctx)fdisk_unref_context(di->ctx);
	if(di->path)free(di->path);
	di->ctx=NULL,di->path=NULL;
	return -1;
}

static void reload_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(obj!=di->btn_reload)return;
	tlog_debug("request reload");
	reload_partitions(di);
}

static void save_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(obj!=di->btn_save)return;
	tlog_debug("request save");
	if(guipm_save_label(di->ctx))lv_obj_set_enabled(obj,false);
}

static void new_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(obj!=di->btn_new||!di->selected||!di->selected->free)return;
	tlog_debug("request new");
	guiact_start_activity(&guireg_guipm_new_partition,di->selected);
}

static void disk_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(obj!=di->btn_disk)return;
	tlog_debug("request disk submenu");
	guipm_disk_operation_menu(di->ctx);
}

static void part_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct part_disk_info*di=lv_obj_get_user_data(obj);
	if(obj!=di->btn_part||!di->selected||di->selected->free)return;
	tlog_debug("request partition submenu");
	guipm_part_operation_menu(di->selected);
}

static void do_reload(lv_task_t*t){
	struct gui_activity*d=t->user_data;
	struct part_disk_info*di=d->data;
	if(!guiact_is_active_page(d->page)||!di)return;
	errno=0;
	reload_partitions(di);
	lv_group_add_obj(gui_grp,di->btn_disk);
	lv_group_add_obj(gui_grp,di->btn_part);
	lv_group_add_obj(gui_grp,di->btn_reload);
	lv_group_add_obj(gui_grp,di->btn_save);
	lv_group_add_obj(gui_grp,di->btn_new);
	lv_obj_set_enabled(di->btn_save,fdisk_label_is_changed(di->label));
}

static int guipm_part_get_focus(struct gui_activity*d){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,d));
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_disk_info*di=d->data;
	if(!di)return 0;
	for(int i=0;i<1024;i++){
		if(!di->partitions[i])continue;
		lv_group_remove_obj(di->partitions[i]->chk);
	}
	lv_group_remove_obj(di->btn_new);
	lv_group_remove_obj(di->btn_disk);
	lv_group_remove_obj(di->btn_part);
	lv_group_remove_obj(di->btn_save);
	lv_group_remove_obj(di->btn_reload);
	return 0;
}

static int do_cleanup(struct gui_activity*d){
	struct part_disk_info*di=d->data;
	if(!di)return 0;
	partition_clear(di,false);
	if(di->target)free(di->target);
	if(di->ctx)fdisk_unref_context(di->ctx);
	if(di->path)free(di->path);
	di->target=NULL,di->ctx=NULL,di->path=NULL;
	free(di);
	d->data=NULL;
	return 0;
}

static int init(struct gui_activity*act){
	if(!act->args){
		tlog_warn("target disk not set");
		return -EINVAL;
	}
	struct part_disk_info*di=malloc(sizeof(struct part_disk_info));
	if(!di)return -ENOMEM;
	memset(di,0,sizeof(struct part_disk_info));
	char*disk=act->args;
	if(disk[0]!='/'&&disk[1]==':')disk+=2;
	if(!(di->target=strdup(disk))){
		free(di);
		return -ENOMEM;
	}
	act->data=di;
	act->mask=init_disk(di)<0;
	return 0;
}

static int guipm_draw_partitions(struct gui_activity*act){
	struct part_disk_info*di=act->data;
	if(act->mask){
		msgbox_alert("init disk context failed");
		free(di);
		return -1;
	}else{
		lv_coord_t btw1=gui_sw/2-(gui_dpi/5);
		lv_coord_t btw2=gui_sw/3-(gui_dpi/8);
		lv_coord_t bth=gui_font_size+(gui_dpi/10);

		guipm_draw_title(act->page);

		// function title
		lv_obj_t*title=lv_label_create(act->page,NULL);
		lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
		lv_obj_set_y(title,gui_sh/16);
		lv_obj_set_size(title,gui_sw,gui_sh/16);
		lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
		lv_label_set_text(title,_("Partition a disk"));

		// partitions list
		static lv_style_t lst_style;
		lv_style_init(&lst_style);
		lv_style_set_border_width(&lst_style,LV_STATE_DEFAULT,0);
		lv_style_set_border_width(&lst_style,LV_STATE_FOCUSED,0);
		lv_style_set_border_width(&lst_style,LV_STATE_PRESSED,0);
		di->page=lv_page_create(act->page,NULL);
		lv_obj_add_style(di->page,LV_PAGE_PART_BG,&lst_style);
		lv_obj_set_width(di->page,gui_sw-gui_dpi/10);
		lv_obj_align(di->page,title,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
		lv_obj_set_height(di->page,gui_sh-bth*2-gui_font_size*3-gui_sh/16*2);

		// disk operate button
		di->btn_disk=lv_btn_create(act->page,NULL);
		lv_obj_set_size(di->btn_disk,btw1,bth);
		lv_style_set_action_button(di->btn_disk,true);
		lv_obj_set_user_data(di->btn_disk,di);
		lv_obj_set_event_cb(di->btn_disk,disk_click);
		lv_label_set_text(lv_label_create(di->btn_disk,NULL),_("Disk..."));
		lv_obj_align(di->btn_disk,di->page,LV_ALIGN_OUT_BOTTOM_LEFT,gui_font_size/2,gui_font_size);
		lv_group_add_obj(gui_grp,di->btn_disk);

		// partition operate button
		di->btn_part=lv_btn_create(act->page,NULL);
		lv_obj_set_size(di->btn_part,btw1,bth);
		lv_obj_set_user_data(di->btn_part,di);
		lv_obj_set_event_cb(di->btn_part,part_click);
		lv_style_set_action_button(di->btn_part,false);
		lv_label_set_text(lv_label_create(di->btn_part,NULL),_("Partition..."));
		lv_obj_align(di->btn_part,di->page,LV_ALIGN_OUT_BOTTOM_RIGHT,-gui_font_size/2,gui_font_size);
		lv_group_add_obj(gui_grp,di->btn_part);

		// reload button
		di->btn_reload=lv_btn_create(act->page,NULL);
		lv_obj_set_size(di->btn_reload,btw2,bth);
		lv_obj_align(di->btn_reload,di->page,LV_ALIGN_OUT_BOTTOM_LEFT,gui_font_size/2,gui_font_size*2+bth);
		lv_obj_set_user_data(di->btn_reload,di);
		lv_style_set_action_button(di->btn_reload,true);
		lv_obj_set_event_cb(di->btn_reload,reload_click);
		lv_label_set_text(lv_label_create(di->btn_reload,NULL),_("Reload"));
		lv_group_add_obj(gui_grp,di->btn_reload);

		// save button
		di->btn_save=lv_btn_create(act->page,NULL);
		lv_obj_set_size(di->btn_save,btw2,bth);
		lv_obj_align(di->btn_save,di->page,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size*2+bth);
		lv_obj_set_user_data(di->btn_save,di);
		lv_style_set_action_button(di->btn_save,true);
		lv_obj_set_event_cb(di->btn_save,save_click);
		lv_label_set_text(lv_label_create(di->btn_save,NULL),_("Save"));
		lv_group_add_obj(gui_grp,di->btn_save);

		// new partition button
		di->btn_new=lv_btn_create(act->page,NULL);
		lv_obj_set_size(di->btn_new,btw2,bth);
		lv_obj_align(di->btn_new,di->page,LV_ALIGN_OUT_BOTTOM_RIGHT,-gui_font_size/2,gui_font_size*2+bth);
		lv_obj_set_user_data(di->btn_new,di);
		lv_style_set_action_button(di->btn_new,false);
		lv_obj_set_event_cb(di->btn_new,new_click);
		lv_label_set_text(lv_label_create(di->btn_new,NULL),_("New"));
		lv_group_add_obj(gui_grp,di->btn_new);
	}
	return 0;
}

struct gui_register guireg_guipm_partitions={
	.name="guipm-partitions",
	.title="Partition Manager",
	.icon="guipm.png",
	.show_app=false,
	.open_file=true,
	.init=init,
	.quiet_exit=do_cleanup,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_partitions,
	.back=true
};
#endif
#endif

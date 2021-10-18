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
#include"system.h"
#include"logger.h"
#include"gui/activity.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#define TAG "guipm"

static char*guipm_target_disk=NULL;
static char*path=NULL;
static lv_obj_t*selscr,*page,*disk_info=NULL;
static lv_obj_t*btn_disk,*btn_part,*btn_reload,*btn_new;
static struct fdisk_context*ctx;
static struct disk_info{
	struct fdisk_label*label;
	struct fdisk_table*table;
	fdisk_sector_t secs,lsec_size,psec_size;
	unsigned long size;
	char size_str[80];
	char type[128];
}diskinfo;
static struct partition_info{
	bool free;
	size_t no;
	char partname[32];
	char name[320];
	struct fdisk_partition*part;
	struct fdisk_parttype*type;
	fdisk_sector_t start_sec,end_sec,size_sec;
	unsigned long start,end,size;
	char start_str[64],end_str[64],size_str[64];
	char type_str[128];
	lv_obj_t*btn,*chk;
}*partitions[1024],*selected;

extern void guipm_draw_title(lv_obj_t*screen);

static void partition_clear(){
	selected=NULL;
	lv_obj_set_enabled(btn_part,false);
	lv_obj_set_enabled(btn_new,false);
	if(disk_info)lv_obj_del(disk_info);
	disk_info=NULL;
	if(diskinfo.table)fdisk_unref_table(diskinfo.table);
	memset(&diskinfo,0,sizeof(struct disk_info));
	for(int i=0;i<1024;i++){
		struct partition_info*p=partitions[i];
		if(!p)continue;
		if(p->btn)lv_obj_del(p->btn);
		if(p->type)fdisk_unref_parttype(p->type);
		free(p);
		partitions[i]=NULL;
	}
	fdisk_reassign_device(ctx);
}

static void fill_disk_info(){
	diskinfo.label=fdisk_get_label(ctx,NULL);
	diskinfo.secs=fdisk_get_nsectors(ctx);
	diskinfo.lsec_size=fdisk_get_sector_size(ctx);
	diskinfo.psec_size=fdisk_get_physector_size(ctx);
	diskinfo.size=diskinfo.secs*diskinfo.lsec_size;
	make_readable_str_buf(diskinfo.size_str,sizeof(diskinfo.size_str),diskinfo.size,1,0);
	strcpy(diskinfo.type,fdisk_label_get_name(diskinfo.label));
	tlog_debug("disk label: %s",diskinfo.type);
	tlog_debug(
		"disk size: %s (%lu bytes, %lu sectors)",
		diskinfo.size_str,diskinfo.size,diskinfo.secs
	);
	tlog_debug(
		"disk sector size: %lu bytes logical, %lu bytes physical",
		diskinfo.lsec_size,diskinfo.psec_size
	);
}

static int fill_partition_info(struct partition_info*part,struct fdisk_iter*itr){
	memset(part,0,sizeof(struct partition_info));
	if(fdisk_table_next_partition(diskinfo.table,itr,&part->part)!=0)return -1;

	part->start_sec=fdisk_partition_get_start(part->part);
	part->end_sec=fdisk_partition_get_end(part->part);
	part->size_sec=fdisk_partition_get_size(part->part);

	part->start=part->start_sec*diskinfo.lsec_size;
	part->end=part->end_sec*diskinfo.lsec_size;
	part->size=part->size_sec*diskinfo.lsec_size;

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
		strcpy(part->partname,fdisk_partname(guipm_target_disk,part->no+1));
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
	if(selected){
		lv_obj_set_checked(selected->btn,false);
		lv_obj_set_enabled(btn_part,false);
		lv_obj_set_enabled(btn_new,false);
		if(obj==selected->chk){
			tlog_debug("clear selected");
			selected=NULL;
			return;
		}else lv_checkbox_set_checked(selected->chk,false);
	}
	selected=NULL;
	int i;
	for(i=0;i<1024&&!selected;i++)
		if(partitions[i]&&partitions[i]->chk==obj)
			selected=partitions[i];
	if(!selected)return;
	lv_obj_set_checked(selected->btn,true);
	if(selected->free){
		tlog_debug("selected free space %d",i);
		lv_obj_set_enabled(btn_new,true);
	}else{
		tlog_debug("selected partition %s",selected->partname);
		lv_obj_set_enabled(btn_part,true);
	}
}

static void partitions_add_item(int i,struct partition_info*p){
	lv_coord_t bw=lv_page_get_scrl_width(page),m;

	// disk select button
	p->btn=lv_btn_create(page,NULL);
	if(i>0)lv_obj_align(p->btn,partitions[i-1]->btn,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_dpi/10);
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
	lv_obj_set_event_cb(p->chk,partition_click);
	lv_group_add_obj(gui_grp,p->chk);
	lv_obj_align(p->chk,NULL,LV_ALIGN_IN_TOP_LEFT,m,lv_obj_get_height(p->chk)+m);

	// partition size
	m=gui_dpi/10;
	lv_obj_t*size=lv_label_create(line,NULL);
	lv_label_set_text_fmt(size,"%s(%lu)",p->size_str,p->size_sec);
	lv_label_set_align(size,LV_LABEL_ALIGN_RIGHT);
	lv_obj_align(size,NULL,LV_ALIGN_IN_TOP_RIGHT,-m,lv_obj_get_height(size)+m);

	// partition position
	lv_obj_t*pos=lv_label_create(line,NULL);
	lv_label_set_long_mode(pos,LV_LABEL_LONG_SROLL_CIRC);
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

		static lv_style_t xxx;
		lv_style_init(&xxx);

		// partition name
		lv_obj_t*name=lv_label_create(line,NULL);
		lv_label_set_long_mode(name,LV_LABEL_LONG_SROLL_CIRC);
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
		lv_label_set_long_mode(type,LV_LABEL_LONG_SROLL_CIRC);
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

static void set_disks_info(char*text){
	if(disk_info)lv_obj_del(disk_info);
	disk_info=lv_label_create(page,NULL);
	lv_label_set_long_mode(disk_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(disk_info,lv_page_get_scrl_width(page),gui_sh/16);
	lv_label_set_align(disk_info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(disk_info,text);
}

static int reload_partitions(){
	partition_clear();
	if(!fdisk_has_label(ctx)){
		set_disks_info(_("This disk has no label"));
		return 0;
	}
	fill_disk_info();
	if(fdisk_get_partitions(ctx,&diskinfo.table)!=0)
		return tlog_warn("no any partitions found");
	if(fdisk_get_freespaces(ctx,&diskinfo.table)!=0)
		return tlog_warn("no any free spaces found");
	if(fdisk_table_get_nents(diskinfo.table)<=0)return 0;
	fdisk_list_disklabel(ctx);
	struct fdisk_iter*itr=NULL;
	if(!(itr=fdisk_new_iter(FDISK_ITER_FORWARD)))
		return terlog_warn(-1,"create fdisk iterable failed");
	for(size_t i=0;i<1024;i++){
		struct partition_info*p=malloc(sizeof(struct partition_info));
		if(fill_partition_info(p,itr)!=0){
			free(p);
			break;
		}
		partitions[i]=p;
		partitions_add_item(i,p);
	}
	fdisk_free_iter(itr);
	return 0;
}

static int init_disk(){
	errno=0;
	if(!(ctx=fdisk_new_context()))
		return terlog_error(-1,"failed to initialize fdisk context");
	size_t s=strlen(guipm_target_disk)+16;
	if(path)free(path);
	if(!(path=malloc(s)))
		return terlog_error(-1,"malloc path failed");
	if(guipm_target_disk[0]=='/'){
		strcpy(path,guipm_target_disk);
		struct stat st;
		if(stat(path,&st)!=0){
			telog_error("cannot stat %s",path);
			goto fail;
		}
		if(!S_ISREG(st.st_mode)&&!S_ISBLK(st.st_mode)){
			telog_error("unknown type %s",path);
			goto fail;
		}
		if(S_ISREG(st.st_mode)&&st.st_size==0){
			telog_error("empty file %s",path);
			goto fail;
		}
	}else{
		snprintf(path,s-1,_PATH_DEV"/%s",guipm_target_disk);
		if(!is_block(path)){
			snprintf(path,s-1,_PATH_DEV"/block/%s",guipm_target_disk);
			if(!is_block(path)){
				telog_error("cannot found device %s real path",guipm_target_disk);
				goto fail;
			}
		}
	}
	tlog_debug("found disk %s",path);
	errno=0;
	if(fdisk_assign_device(ctx,path,true)!=0){
		telog_error("failed assign block %s to fdisk context",guipm_target_disk);
		goto fail;
	}
	return 0;
	fail:
	if(ctx)fdisk_unref_context(ctx);
	if(path)free(path);
	ctx=NULL,path=NULL;
	return -1;
}

static void reload_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_reload)return;
	tlog_debug("request reload");
	reload_partitions();
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	errno=0;
	reload_partitions();
	lv_group_add_obj(gui_grp,btn_disk);
	lv_group_add_obj(gui_grp,btn_part);
	lv_group_add_obj(gui_grp,btn_reload);
	lv_group_add_obj(gui_grp,btn_new);
}

static int guipm_part_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<1024;i++){
		if(!partitions[i])continue;
		lv_group_remove_obj(partitions[i]->chk);
	}
	lv_group_remove_obj(btn_new);
	lv_group_remove_obj(btn_disk);
	lv_group_remove_obj(btn_part);
	lv_group_remove_obj(btn_reload);
	return 0;
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	partition_clear();
	if(guipm_target_disk)free(guipm_target_disk);
	if(ctx)fdisk_unref_context(ctx);
	if(path)free(path);
	guipm_target_disk=NULL,ctx=NULL,path=NULL;
	return 0;
}

static int init(struct gui_activity*act){
	if(!act->args){
		tlog_warn("target disk not set");
		return -EINVAL;
	}
	char*disk=act->args;
	if(disk[0]!='/'&&disk[1]==':')disk+=2;
	if(!(guipm_target_disk=strdup(disk)))return -ENOMEM;
	act->mask=init_disk()<0;
	return 0;
}

static int guipm_draw_partitions(struct gui_activity*act){
	selscr=act->page;
	if(act->mask){
		msgbox_alert("init disk context failed");
		return -1;
	}else{
		int btw=gui_sw/2-(gui_dpi/5),bth=gui_font_size+(gui_dpi/10);

		guipm_draw_title(selscr);

		// function title
		lv_obj_t*title=lv_label_create(selscr,NULL);
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
		page=lv_page_create(selscr,NULL);
		lv_obj_add_style(page,LV_PAGE_PART_BG,&lst_style);
		lv_obj_set_width(page,gui_sw-gui_dpi/10);
		lv_obj_align(page,title,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/10);
		lv_obj_set_height(page,gui_sh-bth*2-gui_font_size*3-gui_sh/16*2);

		// disk operate button
		btn_disk=lv_btn_create(selscr,NULL);
		lv_obj_set_size(btn_disk,btw,bth);
		lv_style_set_action_button(btn_disk,true);
		lv_label_set_text(lv_label_create(btn_disk,NULL),_("Disk..."));
		lv_obj_align(btn_disk,page,LV_ALIGN_OUT_BOTTOM_LEFT,gui_font_size/2,gui_font_size);
		lv_group_add_obj(gui_grp,btn_disk);

		// partition operate button
		btn_part=lv_btn_create(selscr,NULL);
		lv_obj_set_size(btn_part,btw,bth);
		lv_style_set_action_button(btn_part,false);
		lv_label_set_text(lv_label_create(btn_part,NULL),_("Partition..."));
		lv_obj_align(btn_part,page,LV_ALIGN_OUT_BOTTOM_RIGHT,-gui_font_size/2,gui_font_size);
		lv_group_add_obj(gui_grp,btn_part);

		// reload button
		btn_reload=lv_btn_create(selscr,NULL);
		lv_obj_align(btn_reload,btn_disk,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
		lv_obj_set_size(btn_reload,btw,bth);
		lv_style_set_action_button(btn_reload,true);
		lv_obj_set_event_cb(btn_reload,reload_click);
		lv_label_set_text(lv_label_create(btn_reload,NULL),_("Reload"));
		lv_group_add_obj(gui_grp,btn_reload);

		// new partition button
		btn_new=lv_btn_create(selscr,NULL);
		lv_obj_align(btn_new,btn_part,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size);
		lv_obj_set_size(btn_new,btw,bth);
		lv_style_set_action_button(btn_new,false);
		lv_label_set_text(lv_label_create(btn_new,NULL),_("New"));
		lv_group_add_obj(gui_grp,btn_new);
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

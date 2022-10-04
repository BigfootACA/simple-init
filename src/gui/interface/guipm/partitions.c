/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
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
		const char*pname=fdisk_partition_get_name(part->part);
		if(pname&&pname[0])strcpy(part->name,pname);
		char*partname=fdisk_partname(di->target,part->no+1);
		if(partname){
			if(partname[0])strcpy(part->partname,partname);
			free(partname);
		}
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

static void partition_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	if(di->selected){
		lv_obj_set_checked(di->selected->btn,false);
		lv_obj_set_enabled(di->btn_part,false);
		lv_obj_set_enabled(di->btn_new,false);
		if(e->target==di->selected->btn){
			tlog_debug("clear selected");
			di->selected=NULL;
			return;
		}else lv_obj_set_checked(di->selected->btn,false);
	}
	di->selected=NULL;
	int i;
	for(i=0;i<1024&&!di->selected;i++)
		if(di->partitions[i]&&di->partitions[i]->btn==e->target)
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

static void partitions_add_item(struct part_partition_info*p){
	struct part_disk_info*di=p->di;
	static lv_coord_t grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	},grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	};

	// disk select button
	p->btn=lv_btn_create(di->page);
	lv_style_set_btn_item(p->btn);
	lv_obj_set_size(p->btn,lv_pct(100),gui_font_size*(p->free?4:5));
	lv_obj_set_grid_dsc_array(p->btn,grid_col,grid_row);
	lv_obj_add_event_cb(p->btn,partition_click,LV_EVENT_CLICKED,di);
	lv_group_add_obj(gui_grp,p->btn);

	// partition name and checkbox
	lv_obj_t*lbl=lv_label_create(p->btn);
	lv_label_set_text(lbl,p->free?_("Free Space"):p->partname);
	lv_obj_set_grid_cell(
		lbl,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	// partition size
	lv_obj_t*size=lv_label_create(p->btn);
	lv_label_set_long_mode(size,LV_LABEL_LONG_CLIP);
	lv_label_set_text_fmt(size,"%s(%lu)",p->size_str,p->size_sec);
	lv_obj_set_style_text_align(size,LV_TEXT_ALIGN_RIGHT,0);
	lv_obj_set_small_text_font(size,LV_PART_MAIN);
	lv_obj_set_grid_cell(
		size,
		LV_GRID_ALIGN_END,1,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;

	// partition position
	lv_obj_t*pos=lv_label_create(p->btn);
	lv_label_set_long_mode(pos,lm);
	lv_label_set_text_fmt(
		pos,_("Start: %s(%lu) End %s(%lu)"),
		p->start_str,p->start_sec,
		p->end_str,p->end_sec
	);
	lv_obj_set_style_text_align(pos,LV_TEXT_ALIGN_LEFT,0);
	lv_obj_set_small_text_font(pos,LV_PART_MAIN);
	lv_obj_set_grid_cell(
		pos,
		LV_GRID_ALIGN_START,0,2,
		LV_GRID_ALIGN_CENTER,1,p->free?2:1
	);

	if(!p->free){
		// partition type
		lv_obj_t*type=lv_label_create(p->btn);
		lv_label_set_long_mode(type,lm);
		lv_obj_set_small_text_font(type,LV_PART_MAIN);
		lv_label_set_text(type,p->type_str[0]?p->type_str:_("(unknown)"));
		lv_obj_set_style_text_align(type,LV_TEXT_ALIGN_LEFT,0);
		lv_obj_set_grid_cell(
			type,
			LV_GRID_ALIGN_START,0,1,
			LV_GRID_ALIGN_CENTER,2,1
		);

		// partition name
		lv_obj_t*name=lv_label_create(p->btn);
		lv_label_set_long_mode(name,lm);
		lv_obj_set_small_text_font(name,LV_PART_MAIN);
		lv_label_set_text(name,p->name[0]?p->name:_("(none)"));
		lv_obj_set_style_text_align(name,LV_TEXT_ALIGN_RIGHT,0);
		lv_obj_set_grid_cell(
			name,
			LV_GRID_ALIGN_END,1,1,
			LV_GRID_ALIGN_CENTER,2,1
		);
	}
}

static void set_disks_info(struct part_disk_info*di,char*text){
	if(di->disk_info)lv_obj_del(di->disk_info);
	di->disk_info=lv_label_create(di->page);
	lv_label_set_long_mode(di->disk_info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(di->disk_info,lv_pct(100));
	lv_obj_set_style_text_align(di->disk_info,LV_TEXT_ALIGN_CENTER,0);
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
		partitions_add_item(p);
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

static void reload_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	tlog_debug("request reload");
	reload_partitions(di);
}

static void save_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	tlog_debug("request save");
	if(guipm_save_label(di->ctx))
		lv_obj_set_enabled(e->target,false);
}

static void new_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	if(!di->selected||!di->selected->free)return;
	tlog_debug("request new");
	guiact_start_activity(&guireg_guipm_new_partition,di->selected);
}

static void disk_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	tlog_debug("request disk submenu");
	guipm_disk_operation_menu(di->ctx);
}

static void part_click(lv_event_t*e){
	struct part_disk_info*di=e->user_data;
	if(!di->selected||di->selected->free)return;
	tlog_debug("request partition submenu");
	guipm_part_operation_menu(di->selected);
}

static int do_reload(struct gui_activity*d){
	struct part_disk_info*di=d->data;
	if(!guiact_is_active_page(d->page)||!di)return -1;
	errno=0;
	reload_partitions(di);
	return 0;
}

static int guipm_part_get_focus(struct gui_activity*d){
	struct part_disk_info*di=d->data;
	if(!guiact_is_active_page(d->page)||!di)return -1;
	lv_group_add_obj(gui_grp,di->btn_disk);
	lv_group_add_obj(gui_grp,di->btn_part);
	lv_group_add_obj(gui_grp,di->btn_reload);
	lv_group_add_obj(gui_grp,di->btn_save);
	lv_group_add_obj(gui_grp,di->btn_new);
	lv_obj_set_enabled(di->btn_save,fdisk_label_is_changed(di->label));
	return 0;
}

static int guipm_part_lost_focus(struct gui_activity*d){
	struct part_disk_info*di=d->data;
	if(!di)return 0;
	for(int i=0;i<1024;i++){
		if(!di->partitions[i])continue;
		lv_group_remove_obj(di->partitions[i]->btn);
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
	struct part_disk_info*di;
	if(!act->args){
		tlog_warn("target disk not set");
		return -EINVAL;
	}
	if(!(di=malloc(sizeof(struct part_disk_info))))return -ENOMEM;
	memset(di,0,sizeof(struct part_disk_info));
	char*disk=act->args,*x;
	if((x=strstr(disk,"://"))){
		if(strncmp(disk,"file",x-disk)!=0){
			msgbox_alert("Only file:// path supported");
			return -1;
		}
		disk=x+3;
	}
	if(!(di->target=strdup(disk))){
		free(di);
		return -ENOMEM;
	}
	if(init_disk(di)<0){
		msgbox_alert("init disk context failed");
		free(di->target);
		free(di);
		return -1;
	}
	act->data=di;
	return 0;
}

static int guipm_draw_partitions(struct gui_activity*act){
	struct part_disk_info*di=act->data;
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);
	guipm_draw_title(act->page);

	// function title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("Partition a disk"));

	// partitions list
	di->page=lv_obj_create(act->page);
	lv_obj_set_width(di->page,lv_pct(100));
	lv_obj_set_flex_flow(di->page,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(di->page,1);

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,title,x,c,y,r)&(struct button_dsc){\
			&di->btn_##tgt,en,_(title),\
			tgt##_click,di,x,c,y,r,NULL\
		}
		BTN(disk,   true,  "Disk...",      0,3,0,1),
		BTN(part,   false, "Partition...", 3,3,0,1),
		BTN(reload, true,  "Reload",       0,2,1,1),
		BTN(save,   false, "Save",         2,2,1,1),
		BTN(new,    false, "New",          4,2,1,1),
		#undef BTN
		NULL
	);
	return 0;
}

struct gui_register guireg_guipm_partitions={
	.name="guipm-partitions",
	.title="Partition Manager",
	.show_app=false,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"file:///dev/.+",
		".*\\.img$",
		".*\\.raw$",
		NULL
	},
	.init=init,
	.quiet_exit=do_cleanup,
	.get_focus=guipm_part_get_focus,
	.lost_focus=guipm_part_lost_focus,
	.draw=guipm_draw_partitions,
	.data_load=do_reload,
	.back=true
};
#endif

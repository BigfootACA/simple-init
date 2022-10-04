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
#include"gui/tools.h"
#include"gui/activity.h"
#define TAG "guipm"

static char*get_model(struct disks_disk_info*d){
	return d->model[0]==0?"Unknown":d->model;
}

static char*get_layout(struct disks_disk_info*d){
	return d->layout[0]==0?"Unknown":d->layout;
}

static void disk_click(lv_event_t*e){
	struct disks_info*di=e->user_data;
	if(!di)return;
	lv_obj_set_checked(e->target,true);
	if(di->selected){
		if(e->target==di->selected->btn)return;
		else lv_obj_set_checked(di->selected->btn,false);
	}
	di->selected=NULL;
	for(int i=0;i<32&&!di->selected;i++)
		if(di->disks[i].enable&&di->disks[i].btn==e->target)
			di->selected=&di->disks[i];
	if(!di->selected)return;
	lv_obj_set_checked(di->selected->btn,true);
	tlog_debug("selected disk %s",di->selected->name);
	lv_obj_set_enabled(di->btn_ok,true);
	lv_group_focus_obj(di->btn_ok);
}

static void guipm_disk_clear(struct disks_info*di,bool ui){
	if(!di)return;
	if(ui){
		lv_obj_set_enabled(di->btn_ok,false);
		if(di->disks_info)lv_obj_del(di->disks_info);
	}
	di->disks_info=NULL,di->selected=NULL;
	for(int i=0;i<32;i++){
		if(!di->disks[i].enable)continue;
		if(ui)lv_obj_del(di->disks[i].btn);
		if(di->disks[i].ctx)fdisk_unref_context(di->disks[i].ctx);
		close(di->disks[i].sysfs_fd);
		memset(&di->disks[i],0,sizeof(struct disks_disk_info));
	}
}

static void guipm_set_disks_info(struct disks_info*di,char*text){
	if(!di)return;
	guipm_disk_clear(di,true);
	di->disks_info=lv_label_create(di->lst);
	lv_label_set_long_mode(di->disks_info,LV_LABEL_LONG_WRAP);
	lv_obj_set_size(di->disks_info,lv_obj_get_width(di->lst),gui_sh/16);
	lv_obj_set_style_text_align(di->disks_info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(di->disks_info,text);
}

static long get_block_size(struct disks_disk_info*k){
	char xsize[32]={0};
	errno=0;
	int xs=openat(k->sysfs_fd,"size",O_RDONLY);
	if(xs>=0){
		read(xs,xsize,31);
		close(xs);
	}
	if(xsize[0]==0)return terlog_warn(
		-1,
		"cannot read block %s size",
		k->name
	);
	return k->size=parse_long(xsize,0);
}

static int get_block_path(struct disks_disk_info*k){
	char path[BUFSIZ]={0};
	snprintf(
		path,sizeof(path)-1,
		_PATH_DEV"/%s",
		k->name
	);
	if(!is_block(path))snprintf(
		path,sizeof(path)-1,
		_PATH_DEV"/block/%s",
		k->name
	);
	errno=0;
	if(!is_block(path))return terlog_warn(
		-1,
		"cannot find block %s real path",
		k->name
	);
	strcpy(k->path,path);
	return 0;
}

static int get_fdisk_ctx(struct disks_disk_info*k){
	errno=0;
	if(!(k->ctx=fdisk_new_context()))
		return terlog_error(-1,"failed to initialize fdisk context");
	errno=0;
	if(fdisk_assign_device(k->ctx,k->path,true)!=0){
		telog_warn("failed assign block %s to fdisk context",k->name);
		fdisk_unref_context(k->ctx);
		k->ctx=NULL,k->lbl=NULL;
		return -1;
	}
	k->lbl=fdisk_get_label(k->ctx,NULL);
	if(fdisk_has_label(k->ctx)&&k->lbl){
		strncpy(k->layout,fdisk_label_get_name(k->lbl),15);
		strtoupper(k->layout);
	}
	return 0;
}

static int get_block_model(struct disks_disk_info*k){
	char model[511]={0},vendor[511]={0};
	int xm;
	if((xm=openat(k->sysfs_fd,"device/vendor",O_RDONLY))>=0){
		read(xm,&vendor,sizeof(vendor)-1);
		trim(vendor);
		close(xm);
	}
	if((xm=openat(k->sysfs_fd,"device/model",O_RDONLY))>=0){
		read(xm,&model,sizeof(model)-1);
		trim(model);
		close(xm);
	}
	strcat(k->model,vendor);
	strcat(k->model," ");
	strcat(k->model,model);
	trim(k->model);
	if(k->model[0])tlog_info(
		"block %s model name: %s",
		k->name,
		k->model
	);
	errno=0;
	return 0;
}

static void disks_add_item(struct disks_disk_info*k){
	char buf[64];
	static lv_coord_t grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	},grid_col[]={
		LV_GRID_FR(2),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST,
	};

	// disk select button
	k->btn=lv_btn_create(k->di->lst);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_size(k->btn,lv_pct(100),gui_font_size*5);
	lv_obj_set_grid_dsc_array(k->btn,grid_col,grid_row);
	lv_obj_add_event_cb(k->btn,disk_click,LV_EVENT_CLICKED,k->di);
	lv_group_add_obj(gui_grp,k->btn);

	// disk name
	lv_obj_t*lbl=lv_label_create(k->btn);
	lv_label_set_text(lbl,k->name);
	lv_obj_set_grid_cell(
		lbl,
		LV_GRID_ALIGN_START,0,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;

	// disk model name
	lv_obj_t*d_model=lv_label_create(k->btn);
	lv_label_set_long_mode(d_model,lm);
	lv_obj_set_style_text_align(d_model,LV_TEXT_ALIGN_LEFT,0);
	lv_label_set_text(d_model,_(get_model(k)));
	lv_obj_set_grid_cell(
		d_model,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_CENTER,1,1
	);

	// disk size
	lv_obj_t*d_size=lv_label_create(k->btn);
	lv_label_set_long_mode(d_size,lm);
	lv_obj_set_style_text_align(d_size,LV_TEXT_ALIGN_RIGHT,0);
	lv_label_set_text(d_size,make_readable_str_buf(
		buf,sizeof(buf),k->size,512,0
	));
	lv_obj_set_grid_cell(
		d_size,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_CENTER,0,1
	);

	// disk layout type
	lv_obj_t*d_layout=lv_label_create(k->btn);
	lv_label_set_long_mode(d_layout,lm);
	lv_obj_set_style_text_align(d_layout,LV_TEXT_ALIGN_RIGHT,0);
	lv_label_set_text(d_layout,_(get_layout(k)));
	lv_obj_set_grid_cell(
		d_layout,
		LV_GRID_ALIGN_STRETCH,1,1,
		LV_GRID_ALIGN_CENTER,1,1
	);
}

static void guipm_disk_reload(struct disks_info*di){
	guipm_disk_clear(di,true);
	int i;
	DIR*d;
	if(
		(i=open(_PATH_SYS_BLOCK,O_DIR))<0||
		!(d=fdopendir(i))
	){
		telog_error("open "_PATH_SYS_BLOCK);
		if(i>=0)close(i);
		guipm_set_disks_info(di,_("Initialize disks scanner failed"));
		return;
	}
	int blk=0;
	struct dirent*e;
	while((e=readdir(d))){
		struct disks_disk_info*k=&di->disks[blk];
		if(e->d_type!=DT_LNK)continue;
		memset(k,0,sizeof(struct disks_disk_info));
		strcpy(k->name,e->d_name);
		errno=0;
		if((k->sysfs_fd=openat(i,k->name,O_DIR))<0){
			telog_warn("cannot open block %s",k->name);
			continue;
		}
		if(
			get_block_size(k)<0||
			get_block_path(k)<0||
			(!di->is_show_all&&(
			     strncmp(k->name,"dm",2)==0||
			     strncmp(k->name,"fd",2)==0||
			     strncmp(k->name,"nbd",3)==0||
			     strncmp(k->name,"mtd",3)==0||
			     strncmp(k->name,"aoe",3)==0||
			     strncmp(k->name,"ram",3)==0||
			     strncmp(k->name,"zram",4)==0||
			     strncmp(k->name,"loop",4)==0||
			     k->size<=0
		     ))
		){
			close(k->sysfs_fd);
			continue;
		}
		k->enable=true,k->di=di;

		get_fdisk_ctx(k);
		get_block_model(k);
		disks_add_item(k);

		tlog_debug("scan block device %s (%s)",k->path,get_layout(k));

		if(++blk>=32){
			tlog_warn("disk too many, only show 31 disks");
			break;
		}
	}
	tlog_info("found %d disks",blk);
	free(d);
	close(i);
}

static void refresh_click(lv_event_t*e){
	struct disks_info*di=e->user_data;
	tlog_debug("request refresh");
	guipm_disk_reload(di);
}

static void ok_click(lv_event_t*e){
	struct disks_info*di=e->user_data;
	if(!di->selected)return;
	tlog_debug("ok clicked");
	guiact_start_activity_by_name("guipm-partitions",di->selected->name);
}

static void cancel_click(lv_event_t*e __attribute__((unused))){
	tlog_debug("cancel clicked");
	guiact_do_back();
}

static void show_all_click(lv_event_t*e){
	struct disks_info*di=e->user_data;
	di->is_show_all=lv_obj_is_checked(di->show_all);
	tlog_debug("request show all %s",BOOL2STR(di->is_show_all));
	guipm_disk_reload(di);
}

static int do_cleanup(struct gui_activity*d){
	struct disks_info*di=d->data;
	if(!di)return 0;
	guipm_disk_clear(di,false);
	di->is_show_all=false;
	free(di);
	d->data=NULL;
	return 0;
}

static int do_reload(struct gui_activity*d){
	struct disks_info*di=d->data;
	if(di)guipm_disk_reload(di);
	return 0;
}

static int guipm_disk_get_focus(struct gui_activity*d){
	struct disks_info*di=d->data;
	if(!di)return -1;
	lv_group_add_obj(gui_grp,di->show_all);
	lv_group_add_obj(gui_grp,di->btn_ok);
	lv_group_add_obj(gui_grp,di->btn_refresh);
	lv_group_add_obj(gui_grp,di->btn_cancel);
	return 0;
}

static int guipm_disk_lost_focus(struct gui_activity*d){
	struct disks_info*di=d->data;
	if(!di)return 0;
	for(int i=0;i<32;i++){
		if(!di->disks[i].enable)continue;
		lv_group_remove_obj(di->disks[i].btn);
	}
	lv_group_remove_obj(di->show_all);
	lv_group_remove_obj(di->btn_ok);
	lv_group_remove_obj(di->btn_refresh);
	lv_group_remove_obj(di->btn_cancel);
	return 0;
}
static int guipm_init(struct gui_activity*act){
	struct disks_info*di=malloc(sizeof(struct disks_info));
	if(!di)return -ENOMEM;
	memset(di,0,sizeof(struct disks_info));
	act->data=di;
	return 0;
}

static int guipm_draw_disk_sel(struct gui_activity*act){
	struct disks_info*di=act->data;
	lv_obj_set_style_pad_all(act->page,gui_font_size/2,0);
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	guipm_draw_title(act->page);

	// function title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("Select a disk to process"));

	// disk list
	di->lst=lv_obj_create(act->page);
	lv_obj_set_width(di->lst,lv_pct(100));
	lv_obj_set_flex_flow(di->lst,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(di->lst,1);

	// show all checkbox
	di->show_all=lv_draw_checkbox(act->page,"Show all blocks",false,show_all_click,di);

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,title,x)&(struct button_dsc){\
			&di->btn_##tgt,en,_(title),\
			tgt##_click,di,x,1,0,1,NULL\
		}
		BTN(ok,      false, "OK",      0),
		BTN(refresh, true,  "Refresh", 1),
		BTN(cancel,  true,  "Cancel",  2),
		#undef BTN
		NULL
	);
	return 0;
}

struct gui_register guireg_guipm_disk_select={
	.name="guipm-disk-select",
	.title="Partition Manager",
	.show_app=true,
	.quiet_exit=do_cleanup,
	.init=guipm_init,
	.get_focus=guipm_disk_get_focus,
	.lost_focus=guipm_disk_lost_focus,
	.draw=guipm_draw_disk_sel,
	.data_load=do_reload,
	.back=true
};
#endif

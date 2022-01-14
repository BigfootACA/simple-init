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
#include"gui/tools.h"
#include"gui/activity.h"
#define TAG "guipm"

static char*get_model(struct disks_disk_info*d){return d->model[0]==0?"Unknown":d->model;}
static char*get_layout(struct disks_disk_info*d){return d->layout[0]==0?"Unknown":d->layout;}

static void disk_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct disks_info*di=lv_obj_get_user_data(obj);
	if(!di)return;
	lv_checkbox_set_checked(obj,true);
	if(di->selected){
		if(obj==di->selected->chk)return;
		else{
			lv_checkbox_set_checked(di->selected->chk,false);
			lv_obj_set_checked(di->selected->btn,false);
		}
	}
	di->selected=NULL;
	for(int i=0;i<32&&!di->selected;i++)
		if(di->disks[i].enable&&di->disks[i].chk==obj)
			di->selected=&di->disks[i];
	if(!di->selected)return;
	lv_obj_set_checked(di->selected->btn,true);
	tlog_debug("selected disk %s",di->selected->name);
	lv_obj_set_enabled(di->btn_ok,true);
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
	di->disks_info=lv_label_create(di->lst,NULL);
	lv_label_set_long_mode(di->disks_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(di->disks_info,lv_page_get_scrl_width(di->lst),gui_sh/16);
	lv_label_set_align(di->disks_info,LV_LABEL_ALIGN_CENTER);
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

static void disks_add_item(int blk,struct disks_disk_info*k){
	lv_coord_t c1w,c2w,c1l,c2l,bw;
	bw=lv_page_get_scrl_width(k->di->lst);

	// disk select button
	k->btn=lv_btn_create(k->di->lst,NULL);
	lv_obj_set_y(k->btn,(gui_dpi/2+32)*blk);
	lv_obj_set_size(k->btn,bw,gui_dpi/2);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);

	// line for button text
	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	c1l=16,c2w=(bw-c1l)/4;
	c2l=c2w*3,c1w=c2l-(c1l*2);

	// disk name and checkbox
	k->chk=lv_checkbox_create(line,NULL);
	lv_obj_align(k->chk,NULL,LV_ALIGN_IN_LEFT_MID,c1l,-(gui_dpi/10));
	lv_obj_set_width(k->chk,c1w);
	lv_checkbox_set_text(k->chk,k->name);
	lv_obj_set_user_data(k->chk,k->di);
	lv_obj_set_event_cb(k->chk,disk_click);
	lv_style_set_focus_checkbox(k->chk);
	lv_group_add_obj(gui_grp,k->chk);

	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT;

	// disk model name
	lv_obj_t*d_model=lv_label_create(line,NULL);
	lv_obj_align(d_model,NULL,LV_ALIGN_IN_LEFT_MID,c1l,(gui_dpi/10));
	lv_label_set_long_mode(d_model,lm);
	lv_obj_set_width(d_model,c1w);
	lv_label_set_align(d_model,LV_LABEL_ALIGN_LEFT);
	lv_label_set_text(d_model,_(get_model(k)));
	if(!k->model[0])lv_obj_set_gray240_text_color(d_model,LV_LABEL_PART_MAIN);

	// disk size
	lv_obj_t*d_size=lv_label_create(line,NULL);
	lv_obj_align(d_size,NULL,LV_ALIGN_IN_LEFT_MID,c2l,-(gui_dpi/10));
	lv_label_set_long_mode(d_size,lm);
	lv_obj_set_width(d_size,c2w);
	lv_label_set_align(d_size,LV_LABEL_ALIGN_RIGHT);
	const char*ss=make_readable_str(k->size,512,0);
	lv_label_set_text(d_size,ss);
	free((char*)ss);

	// disk layout type
	lv_obj_t*d_layout=lv_label_create(line,NULL);
	lv_obj_align(d_layout,NULL,LV_ALIGN_IN_LEFT_MID,c2l,(gui_dpi/10));
	lv_label_set_long_mode(d_layout,lm);
	lv_obj_set_width(d_layout,c2w);
	lv_label_set_align(d_layout,LV_LABEL_ALIGN_RIGHT);
	lv_label_set_text(d_layout,_(get_layout(k)));
	if(!k->lbl)lv_obj_set_gray240_text_color(d_layout,LV_LABEL_PART_MAIN);
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
		disks_add_item(blk,k);

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

static void refresh_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct disks_info*di=lv_obj_get_user_data(obj);
	if(!di||obj!=di->btn_refresh)return;
	tlog_debug("request refresh");
	guipm_disk_reload(di);
}

static void ok_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct disks_info*di=lv_obj_get_user_data(obj);
	if(!di||obj!=di->btn_ok||!di->selected)return;
	tlog_debug("ok clicked");
	guiact_start_activity_by_name("guipm-partitions",di->selected->name);
}

static void cancel_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct disks_info*di=lv_obj_get_user_data(obj);
	if(!di||obj!=di->btn_cancel)return;
	tlog_debug("cancel clicked");
	guiact_do_back();
}

static void show_all_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct disks_info*di=lv_obj_get_user_data(obj);
	if(!di||obj!=di->show_all)return;
	di->is_show_all=lv_checkbox_is_checked(obj);
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

static void do_reload(lv_task_t*t){
	struct disks_info*di=t->user_data;
	if(!di)return;
	guipm_disk_reload(di);
	lv_group_add_obj(gui_grp,di->show_all);
	lv_group_add_obj(gui_grp,di->btn_ok);
	lv_group_add_obj(gui_grp,di->btn_refresh);
	lv_group_add_obj(gui_grp,di->btn_cancel);
}

static int guipm_disk_get_focus(struct gui_activity*d){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,d->data));
	return 0;
}

static int guipm_disk_lost_focus(struct gui_activity*d){
	struct disks_info*di=d->data;
	if(!di)return 0;
	for(int i=0;i<32;i++){
		if(!di->disks[i].enable)continue;
		lv_group_remove_obj(di->disks[i].chk);
	}
	lv_group_remove_obj(di->show_all);
	lv_group_remove_obj(di->btn_ok);
	lv_group_remove_obj(di->btn_refresh);
	lv_group_remove_obj(di->btn_cancel);
	return 0;
}

static int guipm_draw_disk_sel(struct gui_activity*act){

	lv_coord_t mar=(gui_dpi/20);
	lv_coord_t btw=gui_sw/3-gui_dpi/5;
	lv_coord_t bth=gui_font_size+gui_dpi/10;
	lv_coord_t btt=gui_sh-bth-gui_dpi/10;
	struct disks_info*di=malloc(sizeof(struct disks_info));
	if(!di)return -ENOMEM;
	memset(di,0,sizeof(struct disks_info));
	act->data=di;

	guipm_draw_title(act->page);

	// function title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_sh/16);
	lv_obj_set_size(title,gui_sw,gui_sh/16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("Select a disk to process"));

	// disk list
	static lv_style_t lst_style;
	lv_style_init(&lst_style);
	lv_style_set_border_width(&lst_style,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&lst_style,LV_STATE_FOCUSED,0);
	lv_style_set_border_width(&lst_style,LV_STATE_PRESSED,0);
	di->lst=lv_page_create(act->page,NULL);
	lv_obj_add_style(di->lst,LV_PAGE_PART_BG,&lst_style);
	lv_obj_set_size(di->lst,gui_sw-gui_dpi/10,gui_sh-(gui_sh/16*2)-(bth*2)-(gui_dpi/10*3));
	lv_obj_set_pos(di->lst,mar,gui_sh/16*2);

	// show all checkbox
	di->show_all=lv_checkbox_create(act->page,NULL);
	lv_obj_set_pos(di->show_all,gui_dpi/10,gui_sh-(bth*2)-(gui_dpi/5));
	lv_obj_set_size(di->show_all,gui_sw/3-(gui_dpi/5),bth);
	lv_obj_set_user_data(di->show_all,di);
	lv_obj_set_event_cb(di->show_all,show_all_click);
	lv_style_set_focus_checkbox(di->show_all);
	lv_checkbox_set_text(di->show_all,_("Show all blocks"));

	// ok button
	di->btn_ok=lv_btn_create(act->page,NULL);
	lv_obj_set_pos(di->btn_ok,gui_dpi/10,btt);
	lv_obj_set_size(di->btn_ok,btw,bth);
	lv_style_set_action_button(di->btn_ok,false);
	lv_obj_set_user_data(di->btn_ok,di);
	lv_obj_set_event_cb(di->btn_ok,ok_click);
	lv_label_set_text(lv_label_create(di->btn_ok,NULL),_("OK"));

	// refresh button
	di->btn_refresh=lv_btn_create(act->page,NULL);
	lv_obj_set_pos(di->btn_refresh,gui_sw/3+(gui_dpi/10),btt);
	lv_obj_set_size(di->btn_refresh,btw,bth);
	lv_style_set_action_button(di->btn_refresh,true);
	lv_obj_set_user_data(di->btn_refresh,di);
	lv_obj_set_event_cb(di->btn_refresh,refresh_click);
	lv_label_set_text(lv_label_create(di->btn_refresh,NULL),_("Refresh"));

	// cancel button
	di->btn_cancel=lv_btn_create(act->page,NULL);
	lv_obj_set_pos(di->btn_cancel,gui_sw/3*2+(gui_dpi/10),btt);
	lv_obj_set_size(di->btn_cancel,btw,bth);
	lv_style_set_action_button(di->btn_cancel,true);
	lv_obj_set_user_data(di->btn_cancel,di);
	lv_obj_set_event_cb(di->btn_cancel,cancel_click);
	lv_label_set_text(lv_label_create(di->btn_cancel,NULL),_("Cancel"));
	return 0;
}

struct gui_register guireg_guipm_disk_select={
	.name="guipm-disk-select",
	.title="Partition Manager",
	.icon="guipm.svg",
	.show_app=true,
	.quiet_exit=do_cleanup,
	.get_focus=guipm_disk_get_focus,
	.lost_focus=guipm_disk_lost_focus,
	.draw=guipm_draw_disk_sel,
	.back=true
};
#endif
#endif

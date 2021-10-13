#ifdef ENABLE_GUI
#ifdef ENABLE_FDISK
#define _GNU_SOURCE
#include<stdlib.h>
#include<libfdisk/libfdisk.h>
#include"gui.h"
#include"str.h"
#include"system.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/activity.h"
#define TAG "guipm"

static int xdpi;
static bool is_show_all=false;
static  lv_obj_t*lst=NULL,*selscr=NULL;
static lv_obj_t*btn_ok,*btn_refresh,*btn_cancel;
static lv_obj_t*disks_info=NULL,*show_all=NULL;
static struct disk_info{
	bool enable;
	lv_obj_t*btn,*chk;
	struct fdisk_context*ctx;
	struct fdisk_label*lbl;
	long size;
	char name[256];
	char model[BUFSIZ];
	char path[BUFSIZ];
	char layout[16];
	int sysfs_fd;
}disks[32]={0};
static struct disk_info*selected=NULL;

extern void guipm_draw_title(lv_obj_t*screen);

static char*get_model(struct disk_info*d){return d->model[0]==0?"Unknown":d->model;}
static char*get_layout(struct disk_info*d){return d->layout[0]==0?"Unknown":d->layout;}

void disk_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected){
		if(obj==selected->chk)return;
		else{
			lv_checkbox_set_checked(selected->chk,false);
			lv_obj_set_checked(selected->btn,false);
		}
	}
	selected=NULL;
	for(int i=0;i<32&&!selected;i++)
		if(disks[i].enable&&disks[i].chk==obj)
			selected=&disks[i];
	if(!selected)return;
	lv_obj_set_checked(selected->btn,true);
	tlog_debug("selected disk %s",selected->name);
	lv_obj_set_enabled(btn_ok,true);
}

void guipm_disk_clear(){
	selected=NULL;
	lv_obj_set_enabled(btn_ok,false);
	if(disks_info)lv_obj_del(disks_info);
	disks_info=NULL;
	for(int i=0;i<32;i++){
		if(!disks[i].enable)continue;
		lv_obj_del(disks[i].btn);
		if(disks[i].ctx)fdisk_unref_context(disks[i].ctx);
		close(disks[i].sysfs_fd);
		memset(&disks[i],0,sizeof(struct disk_info));
	}
}

void guipm_set_disks_info(char*text){
	guipm_disk_clear();
	disks_info=lv_label_create(lst,NULL);
	lv_label_set_long_mode(disks_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(disks_info,lv_page_get_scrl_width(lst),gui_sh/16);
	lv_label_set_align(disks_info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(disks_info,text);
}

static long get_block_size(struct disk_info*k){
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

static int get_block_path(struct disk_info*k){
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

static int get_fdisk_ctx(struct disk_info*k){
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

static int get_block_model(struct disk_info*k){
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

static void disks_add_item(int blk,struct disk_info*k){
	lv_coord_t c1w,c2w,c1l,c2l,bw;
	bw=lv_page_get_scrl_width(lst);

	// disk select button
	k->btn=lv_btn_create(lst,NULL);
	lv_obj_set_y(k->btn,(xdpi*5+32)*blk);
	lv_obj_set_size(k->btn,bw,xdpi*5);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);

	// line for button text
	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	c1l=16,c2w=(bw-c1l)/4;
	c2l=c2w*3,c1w=c2l-(c1l*2);

	// disk name and checkbox
	k->chk=lv_checkbox_create(line,NULL);
	lv_obj_align(k->chk,NULL,LV_ALIGN_IN_LEFT_MID,c1l,-xdpi);
	lv_obj_set_width(k->chk,c1w);
	lv_checkbox_set_text(k->chk,k->name);
	lv_obj_set_event_cb(k->chk,disk_click);
	lv_style_set_focus_checkbox(k->chk);
	lv_group_add_obj(gui_grp,k->chk);

	// disk model name
	lv_obj_t*d_model=lv_label_create(line,NULL);
	lv_obj_align(d_model,NULL,LV_ALIGN_IN_LEFT_MID,c1l,xdpi);
	lv_label_set_long_mode(d_model,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(d_model,c1w);
	lv_label_set_align(d_model,LV_LABEL_ALIGN_LEFT);
	lv_label_set_text(d_model,_(get_model(k)));
	if(!k->model[0])lv_obj_set_gray240_text_color(d_model,LV_LABEL_PART_MAIN);

	// disk size
	lv_obj_t*d_size=lv_label_create(line,NULL);
	lv_obj_align(d_size,NULL,LV_ALIGN_IN_LEFT_MID,c2l,-xdpi);
	lv_label_set_long_mode(d_size,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(d_size,c2w);
	lv_label_set_align(d_size,LV_LABEL_ALIGN_RIGHT);
	lv_label_set_text(d_size,make_readable_str(k->size,512,0));

	// disk layout type
	lv_obj_t*d_layout=lv_label_create(line,NULL);
	lv_obj_align(d_layout,NULL,LV_ALIGN_IN_LEFT_MID,c2l,xdpi);
	lv_label_set_long_mode(d_layout,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(d_layout,c2w);
	lv_label_set_align(d_layout,LV_LABEL_ALIGN_RIGHT);
	lv_label_set_text(d_layout,_(get_layout(k)));
	if(!k->lbl)lv_obj_set_gray240_text_color(d_layout,LV_LABEL_PART_MAIN);
}

void guipm_disk_reload(){
	guipm_disk_clear();
	int i;
	DIR*d;
	if(
		(i=open(_PATH_SYS_BLOCK,O_DIR))<0||
		!(d=fdopendir(i))
	){
		telog_error("open "_PATH_SYS_BLOCK);
		if(i>=0)close(i);
		guipm_set_disks_info(_("Initialize disks scanner failed"));
		return;
	}
	int blk=0;
	struct dirent*e;
	while((e=readdir(d))){
		struct disk_info*k=&disks[blk];
		if(e->d_type!=DT_LNK)continue;
		memset(k,0,sizeof(struct disk_info));
		strcpy(k->name,e->d_name);
		errno=0;
		if((k->sysfs_fd=openat(i,k->name,O_DIR))<0){
			telog_warn("cannot open block %s",k->name);
			continue;
		}
		if(
			get_block_size(k)<0||
			get_block_path(k)<0||
			(!is_show_all&&(
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
		k->enable=true;

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
	if(e!=LV_EVENT_CLICKED||obj!=btn_refresh)return;
	tlog_debug("request refresh");
	guipm_disk_reload();
}

static void ok_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_ok||!selected)return;
	tlog_debug("ok clicked");
	guiact_start_activity_by_name("guipm-partitions",selected->name);
}

static void cancel_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_cancel)return;
	tlog_debug("cancel clicked");
	guiact_do_back();
}

static void show_all_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED||obj!=show_all)return;
	is_show_all=lv_checkbox_is_checked(obj);
	tlog_debug("request show all %s",BOOL2STR(is_show_all));
	guipm_disk_reload();
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	guipm_disk_clear();
	is_show_all=false;
	return 0;
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	guipm_disk_reload();
	lv_group_add_obj(gui_grp,show_all);
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,btn_refresh);
	lv_group_add_obj(gui_grp,btn_cancel);
}

static int guipm_disk_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int guipm_disk_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<32;i++){
		if(!disks[i].enable)continue;
		lv_group_remove_obj(disks[i].chk);
	}
	lv_group_remove_obj(show_all);
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(btn_refresh);
	lv_group_remove_obj(btn_cancel);
	return 0;
}

static int guipm_draw_disk_sel(struct gui_activity*act){

	xdpi=gui_dpi/10;
	int mar=(xdpi/2),btw=gui_sw/3-(xdpi*2),bth=gui_font_size+xdpi,btt=gui_sh-bth-xdpi;
	selscr=act->page;

	guipm_draw_title(selscr);

	// function title
	lv_obj_t*title=lv_label_create(selscr,NULL);
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
	lst=lv_page_create(selscr,NULL);
	lv_obj_add_style(lst,LV_PAGE_PART_BG,&lst_style);
	lv_obj_set_size(lst,gui_sw-xdpi,gui_sh-(gui_sh/16*2)-(bth*2)-(xdpi*3));
	lv_obj_set_pos(lst,mar,gui_sh/16*2);

	// show all checkbox
	static lv_style_t chk_style,bul_style;
	lv_color_t pri=lv_theme_get_color_primary();
	lv_color_t bul=lv_color_lighten(pri,LV_OPA_80);
	lv_style_init(&chk_style);
	lv_style_set_outline_width(&chk_style,LV_STATE_DEFAULT,0);
	lv_style_set_outline_width(&chk_style,LV_STATE_FOCUSED,0);
	lv_style_init(&bul_style);
	lv_style_set_bg_color(&bul_style,LV_STATE_FOCUSED,bul);
	lv_style_set_bg_color(&bul_style,LV_STATE_FOCUSED|LV_STATE_CHECKED,pri);
	show_all=lv_checkbox_create(selscr,NULL);
	lv_obj_set_pos(show_all,xdpi,gui_sh-(bth*2)-(xdpi*2));
	lv_obj_set_size(show_all,gui_sw/3-(xdpi*2),bth);
	lv_obj_add_style(show_all,LV_CHECKBOX_PART_BG,&chk_style);
	lv_obj_add_style(show_all,LV_CHECKBOX_PART_BULLET,&bul_style);
	lv_obj_set_event_cb(show_all,show_all_click);
	lv_checkbox_set_text(show_all,_("Show all blocks"));

	// ok button
	btn_ok=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_ok,xdpi,btt);
	lv_obj_set_size(btn_ok,btw,bth);
	lv_style_set_action_button(btn_ok,false);
	lv_obj_set_event_cb(btn_ok,ok_click);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	// refresh button
	btn_refresh=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_refresh,gui_sw/3+xdpi,btt);
	lv_obj_set_size(btn_refresh,btw,bth);
	lv_style_set_action_button(btn_refresh,true);
	lv_obj_set_event_cb(btn_refresh,refresh_click);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),_("Refresh"));

	// cancel button
	btn_cancel=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_cancel,gui_sw/3*2+xdpi,btt);
	lv_obj_set_size(btn_cancel,btw,bth);
	lv_style_set_action_button(btn_cancel,true);
	lv_obj_set_event_cb(btn_cancel,cancel_click);
	lv_label_set_text(lv_label_create(btn_cancel,NULL),_("Cancel"));
	return 0;
}

struct gui_register guireg_guipm_disk_select={
	.name="guipm-disk-select",
	.title="Partition Manager",
	.icon="guipm.png",
	.show_app=true,
	.quiet_exit=do_cleanup,
	.get_focus=guipm_disk_get_focus,
	.lost_focus=guipm_disk_lost_focus,
	.draw=guipm_draw_disk_sel,
	.back=true
};
#endif
#endif

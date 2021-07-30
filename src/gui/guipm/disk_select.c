#include"guipm.h"
static int xdpi;
static lv_style_t f24_style,item_style,gray_style;
static lv_obj_t*lst=NULL,*selscr=NULL;
static lv_obj_t*btn_ok,*btn_refresh,*btn_cancel;
static lv_obj_t*disks_info=NULL;
static struct disk_info{
	bool enable;
	lv_obj_t*btn;
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

static char*get_model(struct disk_info*d){return d->model[0]==0?_("Unknown"):d->model;}
static char*get_layout(struct disk_info*d){return d->layout[0]==0?_("Unknown"):d->layout;}

static void disk_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	if(selected){
		if(obj==selected->btn)return;
		else lv_obj_clear_state(selected->btn,LV_STATE_CHECKED);
	}
	selected=NULL;
	for(int i=0;i<32&&!selected;i++)
		if(disks[i].enable&&disks[i].btn==obj)
			selected=&disks[i];
	if(!selected)return;
	lv_obj_add_state(obj,LV_STATE_CHECKED);
	tlog_debug("selected disk %s",selected->name);
	lv_obj_clear_state(btn_ok,LV_STATE_DISABLED);
}

void guipm_disk_clear(){
	selected=NULL;
	lv_obj_add_state(btn_ok,LV_STATE_DISABLED);
	if(disks_info)lv_obj_del(disks_info);
	disks_info=NULL;
	for(int i=0;i<32;i++){
		if(!disks[i].enable)continue;
		lv_obj_del(disks[i].btn);
		fdisk_unref_context(disks[i].ctx);
		close(disks[i].sysfs_fd);
		memset(&disks[i],0,sizeof(struct disk_info));
	}
}

void guipm_set_disks_info(char*text){
	guipm_disk_clear();
	disks_info=lv_label_create(lst,NULL);
	lv_label_set_long_mode(disks_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(disks_info,lv_page_get_scrl_width(lst),h/16);
	lv_obj_add_style(disks_info,0,&f24_style);
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
	lv_obj_add_style(k->btn,LV_BTN_PART_MAIN,&item_style);
	lv_obj_set_event_cb(k->btn,disk_click);

	// line for button text
	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	c1l=16,c2w=(bw-c1l)/4;
	c2l=c2w*3,c1w=c2l-(c1l*2);

	// disk name
	lv_obj_t*d_name=lv_label_create(line,NULL);
	lv_obj_align(d_name,NULL,LV_ALIGN_IN_LEFT_MID,c1l,-xdpi);
	lv_label_set_long_mode(d_name,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(d_name,c1w);
	lv_label_set_align(d_name,LV_LABEL_ALIGN_LEFT);
	lv_label_set_text(d_name,k->name);

	// disk model name
	lv_obj_t*d_model=lv_label_create(line,NULL);
	lv_obj_align(d_model,NULL,LV_ALIGN_IN_LEFT_MID,c1l,xdpi);
	lv_label_set_long_mode(d_model,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(d_model,c1w);
	lv_label_set_align(d_model,LV_LABEL_ALIGN_LEFT);
	lv_label_set_text(d_model,get_model(k));
	if(!k->model[0])lv_obj_add_style(d_model,0,&gray_style);

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
	lv_label_set_text(d_layout,get_layout(k));
	if(!k->lbl)lv_obj_add_style(d_layout,0,&gray_style);
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
			get_block_size(k)<=0||
			get_block_path(k)<0||
			get_fdisk_ctx(k)<0
		){
			close(k->sysfs_fd);
			continue;
		}
		tlog_debug("scan block device %s (%s)",k->path,get_layout(k));
		k->enable=true;

		get_block_model(k);
		disks_add_item(blk,k);

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

void guipm_draw_disk_sel(lv_obj_t*screen){

	xdpi=gui_dpi/10;
	int mar=(xdpi/2);

	static lv_style_t scr_style;
	lv_style_init(&scr_style);
	lv_style_set_outline_width(&scr_style,0,0);
	selscr=lv_obj_create(screen,NULL);
	lv_obj_set_size(selscr,w,h);
	lv_theme_apply(selscr,LV_THEME_SCR);

	guipm_draw_title(selscr);

	// disk item button style
	lv_style_init(&item_style);
	lv_style_set_radius(&item_style,LV_STATE_DEFAULT,5);
	lv_color_t click=lv_color_make(240,240,240),bg=lv_color_make(64,64,64);
	lv_style_set_border_width(&item_style,LV_STATE_DEFAULT,1);
	lv_style_set_border_width(&item_style,LV_STATE_PRESSED,0);
	lv_style_set_border_width(&item_style,LV_STATE_CHECKED,0);
	lv_style_set_border_color(&item_style,LV_STATE_DEFAULT,click);
	lv_style_set_outline_width(&item_style,LV_STATE_PRESSED,0);
	lv_style_set_outline_width(&item_style,LV_STATE_FOCUSED,0);
	lv_style_set_outline_width(&item_style,LV_STATE_DEFAULT,0);
	lv_style_set_bg_color(&item_style,LV_STATE_PRESSED,click);
	lv_style_set_bg_color(&item_style,LV_STATE_CHECKED,bg);
	lv_style_set_bg_color(&item_style,LV_STATE_CHECKED|LV_STATE_PRESSED,bg);

	// gray label
	lv_style_init(&gray_style);
	lv_style_set_text_color(&gray_style,0,lv_color_make(225,225,225));

	// function title
	lv_obj_t*title=lv_label_create(selscr,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,h/16);
	lv_obj_set_size(title,w,h/16);
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
	lv_obj_set_size(lst,w-xdpi,h/16*12);
	lv_obj_set_pos(lst,mar,h/16*2);

	// button style
	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
	lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);
	lv_style_set_outline_width(&btn_style,LV_STATE_FOCUSED,0);

	int btw=w/3-(xdpi*2),bth=h/16,btt=h-bth-xdpi;

	// ok button
	btn_ok=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_ok,xdpi,btt);
	lv_obj_set_size(btn_ok,btw,bth);
	lv_obj_add_style(btn_ok,0,&btn_style);
	lv_obj_add_state(btn_ok,LV_STATE_CHECKED|LV_STATE_DISABLED);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	// refresh button
	btn_refresh=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_refresh,w/3+xdpi,btt);
	lv_obj_set_size(btn_refresh,btw,bth);
	lv_obj_add_style(btn_refresh,0,&btn_style);
	lv_obj_add_state(btn_refresh,LV_STATE_CHECKED);
	lv_obj_set_event_cb(btn_refresh,refresh_click);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),_("Refresh"));

	// cancel button
	btn_cancel=lv_btn_create(selscr,NULL);
	lv_obj_set_pos(btn_cancel,w/3*2+xdpi,btt);
	lv_obj_set_size(btn_cancel,btw,bth);
	lv_obj_add_style(btn_cancel,0,&btn_style);
	lv_obj_add_state(btn_cancel,LV_STATE_CHECKED);
	lv_label_set_text(lv_label_create(btn_cancel,NULL),_("Cancel"));

	guipm_disk_reload();
}

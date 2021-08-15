#define _GNU_SOURCE
#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include"list.h"
#include"lvgl.h"
#include"gui.h"
#include"str.h"
#include"tools.h"
#include"logger.h"
#include"system.h"
#include"filetab.h"
#include"fileview.h"
#define TAG "fileview"
#define MIME_DIR _PATH_USR"/share/pixmaps/mime"
#define MIME_EXT ".png"

struct fileview{
	char path[PATH_MAX],old_path[PATH_MAX];
	lv_obj_t*view,*info,*last_btn;
	list*items;
	bool hidden,parent;
	int fd,count;
	lv_coord_t bw,bh;
	lv_group_t*grp;
	fileview_on_item_click on_click_item;
	fileview_on_change_dir on_change_dir;
	void*data;
};

struct fileitem{
	struct fileview*view;
	lv_obj_t*btn,*chk,*img;
	lv_obj_t*size,*info1,*info2;
	unsigned char type;
	char name[256];
	struct stat st;
};

static const char*get_icon(struct fileitem*fi){
	switch(fi->type){
		case DT_DIR:return strcmp(fi->name,"..")==0?"inode-parent":"inode-dir";
		case DT_REG:return "inode-file";
		case DT_LNK:return "inode-symlink";
		case DT_BLK:return "inode-blockdevice";
		case DT_CHR:return "inode-chardevice";
		case DT_FIFO:return "inode-fifo";
		case DT_SOCK:return "inode-socket";
		default:return "unknown";
	}
}

static void call_on_change_dir(struct fileview*view){
	if(strcmp(view->path,view->old_path)==0)return;
	tlog_debug("change dir to %s",view->path);
	if(view->on_change_dir)view->on_change_dir(view,view->old_path,view->path);
}

static bool call_on_click_item(struct fileitem*fi){
	if(!fi->view||!fi->view->on_click_item)return true;
	return fi->view->on_click_item(fi->view,fi->name);
}

static void update_fd(struct fileview*fv,int fd){
	close(fv->fd);
	fv->fd=fd;
	strncpy(fv->old_path,fv->path,sizeof(fv->path));
	get_fd_path(fd,fv->path,sizeof(fv->path)-1);
	call_on_change_dir(fv);
	fileview_set_path(fv,NULL);
}

void fileview_go_back(struct fileview*fv){
	int fd;
	if(fileview_is_top(fv))return;
	if((fd=openat(fv->fd,"..",O_DIR|O_CLOEXEC))<0){
		telog_warn("open dir parent folder failed");
		if((fd=open(fv->old_path,O_DIR|O_CLOEXEC))<0){
			telog_warn("open old dir %s failed",fv->old_path);
			return;
		}
	}
	update_fd(fv,fd);
}

static void item_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct fileitem*fi=(struct fileitem*)lv_obj_get_user_data(obj);
	if(!fi)return;
	struct stat st;
	struct fileview*fv=fi->view;
	if(!fv||!call_on_click_item(fi))return;
	switch(fi->type){
		case DT_LNK:
			if(fstatat(fv->fd,fi->name,&st,0)<0||!S_ISDIR(st.st_mode))return;
		break;
		case DT_DIR:break;
		default:return;
	}
	int fd=openat(fv->fd,fi->name,O_DIR|O_CLOEXEC);
	if(fd<0){
		telog_warn("open dir %s/%s failed",fv->path,fi->name);
		if(strcmp(fi->name,"..")!=0||!fv->old_path[0])return;
		tlog_debug("try old dir %s",fv->old_path);
		fileview_go_back(fv);
		return;
	}
	update_fd(fv,fd);
}

static void add_item(struct fileview*view,char*name,unsigned char type){
	if(!view->view||!name||view->fd<0||!type)return;
	if(view->bw==0)view->bw=lv_page_get_scrl_width(view->view)-gui_font_size;
	if(view->bh==0)view->bh=gui_dpi/2;
	struct fileitem*fi=malloc(sizeof(struct fileitem));
	if(!fi){
		telog_error("cannot allocate fileitem");
		abort();
	}
	memset(fi,0,sizeof(struct fileitem));
	fi->type=type,fi->view=view;
	strncpy(fi->name,name,sizeof(fi->name)-1);

	// file item button
	fi->btn=lv_btn_create(view->view,NULL);
	lv_obj_set_size(fi->btn,view->bw,view->bh);
	lv_style_set_btn_item(fi->btn);
	lv_obj_set_click(fi->btn,false);
	lv_obj_align(
		fi->btn,view->last_btn,view->last_btn?
			LV_ALIGN_OUT_BOTTOM_LEFT:
			LV_ALIGN_IN_TOP_MID,
		0,gui_font_size/2
	);
	view->last_btn=fi->btn;
	list*l=list_new(fi);
	if(!l){
		telog_error("cannot allocate fileitem list");
		abort();
	}
	if(!view->items)view->items=l;
	else list_push(view->items,l);

	// line for button text
	lv_obj_t*line=lv_line_create(fi->btn,NULL);
	lv_obj_set_width(line,view->bw);

	static lv_style_t img_s;
	lv_style_init(&img_s);
	lv_style_set_outline_width(&img_s,LV_STATE_FOCUSED,gui_dpi/100);
	lv_style_set_outline_color(&img_s,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_style_set_radius(&img_s,LV_STATE_FOCUSED,gui_dpi/50);

	// file image
	lv_coord_t si=view->bh-gui_font_size;
	fi->img=lv_img_create(line,NULL);
	lv_obj_set_size(fi->img,si,si);
	lv_obj_align(fi->img,NULL,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
	lv_obj_set_click(fi->img,true);
	lv_obj_set_event_cb(fi->img,item_click);
	lv_obj_set_user_data(fi->img,fi);
	lv_obj_add_style(fi->img,LV_IMG_PART_MAIN,&img_s);
	char path[PATH_MAX];
	memset(path,0,PATH_MAX);
	snprintf(path,PATH_MAX-1,MIME_DIR"/%s"MIME_EXT,get_icon(fi));
	lv_img_set_src(fi->img,path);
	lv_img_ext_t*x=lv_obj_get_ext_attr(fi->img);
	if((x->w<=0||x->h<=0)){
		memset(path,0,PATH_MAX);
		strncpy(path,MIME_DIR"/inode-file"MIME_EXT,PATH_MAX-1);
		lv_img_set_src(fi->img,path);
		x=lv_obj_get_ext_attr(fi->img);
	}
	if(x->w>0&&x->h>0)lv_img_set_zoom(fi->img,(int)(((float)si/MAX(x->w,x->h))*256));

	// file name and checkbox
	fi->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(fi->chk,strcmp(name,"..")==0?_("Parent folder"):name);
	lv_style_set_focus_checkbox(fi->chk);
	lv_obj_align(fi->chk,fi->img,LV_ALIGN_OUT_RIGHT_TOP,gui_font_size/2,0);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(fi->chk);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_SROLL_CIRC);

	// add group
	if(view->grp){
		lv_group_add_obj(view->grp,fi->img);
		lv_group_add_obj(view->grp,fi->chk);
	}

	if(fstatat(view->fd,name,&fi->st,AT_SYMLINK_NOFOLLOW)==0){
		mode_t m=fi->st.st_mode;
		dev_t d=fi->st.st_dev;
		char dev[32]={0},times[64]={0};
		strftime(times,63,"%Y/%m/%d %H:%M:%S",localtime(&fi->st.st_mtime));
		if(S_ISCHR(m))snprintf(dev,31,"char[%d:%d] ",major(d),minor(d));
		if(S_ISBLK(m))snprintf(dev,31,"block[%d:%d] ",major(d),minor(d));

		// file size
		if(S_ISREG(m)){
			char size[32]={0};
			fi->size=lv_label_create(line,NULL);
			lv_label_set_text(fi->size,make_readable_str_buf(size,31,fi->st.st_size,1,0));
			lv_label_set_long_mode(fi->size,LV_LABEL_LONG_CROP);
			lv_coord_t xs=lv_obj_get_width(fi->size),min=view->bw/5;
			if(xs>min){
				lv_obj_set_width(fi->size,min);
				xs=min;
			}
			lv_obj_align(fi->size,fi->btn,LV_ALIGN_IN_TOP_RIGHT,-gui_font_size/2,gui_font_size/2);
			lv_obj_set_width(e->label,
				 view->bw-si-gui_font_size*2-
				 xs-lv_obj_get_width(e->bullet)
			 );
		}

		// symbolic link target
		char tgt[128]={0};
		if(S_ISLNK(m))readlinkat(view->fd,name,tgt,127);

		// file info1 (time and permission)
		fi->info1=lv_label_create(line,NULL);
		lv_obj_align(fi->info1,fi->chk,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size/4);
		lv_obj_set_small_text_font(fi->info1,LV_LABEL_PART_MAIN);
		lv_label_set_text_fmt(fi->info1,"%s %s",times,mode_string(fi->st.st_mode));

		// file info2 (owner/group, device node and symbolic link target)
		char owner[128]={0},group[128]={0};
		fi->info2=lv_label_create(line,NULL);
		lv_obj_align(fi->info2,fi->info1,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
		lv_obj_set_small_text_font(fi->info2,LV_LABEL_PART_MAIN);
		lv_label_set_text_fmt(
			fi->info2,
			"%s:%s %s%s",
			get_username(fi->st.st_uid,owner,127),
			get_groupname(fi->st.st_gid,group,127),
			dev,tgt
		);
	}
	view->count++;
}

static void set_info(struct fileview*view,char*text){
	if(!view->view)return;
	if(view->info)lv_obj_del(view->info);
	view->info=lv_label_create(view->view,NULL);
	lv_label_set_long_mode(view->info,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(view->info,lv_page_get_scrl_width(view->view)-gui_font_size);
	lv_label_set_align(view->info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(view->info,text);
}

static int _free_item(void*d){
	struct fileitem*fi=(struct fileitem*)d;
	lv_obj_del(fi->btn);
	free(fi);
	return 0;
}

static void clean_items(struct fileview*view){
	if(!view->view)return;
	if(view->items)list_free_all(view->items,_free_item);
	if(view->info)lv_obj_del(view->info);
	view->last_btn=NULL,view->items=NULL;
	view->info=NULL,view->count=0;
	lv_obj_set_y(lv_page_get_scrollable(view->view),0);
}

static void scan_items(struct fileview*view){
	if(!view->view||!view->path[0])return;
	clean_items(view);
	if(strcmp(view->path,"/")!=0&&view->parent)add_item(view,"..",DT_DIR);
	DIR*d=fdopendir(view->fd);
	if(!d){
		telog_warn("fdopen folder %s failed",view->path);
		set_info(view,_("fd open dir failed"));
		return;
	}
	int i=0;
	struct dirent*e;
	seekdir(d,0);
	while((e=readdir(d))){
		if(e->d_type!=DT_DIR)continue;
		if(strcmp(e->d_name,".")==0||strcmp(e->d_name,"..")==0)continue;
		if(e->d_name[0]=='.'&&!view->hidden)continue;
		add_item(view,e->d_name,e->d_type);
		if(++i>=1024)break;
	}
	seekdir(d,0);
	while((e=readdir(d))){
		if(e->d_type==DT_DIR)continue;
		if(e->d_name[0]=='.'&&!view->hidden)continue;
		add_item(view,e->d_name,e->d_type);
		if(++i>=1024)break;
	}
	if(i>=1024)tlog_warn("too many files, skip");
	free(d);
	if(view->count<=0)set_info(view,_("nothing here"));
}

void fileview_set_path(struct fileview*view,char*path){
	if(!view)return;
	if(path&&path[0]){
		if(view->fd>0)close(view->fd);
		if((view->fd=open(path,O_DIR|O_CLOEXEC))<0){
			telog_warn("open folder %s failed",path);
			clean_items(view);
			set_info(view,_("open dir failed"));
			return;
		}
		strncpy(view->old_path,view->path,sizeof(view->old_path));
		get_fd_path(view->fd,view->path,sizeof(view->path)-1);
		call_on_change_dir(view);
	}
	scan_items(view);
}

struct fileview*fileview_create(lv_obj_t*screen){
	struct fileview*view=malloc(sizeof(struct fileview));
	if(!view)return NULL;
	memset(view,0,sizeof(struct fileview));
	view->view=screen;
	view->fd=-1;
	view->parent=true;
	return view;
}

void fileview_set_on_change_dir(struct fileview*view,fileview_on_change_dir cb){
	view->on_change_dir=cb;
}

void fileview_set_on_item_click(struct fileview*view,fileview_on_item_click cb){
	view->on_click_item=cb;
}

void fileview_set_data(struct fileview*view,void*data){
	if(view)view->data=data;
}

void*fileview_get_data(struct fileview*view){
	return view?view->data:NULL;
}

char*fileview_get_path(struct fileview*view){
	return view?view->path:NULL;
}

void fileview_set_show_parent(struct fileview*view,bool parent){
	if(view)view->parent=parent;
}

bool fileview_is_top(struct fileview*view){
	return view?strcmp(view->path,"/")==0:false;
}

void fileview_add_group(struct fileview*view,lv_group_t*grp){
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		lv_group_add_obj(grp,fi->img);
		lv_group_add_obj(grp,fi->chk);
	}while((l=l->next));
	view->grp=grp;
}

void fileview_remove_group(struct fileview*view){
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		lv_group_remove_obj(fi->img);
		lv_group_remove_obj(fi->chk);
	}while((l=l->next));
	view->grp=NULL;
}

void fileview_free(struct fileview*view){
	if(!view)return;
	fileview_remove_group(view);
	clean_items(view);
	free(view);
}

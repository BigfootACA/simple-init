#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<stdlib.h>
#ifndef ENABLE_UEFI
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#endif
#include"gui.h"
#include"str.h"
#include"list.h"
#include"logger.h"
#include"system.h"
#include"gui/fsext.h"
#include"gui/tools.h"
#include"gui/fileview.h"
#define TAG "fileview"
#define MIME_DIR _PATH_USR"/share/pixmaps/mime"
#define MIME_EXT ".png"

struct fileview{
	char letter;
	char path[PATH_MAX],full_path[PATH_MAX];
	lv_obj_t*view,*info,*last_btn;
	list*items;
	bool hidden,parent;
	int count;
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
	char name[256],letter;
	struct stat st;
};

static char*get_parent(char*buff,size_t size,char*path){
	if(!buff||!path||size<=0)return NULL;
	memset(buff,0,size);
	strncpy(buff,path,size-1);
	size_t len=strlen(buff);
	if(len==0)return buff;
	if(buff[len-1]=='/')buff[len-1]=0,len--;
	while(len>0&&buff[len-1]!='/')buff[len-1]=0,len--;
	return buff;
}

static const char*fileitem_get_path(char*path,size_t size,struct fileitem*fi){
	snprintf(path,size,"%c:%s/%s",fi->view->letter,fi->view->path,fi->name);
	return path;
}

static enum item_type get_type(struct fileitem*fi){
	if(!fi->view->letter)return TYPE_DISK;
	enum item_type type=0;
	char xpath[PATH_MAX+4]={0};
	fileitem_get_path(xpath,PATH_MAX+3,fi);
	lv_fs_get_type(&type,xpath);
	return type;
}

static const char*get_icon(struct fileitem*fi){
	switch(get_type(fi)){
		case TYPE_DIR:return strcmp(fi->name,"..")==0?"inode-parent":"inode-dir";
		case TYPE_BLOCK:return "inode-blockdevice";
		case TYPE_CHAR:return "inode-chardevice";
		case TYPE_SOCK:return "inode-socket";
		case TYPE_LINK:return "inode-symlink";
		case TYPE_FIFO:return "inode-fifo";
		case TYPE_FILE:return "inode-file";
		case TYPE_DISK:return "inode-disk";
		default:return "unknown";
	}
}

static void call_on_change_dir(struct fileview*view,char*oldpath){
	if(strcmp(view->full_path,oldpath)==0)return;
	tlog_debug("change dir to %s",view->full_path);
	if(view->on_change_dir)view->on_change_dir(view,oldpath,view->full_path);
}

static bool call_on_click_item(struct fileitem*fi,bool dir){
	if(!fi->view||!fi->view->on_click_item)return true;
	return fi->view->on_click_item(fi->view,fi->name,dir);
}

void fileview_go_back(struct fileview*fv){
	if(strcmp(fv->path,"/")!=0){
		char path[PATH_MAX]={0};
		get_parent(path,PATH_MAX,fv->path);
		fileview_set_path(fv,path);
		return;
	}
	if(fsext_is_multi&&fv->letter){
		fv->letter=0;
		fileview_set_path(fv,"/");
	}
}

static void item_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct fileitem*fi=(struct fileitem*)lv_obj_get_user_data(obj);
	if(!fi)return;
	struct fileview*fv=fi->view;
	if(!fv)return;
	if(fi->letter){
		fv->letter=fi->letter;
		fileview_set_path(fv,"/");
		return;
	}
	char path[PATH_MAX+4]={0};
	fileitem_get_path(path,PATH_MAX+3,fi);
	if(strcmp(fi->name,"..")==0)fileview_go_back(fv);
	else{
		bool dir=lv_fs_is_dir(path);
		if(!call_on_click_item(fi,dir))return;
		if(!dir)return;
		size_t s=strlen(fv->path);
		if(fv->path[s-1]=='/')fv->path[s-1]=0;
		char xpath[sizeof(fv->path)+sizeof(fi->name)+2]={0};
		snprintf(xpath,sizeof(xpath)-1,"%s/%s",fv->path,fi->name);
		fileview_set_path(fv,xpath);
	}
}

static struct fileitem*add_item(struct fileview*view,char*name){
	if(!view->view||!name)return NULL;
	if(view->bw==0)view->bw=lv_page_get_scrl_width(view->view)-gui_font_size;
	if(view->bh==0)view->bh=gui_dpi/2;
	struct fileitem*fi=malloc(sizeof(struct fileitem));
	if(!fi){
		telog_error("cannot allocate fileitem");
		abort();
	}
	memset(fi,0,sizeof(struct fileitem));
	fi->view=view;
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
	if(list_obj_add_new(&view->items,fi)!=0){
		telog_error("cannot add fileitem list");
		abort();
	}

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
	lv_obj_align(fi->img,fi->btn,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
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
	lv_img_set_pivot(fi->img,0,0);

	// file name and checkbox
	fi->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(fi->chk,strcmp(fi->name,"..")==0?_("Parent folder"):name);
	lv_style_set_focus_checkbox(fi->chk);
	lv_obj_align(fi->chk,NULL,LV_ALIGN_IN_LEFT_MID,gui_font_size+si,-gui_font_size);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(fi->chk);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_SROLL_CIRC);

	// add group
	if(view->grp){
		lv_group_add_obj(view->grp,fi->img);
		lv_group_add_obj(view->grp,fi->chk);
	}

	if(get_type(fi)==TYPE_FILE){
		lv_fs_file_t f;
		char fp[PATH_MAX+4]={0};
		fileitem_get_path(fp,PATH_MAX+3,fi);
		uint32_t s=0;
		if(lv_fs_open(&f,fp,LV_FS_MODE_RD)==LV_FS_RES_OK){
			if(lv_fs_size(&f,&s)==LV_FS_RES_OK){
				char size[32]={0};
				fi->size=lv_label_create(line,NULL);
				lv_label_set_text(fi->size,make_readable_str_buf(size,31,s,1,0));
				lv_label_set_long_mode(fi->size,LV_LABEL_LONG_CROP);
				lv_coord_t xs=lv_obj_get_width(fi->size),min=view->bw/5;
				if(xs>min){
					lv_obj_set_width(fi->size,min);
					xs=min;
				}
				lv_obj_align(
					fi->size,fi->btn,
					LV_ALIGN_IN_TOP_RIGHT,
					-gui_font_size/2,
					gui_font_size/2
				);
				lv_obj_set_width(e->label,
					 view->bw-si-gui_font_size*2-
					 xs-lv_obj_get_width(e->bullet)
				);
			}
			lv_fs_close(&f);
		}
	}

	#ifndef ENABLE_UEFI
	int fd=open(view->path,O_DIR);
	if(fd>=0&&fstatat(fd,name,&fi->st,AT_SYMLINK_NOFOLLOW)==0){
		mode_t m=fi->st.st_mode;
		char times[64]={0};
		strftime(times,63,"%Y/%m/%d %H:%M:%S",localtime(&fi->st.st_mtime));

		// file info1 (time and permission)
		fi->info1=lv_label_create(line,NULL);
		lv_obj_align(fi->info1,fi->chk,LV_ALIGN_OUT_BOTTOM_LEFT,0,gui_font_size/4);
		lv_obj_set_small_text_font(fi->info1,LV_LABEL_PART_MAIN);
		lv_label_set_text_fmt(fi->info1,"%s %s",times,mode_string(fi->st.st_mode));

		#ifndef ENABLE_UEFI
		dev_t d=fi->st.st_dev;
		char dev[32]={0};
		if(S_ISCHR(m))snprintf(dev,31,"char[%d:%d] ",major(d),minor(d));
		if(S_ISBLK(m))snprintf(dev,31,"block[%d:%d] ",major(d),minor(d));
		// symbolic link target
		char tgt[128]={0};
		if(S_ISLNK(m))readlinkat(fd,name,tgt,127);

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
		#endif
		close(fd);
	}
	#endif
	view->count++;
	return fi;
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

static void set_info(struct fileview*view,char*text,...){
	if(!view->view)return;
	clean_items(view);
	if(view->info)lv_obj_del(view->info);
	view->info=lv_label_create(view->view,NULL);
	lv_label_set_long_mode(view->info,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(view->info,lv_page_get_scrl_width(view->view)-gui_font_size);
	lv_label_set_align(view->info,LV_LABEL_ALIGN_CENTER);
	char buf[BUFSIZ]={0};
	va_list va;
	va_start(va,text);
	vsnprintf(buf,BUFSIZ-1,text,va);
	va_end(va);
	lv_label_set_text(view->info,buf);
}

static void scan_items(struct fileview*view){
	if(!view->view||!view->path[0])return;
	clean_items(view);
	if(fsext_is_multi&&!view->letter){
		char letters[64]={0},x;
		lv_fs_get_letters(letters);
		for(int i=0;(x=letters[i]);i++){
			char name[256]={0};
			lv_fs_drv_t*t=lv_fs_get_drv(x);
			if(!t)continue;
			name[0]=x,name[1]=':',name[2]=' ';
			lv_fs_get_volume_label(t,name+3,253);
			struct fileitem*fi=add_item(view,name);
			if(!fi)continue;
			fi->letter=x;
		}
		return;
	}
	if(view->parent)add_item(view,"..");
	lv_fs_dir_t dir;
	int i;
	char fn[256],*fp;
	char path[PATH_MAX+4]={0};
	snprintf(path,PATH_MAX+3,"%c:%s",view->letter,view->path);
	lv_fs_res_t res=lv_fs_dir_open(&dir,path);
	if(res!=LV_FS_RES_OK){
		telog_warn("open folder %s failed (%s)",view->path,lv_fs_res_to_string(res));
		set_info(view,_("open dir failed: %s"),lv_fs_res_to_i18n_string(res));
		return;
	}
	for(i=0;i<1024;i++){
		fp=fn;
		if(lv_fs_dir_read(&dir,fn)!=LV_FS_RES_OK)break;
		if(strlen(fn)==0)break;
		if(fp[0]=='/')fp++;
		if(fp[0]=='.'&&!view->hidden)continue;
		add_item(view,fp);
	}
	lv_fs_dir_close(&dir);
	if(i>=1024)tlog_warn("too many files, skip");
	if(view->count<=0)set_info(view,_("nothing here"));
	return;
}

void fileview_set_path(struct fileview*view,char*path){
	if(!view)return;
	if(path&&path[0]){
		char oldpath[sizeof(view->path)]={0};
		strncpy(oldpath,view->full_path,sizeof(oldpath)-1);
		strncpy(view->path,path,sizeof(view->path)-1);
		if(!view->letter||!fsext_is_multi)strcpy(view->full_path,view->path);
		else snprintf(view->full_path,PATH_MAX-1,"%c:%s",view->letter,view->path);
		call_on_change_dir(view,oldpath);
	}
	scan_items(view);
}

extern void lvgl_init_all_fs_uefi(bool);
struct fileview*fileview_create(lv_obj_t*screen){
	struct fileview*view=malloc(sizeof(struct fileview));
	if(!view)return NULL;
	memset(view,0,sizeof(struct fileview));
	view->view=screen;
	view->parent=true;
	#ifdef ENABLE_UEFI
	view->letter=0;
	lvgl_init_all_fs_uefi(true);
	#else
	view->letter='C';
	init_lvgl_fs('C',"/",true);
	#endif
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
	return view?view->full_path:NULL;
}

char*fileview_get_lvgl_path(struct fileview*view){
	if(!view)return NULL;
	if(!view->letter)return "/";
	static char path[PATH_MAX];
	memset(path,0,PATH_MAX);
	snprintf(path,PATH_MAX-1,"%c:%s",view->letter,view->path);
	return path;
}

void fileview_set_show_parent(struct fileview*view,bool parent){
	if(view)view->parent=parent;
}

bool fileview_is_top(struct fileview*view){
	if(!view)return true;
	if(fsext_is_multi&&view->letter)return false;
	return strcmp(view->path,"/")==0;
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
#endif

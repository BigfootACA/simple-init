/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<stdlib.h>
#ifdef ENABLE_UEFI
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include<Guid/FileSystemInfo.h>
#include"uefi.h"
#else
#include<fcntl.h>
#include<sys/sysmacros.h>
#endif
#include"gui.h"
#include"str.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"filesystem.h"
#include"gui/tools.h"
#include"gui/fileview.h"
#define TAG "fileview"

static lv_label_long_mode_t lm;

struct fileview{
	lv_obj_t*view,*info;
	list*items;
	bool hidden,parent,verbose,check_mode;
	int count;
	lv_group_t*grp;
	fileview_on_item_select on_select_item;
	fileview_on_item_click on_click_item;
	fileview_on_change_dir on_change_dir;
	url*url,*root;
	fsh*folder;
	void*data;
};

struct fileitem{
	struct fileview*view;
	lv_obj_t*btn,*fn,*w_img,*img;
	lv_obj_t*size,*info1,*info2;
	fs_type type;
	fs_file_info file;
	fsvol_info*vol;
};

static const char*get_icon(struct fileitem*fi){
	if(fs_has_type(fi->type,FS_TYPE_PARENT))return "@mime-inode-parent";
	if(fs_has_type(fi->type,FS_TYPE_VOLUME))return "@mime-inode-disk";
	if(fs_has_type(fi->type,FS_TYPE_FILE))switch(fi->type){
		case FS_TYPE_FILE_FOLDER:return "@mime-inode-dir";
		case FS_TYPE_FILE_BLOCK:return "@mime-inode-blockdevice";
		case FS_TYPE_FILE_CHAR:return "@mime-inode-chardevice";
		case FS_TYPE_FILE_SOCKET:return "@mime-inode-socket";
		case FS_TYPE_FILE_LINK:return "@mime-inode-symlink";
		case FS_TYPE_FILE_FIFO:return "@mime-inode-fifo";
		case FS_TYPE_FILE_REG:return "@mime-inode-file";
		default:;
	}
	return "@mime-unknown";
}

static void call_on_change_dir(struct fileview*view,url*old){
	char buff[PATH_MAX],*path;
	if(url_equals(old,view->url))return;
	path=view->url?url_generate(buff,sizeof(buff),view->url):"/";
	tlog_debug("change dir to %s",path);
	if(view->on_change_dir)view->on_change_dir(view,old,view->url);
}

static bool call_on_click_item(struct fileitem*fi){
	char*name;
	if(!fi->view||!fi->view->on_click_item)return true;
	if(fs_has_type(fi->type,FS_TYPE_PARENT))name="..";
	else if(fs_has_type(fi->type,FS_TYPE_FILE))name=fi->file.name;
	else if(fs_has_type(fi->type,FS_TYPE_VOLUME))name=fi->vol->name;
	else return true;
	return fi->view->on_click_item(fi->view,name,fi->type);
}

static void call_on_select_item(
	struct fileview*view,
	char*item,
	fs_type type,
	bool checked,
	uint16_t cnt
){
	if(!view||!view->on_select_item)return;
	view->on_select_item(view,item,type,checked,cnt);
}

void fileview_go_back(struct fileview*fv){
	url*old;
	if(!fv||!fv->url)return;
	if(
		url_is_on_top(fv->url)||
		(fv->url->path&&fv->root&&fv->root->path&&
		strcmp(fv->root->path,fv->url->path)==0)
	){
		old=fv->url,fv->url=NULL;
		fs_close(&fv->folder);
		call_on_change_dir(fv,old);
		fileview_set_path(fv,NULL);
		url_free(old);
	}else{
		if(!(old=url_dup(fv->url)))return;
		url_go_back(old,true);
		fileview_set_url(fv,old);
		url_free(old);
	}
}

static void check_item(struct fileitem*fi,bool checked){
	if(!fi||!fi->view||!fi->btn)return;
	if(fs_has_type(fi->type,FS_TYPE_PARENT)){
		fileview_go_back(fi->view);
		return;
	}
	if(fs_has_type(fi->type,FS_TYPE_VOLUME)){
		lv_obj_set_checked(fi->btn,false);
		return;
	}
	lv_obj_set_checked(fi->btn,checked);
	call_on_select_item(
		fi->view,fi->file.name,fi->type,checked,
		fileview_get_checked_count(fi->view)
	);
}

static void click_item(struct fileitem*fi){
	int r;
	url*n=NULL;
	fsh*nf=NULL;
	struct fileview*fv;
	if(!fi||!(fv=fi->view))return;
	if(fs_has_type(fi->type,FS_TYPE_VOLUME)){
		if(fv->url||fv->folder)return;
		if((r=fsvol_open_volume(fi->vol,&fv->folder))!=0){
			tlog_warn("open volume failed: %s",strerror(r));
			return;
		}
		fs_get_url(fv->folder,&n);
		fv->root=n;
		fileview_set_url(fv,n);
	}else if(fs_has_type(fi->type,FS_TYPE_PARENT)){
		fileview_go_back(fv);
	}else if(fs_has_type(fi->type,FS_TYPE_FILE)){
		if(fileview_get_checked_count(fi->view)>0){
			if(fs_has_type(fi->type,FS_TYPE_FILE))
				check_item(fi,!lv_obj_is_checked(fi->btn));
			return;
		}
		if(!call_on_click_item(fi))return;
		if(fs_has_type(fi->type,FS_TYPE_FILE_FOLDER)){
			if((r=fs_open_with(&nf,&fi->file,FILE_FLAG_FOLDER))!=0){
				tlog_warn("open folder failed: %s",strerror(r));
				return;
			}
			if((r=fs_get_url(nf,&n))!=0){
				fs_close(&nf);
				tlog_warn("get url failed: %s",strerror(r));
				return;
			}
			fs_close(&fv->folder);
			fv->folder=nf;
			fileview_set_url(fv,n);
			url_free(n);
		}
	}
}

static void item_click(lv_event_t*e){
	lv_indev_t*i=lv_indev_get_act();
	if(i&&i->proc.long_pr_sent)return;
	click_item(e->user_data);
}

static void item_check(lv_event_t*e){
	struct fileitem*fi=e->user_data;
	if(!fi||!fi->btn)return;
	e->stop_processing=1;
	check_item(fi,!lv_obj_is_checked(fi->btn));
}

static struct fileitem*get_item(struct fileview*view,const char*name){
	list*l;
	bool parent;
	if(!view||!name)return NULL;
	parent=strcmp(name,"..")==0;
	if((l=list_first(view->items)))do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		if(!fi||fi->view!=view)continue;
		if(fs_has_type(fi->type,FS_TYPE_PARENT))
			if(parent)return fi;
		if(fs_has_type(fi->type,FS_TYPE_FILE))
			if(strcmp(fi->file.name,name)==0)return fi;
		if(fs_has_type(fi->type,FS_TYPE_VOLUME))
			if(strcmp(fi->vol->name,name)==0)return fi;
	}while((l=l->next));
	return NULL;
}

static void draw_item_file_info(struct fileitem*fi){
	char dev[32];
	uint8_t cs=2,rs=3;
	memset(dev,0,sizeof(dev));

	if(
		fi->file.type==FS_TYPE_FILE_REG&&
		fs_has_feature(fi->file.features,FS_FEATURE_HAVE_SIZE)
	){
		char size[32];
		make_readable_str_buf(size,sizeof(size)-1,fi->file.size,1,0);
		fi->size=lv_label_create(fi->btn);
		lv_label_set_text(fi->size,size);
		lv_label_set_long_mode(fi->size,lm);
		lv_obj_set_small_text_font(fi->size,0);
		lv_obj_set_grid_cell(
			fi->size,
			LV_GRID_ALIGN_START,2,1,
			LV_GRID_ALIGN_CENTER,0,1
		);
		cs=1;
	}

	if(
		fs_has_feature(fi->file.features,FS_FEATURE_UNIX_PERM)||
		fs_has_feature(fi->file.features,FS_FEATURE_HAVE_TIME)
	){
		char times[64];
		memset(times,0,sizeof(times));
		if(fs_has_feature(
			fi->file.features,
			FS_FEATURE_HAVE_TIME
		))strftime(
			times,sizeof(times),
			"%Y/%m/%d %H:%M:%S",
			localtime(&fi->file.mtime)
		);

		// file info1 (time and permission)
		fi->info1=lv_label_create(fi->btn);
		lv_obj_set_small_text_font(fi->info1,LV_PART_MAIN);
		#ifdef ENABLE_UEFI
		lv_label_set_text(fi->info1,times);
		#else
		lv_label_set_text_fmt(
			fi->info1,"%s %s",
			times,mode_string(fi->file.mode)
		);
		#endif
		lv_obj_set_grid_cell(
			fi->info1,
			LV_GRID_ALIGN_START,1,2,
			LV_GRID_ALIGN_CENTER,1,1
		);
		rs--;
	}

	#ifndef ENABLE_UEFI
	if(fs_has_feature(fi->file.features,FS_FEATURE_UNIX_DEVICE)){
		char*dt=NULL;
		if(fi->file.type==FS_TYPE_FILE_CHAR)dt="char";
		if(fi->file.type==FS_TYPE_FILE_BLOCK)dt="block";
		if(dt)snprintf(
			dev,sizeof(dev)-1,
			"%s[%d:%d] ",dt,
			major(fi->file.device),
			minor(fi->file.device)
		);
	}

	if(fs_has_feature(fi->file.features,FS_FEATURE_UNIX_PERM)){
		char owner[128],group[128];
		memset(owner,0,sizeof(owner));
		memset(group,0,sizeof(group));
		// file info2 (owner/group, device node and symbolic link target)
		fi->info2=lv_label_create(fi->btn);
		lv_obj_align_to(fi->info2,fi->info1,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
		lv_obj_set_small_text_font(fi->info2,LV_PART_MAIN);
		lv_label_set_text_fmt(
			fi->info2,"%s:%s %s%s",
			get_username(fi->file.owner,owner,sizeof(owner)-1),
			get_groupname(fi->file.group,group,sizeof(group)-1),
			dev,fi->file.type==FS_TYPE_FILE_LINK?fi->file.target:""
		);
		lv_obj_set_grid_cell(
			fi->info2,
			LV_GRID_ALIGN_START,1,2,
			LV_GRID_ALIGN_CENTER,2,1
		);
		rs--;
	}
	#else
	rs--;
	#endif

	lv_obj_set_grid_cell(
		fi->fn,
		LV_GRID_ALIGN_STRETCH,1,cs,
		LV_GRID_ALIGN_CENTER,0,rs
	);
}

static void draw_item_volume_info(struct fileitem*fi){
	uint8_t rs=3;
	char used[32],size[32];

	if(fi->vol->part.label[0]){
		// partition name
		fi->info1=lv_label_create(fi->btn);
		lv_obj_set_small_text_font(fi->info1,LV_PART_MAIN);
		lv_label_set_text(fi->info1,fi->vol->part.label);
		lv_obj_set_grid_cell(
			fi->info1,
			LV_GRID_ALIGN_START,1,2,
			LV_GRID_ALIGN_CENTER,1,1
		);
		rs--;
	}

	if(fi->vol->fs.size>0){
		int pct=fi->vol->fs.used*100/fi->vol->fs.size;
		make_readable_str_buf(used,sizeof(used),fi->vol->fs.used,1,0);
		make_readable_str_buf(size,sizeof(size),fi->vol->fs.size,1,0);
		fi->info2=lv_label_create(fi->btn);
		lv_label_set_text_fmt(fi->info2,"%s/%s (%d%%)",used,size,pct);
		lv_label_set_long_mode(fi->info2,lm);
		lv_obj_set_small_text_font(fi->info2,0);
		lv_obj_set_grid_cell(
			fi->info2,
			LV_GRID_ALIGN_START,1,2,
			LV_GRID_ALIGN_CENTER,2,1
		);
		rs--;
	}

	lv_obj_set_grid_cell(
		fi->fn,
		LV_GRID_ALIGN_STRETCH,1,2,
		LV_GRID_ALIGN_CENTER,0,rs
	);
}

static struct fileitem*add_item(
	struct fileitem*item,
	struct fileview*view,
	fs_type type,
	fs_file_info*file,
	fsvol_info*vol
){
	struct fileitem*fi=item;
	static lv_coord_t grid_col[]={
		0,
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	if(!view->view)return NULL;
	if(fs_has_type(type,FS_TYPE_PARENT)){
		if(file||vol)return NULL;
	}else if(fs_has_type(type,FS_TYPE_FILE)){
		if(!file||vol)return NULL;
	}else if(fs_has_type(type,FS_TYPE_VOLUME)){
		if(file||!vol)return NULL;
	}else return NULL;
	if(!fi){
		if(!(fi=malloc(sizeof(struct fileitem)))){
			telog_error("cannot allocate fileitem");
			abort();
		}
		memset(fi,0,sizeof(struct fileitem));
		fi->view=view,fi->type=type;
		if(fs_has_type(fi->type,FS_TYPE_FILE))
			memcpy(&fi->file,file,sizeof(fs_file_info));
		else if(fs_has_type(fi->type,FS_TYPE_VOLUME))
			fi->vol=vol;
		if(list_obj_add_new(&view->items,fi)!=0){
			telog_error("cannot add file item list");
			abort();
		}
	}else if(!fi->view||fi->type==FS_TYPE_NONE)abort();
	grid_col[0]=gui_font_size*(view->verbose?3:1);

	// file item button
	fi->btn=lv_btn_create(view->view);
	lv_obj_set_width(fi->btn,lv_pct(100));
	lv_obj_set_content_height(fi->btn,grid_col[0]);
	lv_style_set_btn_item(fi->btn);
	lv_obj_add_event_cb(fi->btn,item_click,LV_EVENT_CLICKED,fi);
	lv_obj_add_event_cb(fi->btn,item_check,LV_EVENT_LONG_PRESSED,fi);
	lv_obj_set_grid_dsc_array(fi->btn,grid_col,grid_row);

	// file image
	fi->w_img=lv_obj_create(fi->btn);
	lv_obj_set_size(fi->w_img,grid_col[0],grid_col[0]);
	lv_obj_clear_flag(fi->w_img,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(fi->w_img,LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_border_width(fi->w_img,0,0);
	lv_obj_set_style_bg_opa(fi->w_img,LV_OPA_0,0);
	lv_obj_set_grid_cell(
		fi->w_img,
		LV_GRID_ALIGN_STRETCH,0,1,
		LV_GRID_ALIGN_STRETCH,0,3
	);
	fi->img=lv_img_create(fi->w_img);
	lv_img_t*ext=(lv_img_t*)fi->img;
	lv_img_set_src(fi->img,get_icon(fi));
	if(ext->w<=0||ext->h<=0)
		lv_img_set_src(fi->img,"@mime-inode-file");
	lv_img_set_size_mode(fi->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_fill_image(fi->img,grid_col[0],grid_col[0]);
	lv_obj_center(fi->img);

	fi->fn=lv_label_create(fi->btn);
	lv_obj_set_small_text_font(fi->fn,0);
	lv_label_set_long_mode(fi->fn,lm);
	if(fs_has_type(type,FS_TYPE_PARENT)){
		lv_label_set_text(fi->fn,_("Parent folder"));
	}else if(fs_has_type(type,FS_TYPE_FILE)){
		lv_label_set_text(fi->fn,fi->file.name);
	}else if(fs_has_type(type,FS_TYPE_VOLUME)){
		lv_label_set_text(fi->fn,fi->vol->title);
	}

	// add group
	if(view->grp)lv_group_add_obj(view->grp,fi->btn);

	if(view->verbose){
		if(fs_has_type(type,FS_TYPE_FILE))
			draw_item_file_info(fi);
		else if(fs_has_type(type,FS_TYPE_VOLUME))
			draw_item_volume_info(fi);
		else lv_obj_set_grid_cell(
			fi->fn,
			LV_GRID_ALIGN_STRETCH,1,2,
			LV_GRID_ALIGN_CENTER,0,3
		);
	}else lv_obj_set_grid_cell(
		fi->fn,
		LV_GRID_ALIGN_STRETCH,1,2,
		LV_GRID_ALIGN_CENTER,0,3
	);
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
	view->items=NULL,view->info=NULL,view->count=0;
	call_on_select_item(view,NULL,0,false,0);
}

static void set_info(struct fileview*view,char*text,...){
	if(!view->view)return;
	clean_items(view);
	if(view->info)lv_obj_del(view->info);
	view->info=lv_label_create(view->view);
	lv_label_set_long_mode(view->info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(view->info,lv_pct(100));
	lv_obj_set_style_text_align(view->info,LV_TEXT_ALIGN_CENTER,0);
	char*buf=NULL;
	va_list va;
	va_start(va,text);
	vasprintf(&buf,text,va);
	va_end(va);
	lv_label_set_text(view->info,buf?buf:"");
	if(buf)free(buf);
}

static bool fileitem_sorter(list*a,list*b){
	LIST_DATA_DECLARE(fa,a,struct fileitem*);
	LIST_DATA_DECLARE(fb,b,struct fileitem*);
	if(!fa||!fb)return false;
	if(
		fs_has_type(fa->type,FS_TYPE_PARENT)!=
		fs_has_type(fb->type,FS_TYPE_PARENT)
	)return !fs_has_type(fa->type,FS_TYPE_PARENT);
	if(
		fs_has_type(fa->type,FS_TYPE_FILE_FOLDER)!=
		fs_has_type(fb->type,FS_TYPE_FILE_FOLDER)
	)return !fs_has_type(fa->type,FS_TYPE_FILE_FOLDER);
	char*na=fa->file.name,*nb=fb->file.name;
	if(!na[0]||!nb[0])return false;
	for(size_t i=0;na[i]&&nb[i];i++)
		if(na[i]!=nb[i])return na[i]>nb[i];
	return false;
}

static void scan_items(struct fileview*view){
	list*l;
	int r=0;
	fs_file_info info;
	fsvol_info**vols;
	struct fileitem item;
	if(!view->view)return;
	clean_items(view);
	if(view->url){
		if(view->parent&&!fileview_is_top(view))
			add_item(NULL,view,FS_TYPE_PARENT,NULL,NULL);
		if(!view->folder){
			if((r=fs_open_uri(
				&view->folder,
				view->url,
				FILE_FLAG_FOLDER
			))!=0){
				telog_warn("open folder failed: %s",strerror(r));
				set_info(view,_("open dir failed: %s"),strerror(r));
				return;
			}
		}else fs_seek(view->folder,0,SEEK_SET);
		while((r=fs_readdir(view->folder,&info))==0){
			if(info.name[0]=='.'&&!view->hidden)continue;
			memset(&item,0,sizeof(item));
			memcpy(&item.file,&info,sizeof(info));
			item.view=view,item.type=info.type;
			list_obj_add_new_dup(&view->items,&item,sizeof(item));
			if(view->count>=256)tlog_warn("too many files, skip");
		}
		list_sort(view->items,fileitem_sorter);
		if((l=list_first(view->items)))do{
			LIST_DATA_DECLARE(i,l,struct fileitem*);
			add_item(i,view,i->type,&i->file,NULL);
		}while((l=l->next));
		if(r==EOF)r=0;
	}else if((vols=fsvol_get_volumes())){
		for(size_t i=0;vols[i];i++){
			if(!view->hidden&&fs_has_vol_feature(
				vols[i]->features,FSVOL_HIDDEN
			))continue;
			if(!fs_has_vol_feature(
				vols[i]->features,FSVOL_FILES
			))continue;
			add_item(NULL,view,FS_TYPE_VOLUME,NULL,vols[i]);
			if(view->count>=256)tlog_warn("too many volumes, skip");
		}
		free(vols);
	}
	if(view->count<=0)set_info(view,_("nothing here"));
	if(r!=0){
		tlog_warn("read dir failed: %s",strerror(r));
		set_info(view,_("read dir failed: %s"),strerror(r));
	}
}

void fileview_set_url(struct fileview*view,url*u){
	url*old=NULL;
	if(!view||u==view->url)return;
	old=view->url,view->url=url_dup(u);
	if(u&&!view->url){
		view->url=old;
		return;
	}
	call_on_change_dir(view,old);
	if(old)url_free(old);
	if(!u){
		url_free(view->root);
		view->root=NULL;
	}
	fs_close(&view->folder);
	scan_items(view);
}

void fileview_set_path(struct fileview*view,char*path){
	if(!view)return;
	if(path&&path[0]){
		url*n=NULL;
		if(strcmp(path,"/")!=0){
			if(!(n=url_new()))return;
			if(!url_parse(n,path,0)){
				tlog_warn("parse url %s failed",path);
				url_free(n);
				return;
			}
		}
		if(n!=view->url){
			fileview_set_url(view,n);
			return;
		}
	}
	scan_items(view);
}

extern void lvgl_init_all_fs_uefi(bool);
struct fileview*fileview_create(lv_obj_t*screen){
	struct fileview*view=malloc(sizeof(struct fileview));
	if(!view)return NULL;
	memset(view,0,sizeof(struct fileview));
	view->verbose=true;
	view->view=screen;
	view->parent=true;
	lv_obj_set_flex_flow(view->view,LV_FLEX_FLOW_COLUMN);
	lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;
	return view;
}

void fileview_set_on_change_dir(struct fileview*view,fileview_on_change_dir cb){
	view->on_change_dir=cb;
}

void fileview_set_on_item_click(struct fileview*view,fileview_on_item_click cb){
	view->on_click_item=cb;
}

void fileview_set_on_item_select(struct fileview*view,fileview_on_item_select cb){
	view->on_select_item=cb;
}

void fileview_set_data(struct fileview*view,void*data){
	if(view)view->data=data;
}

void fileview_set_verbose(struct fileview*view,bool verbose){
	if(view)view->verbose=verbose;
}

uint16_t fileview_get_checked_count(struct fileview*view){
	uint16_t checked=0;
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(ffi,l,struct fileitem*);
		if(ffi&&lv_obj_is_checked(ffi->btn))
			checked++;
	}while((l=l->next));
	return checked;
}

char**fileview_get_checked(struct fileview*view){
	list*l;
	uint16_t checked=fileview_get_checked_count(view),num=0;
	size_t size=sizeof(char*)*(checked+1);
	char**arr=malloc(size);
	if(!arr)return NULL;
	memset(arr,0,size);
	if(checked>0&&(l=list_first(view->items)))do{
		LIST_DATA_DECLARE(ffi,l,struct fileitem*);
		if(!ffi||!lv_obj_is_checked(ffi->btn))continue;
		if(fs_has_type(ffi->type,FS_TYPE_PARENT)){
			arr[num++]=ffi->file.name;
		}else if(fs_has_type(ffi->type,FS_TYPE_FILE)){
			arr[num++]=ffi->file.name;
		}else if(fs_has_type(ffi->type,FS_TYPE_VOLUME)){
			arr[num++]=ffi->vol->name;
		}
	}while((l=l->next));
	return arr;
}

void*fileview_get_data(struct fileview*view){
	return view?view->data:NULL;
}

char*fileview_get_path(struct fileview*view){
	if(!view)return NULL;
	if(!view->url)return strdup("/");
	return url_generate_alloc(view->url);
}

fsh*fileview_get_fsh(struct fileview*view){
	return view?view->folder:NULL;
}

void fileview_set_show_parent(struct fileview*view,bool parent){
	if(view)view->parent=parent;
}

bool fileview_is_top(struct fileview*view){
	if(!view)return true;
	if(view->url)return false;
	return true;
}

bool fileview_click_item(struct fileview*view,const char*name){
	struct fileitem*fi=get_item(view,name);
	if(!fi)return false;
	click_item(fi);
	return true;
}

bool fileview_check_item(struct fileview*view,const char*name,bool checked){
	struct fileitem*fi=get_item(view,name);
	if(!fi)return false;
	check_item(fi,checked);
	return true;
}

void fileview_add_group(struct fileview*view,lv_group_t*grp){
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		lv_group_add_obj(grp,fi->btn);
	}while((l=l->next));
	view->grp=grp;
}

void fileview_remove_group(struct fileview*view){
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		lv_group_remove_obj(fi->btn);
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

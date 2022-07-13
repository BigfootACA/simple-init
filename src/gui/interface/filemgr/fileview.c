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
#include<sys/stat.h>
#include<sys/sysmacros.h>
#endif
#include"gui.h"
#include"str.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"gui/fsext.h"
#include"gui/tools.h"
#include"gui/fileview.h"
#define TAG "fileview"
#define MIME_EXT ".svg"

struct fileview{
	char letter;
	char path[PATH_MAX],full_path[PATH_MAX];
	lv_obj_t*view,*info;
	list*items;
	bool hidden,parent,verbose,check_mode;
	int count;
	lv_group_t*grp;
	fileview_on_item_select on_select_item;
	fileview_on_item_click on_click_item;
	fileview_on_change_dir on_change_dir;
	void*data;
};

struct fileitem{
	struct fileview*view;
	lv_obj_t*btn,*fn,*w_img,*img;
	lv_obj_t*size,*info1,*info2;
	char name[256],letter;
	struct stat st;
	char path[PATH_MAX];
	enum item_type type;
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

static const char*get_icon(struct fileitem*fi){
	switch(fi->type){
		case TYPE_DIR:return strcmp(fi->name,"..")==0?
			"inode-parent"MIME_EXT:
			"inode-dir"MIME_EXT;
		case TYPE_BLOCK:return "inode-blockdevice"MIME_EXT;
		case TYPE_CHAR:return "inode-chardevice"MIME_EXT;
		case TYPE_SOCK:return "inode-socket"MIME_EXT;
		case TYPE_LINK:return "inode-symlink"MIME_EXT;
		case TYPE_FIFO:return "inode-fifo"MIME_EXT;
		case TYPE_FILE:return "inode-file"MIME_EXT;
		case TYPE_DISK:return "inode-disk"MIME_EXT;
		default:return "unknown"MIME_EXT;
	}
}

static void call_on_change_dir(struct fileview*view,char*oldpath){
	if(strcmp(view->full_path,oldpath)==0)return;
	tlog_debug("change dir to %s",view->full_path);
	if(view->on_change_dir)view->on_change_dir(view,oldpath,view->full_path);
}

static bool call_on_click_item(struct fileitem*fi,enum item_type type){
	if(!fi->view||!fi->view->on_click_item)return true;
	return fi->view->on_click_item(fi->view,fi->name,type);
}

static void call_on_select_item(struct fileview*view,char*item,enum item_type type,bool checked,uint16_t cnt){
	if(!view||!view->on_select_item)return;
	view->on_select_item(view,item,type,checked,cnt);
}

void fileview_go_back(struct fileview*fv){
	char*p=fv->path;
	if(p[1]==':')p+=2;
	if(strcmp(p,"/")!=0){
		char path[PATH_MAX];
		path[0]=fv->letter;
		path[1]=':';
		get_parent(path+2,sizeof(path)-2,p);
		fileview_set_path(fv,fsext_is_multi?path:path+2);
		return;
	}
	if(fsext_is_multi&&fv->letter){
		fv->letter=0;
		fileview_set_path(fv,"/");
	}
}

static void check_item(struct fileitem*fi,bool checked){
	if(!fi||!fi->view||!fi->btn)return;
	if(strcmp(fi->name,"..")==0){
		fileview_go_back(fi->view);
		return;
	}
	if(fi->letter){
		lv_obj_set_checked(fi->btn,false);
		return;
	}
	lv_obj_set_checked(fi->btn,checked);
	call_on_select_item(
		fi->view,fi->name,fi->type,checked,
		fileview_get_checked_count(fi->view)
	);
}

static void click_item(struct fileitem*fi){
	if(!fi)return;
	struct fileview*fv=fi->view;
	if(!fv)return;
	if(fi->letter){
		fv->letter=fi->letter;
		fileview_set_path(fv,(char[]){fi->letter,':','/',0});
		return;
	}
	if(fileview_get_checked_count(fi->view)>0){
		check_item(fi,!lv_obj_is_checked(fi->btn));
		return;
	}
	if(strcmp(fi->name,"..")==0)fileview_go_back(fv);
	else{
		if(!call_on_click_item(fi,fi->type))return;
		if(fi->type!=TYPE_DIR)return;
		size_t s=strlen(fv->path);
		if(fv->path[s-1]=='/')fv->path[s-1]=0;
		char xpath[sizeof(fv->path)+sizeof(fi->name)+2]={0};
		snprintf(xpath,sizeof(xpath)-1,"%s/%s",fv->path,fi->name);
		fileview_set_path(fv,xpath);
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
	if(!view||!name)return NULL;
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(fi,l,struct fileitem*);
		if(!fi||fi->view!=view||strcmp(fi->name,name)!=0)continue;
		return fi;
	}while((l=l->next));
	return NULL;
}

static struct fileitem*add_item(struct fileview*view,char*name,char letter){
	if(!view->view||!name)return NULL;
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
	struct fileitem*fi=malloc(sizeof(struct fileitem));
	if(!fi){
		telog_error("cannot allocate fileitem");
		abort();
	}
	grid_col[0]=gui_font_size*(view->verbose?3:1);
	memset(fi,0,sizeof(struct fileitem));
	fi->view=view;
	strncpy(fi->name,name,sizeof(fi->name)-1);
	if(view->path[1]!=':')strlcat(
		fi->path,
		(char[]){view->letter,':',0},
		sizeof(fi->path)-1
	);
	strlcat(
		fi->path,view->path,
		sizeof(fi->path)-1
	);
	size_t len=strlen(fi->path);
	for(size_t i=0;i<len;i++)
		if(fi->path[i]=='\\')
			fi->path[i]='/';
	if(len>0&&fi->path[len-1]!='/')
		fi->path[len]='/';
	strlcat(
		fi->path,fi->name,
		sizeof(fi->path)-1
	);
	if(!view->letter)fi->type=TYPE_DISK;
	else if(strcmp(name,"..")==0)fi->type=TYPE_DIR;
	else lv_fs_get_type(&fi->type,fi->path);
	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT;

	// file item button
	fi->btn=lv_btn_create(view->view);
	lv_obj_set_width(fi->btn,lv_pct(100));
	lv_obj_set_content_height(fi->btn,grid_col[0]);
	lv_style_set_btn_item(fi->btn);
	lv_obj_add_event_cb(fi->btn,item_click,LV_EVENT_CLICKED,fi);
	lv_obj_add_event_cb(fi->btn,item_check,LV_EVENT_LONG_PRESSED,fi);
	lv_obj_set_grid_dsc_array(fi->btn,grid_col,grid_row);
	if(list_obj_add_new(&view->items,fi)!=0){
		telog_error("cannot add file item list");
		abort();
	}

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
		lv_img_set_src(fi->img,"inode-file"MIME_EXT);
	lv_img_set_size_mode(fi->img,LV_IMG_SIZE_MODE_REAL);
	lv_img_fill_image(fi->img,grid_col[0],grid_col[0]);
	lv_obj_center(fi->img);

	fi->fn=lv_label_create(fi->btn);
	lv_obj_set_small_text_font(fi->fn,0);
	lv_label_set_long_mode(fi->fn,lm);
	lv_label_set_text(fi->fn,strcmp(fi->name,"..")==0?_("Parent folder"):name);

	// add group
	if(view->grp)lv_group_add_obj(view->grp,fi->btn);

	uint8_t cs=2,rs=3;
	if(view->verbose){
		uint32_t s=0;
		lv_fs_file_t f;
		char size[32];
		memset(size,0,sizeof(size));
		if(
			fi->type==TYPE_FILE&&
			lv_fs_open(&f,fi->path,LV_FS_MODE_RD)==LV_FS_RES_OK
		){
			if(lv_fs_size(&f,&s)==LV_FS_RES_OK)
				make_readable_str_buf(size,sizeof(size)-1,s,1,0);
			lv_fs_close(&f);
		}
		if(size[0]){
			fi->size=lv_label_create(fi->btn);
			lv_label_set_text(fi->size,size);
			lv_label_set_long_mode(fi->size,lm);
			lv_obj_set_small_text_font(fi->size,0);
			cs=1;
			lv_obj_set_grid_cell(
				fi->size,
				LV_GRID_ALIGN_START,2,1,
				LV_GRID_ALIGN_CENTER,0,1
			);
		}

		#ifdef ENABLE_UEFI
		if(fi->type==TYPE_DISK){
			EFI_STATUS st;
			CHAR8 name[256];
			EFI_PARTITION_INFO_PROTOCOL*pi=NULL;
			EFI_HANDLE hand=fs_get_root_handle_by_letter(letter);
			EFI_FILE_PROTOCOL*root=fs_get_root_by_letter(letter);
			EFI_FILE_SYSTEM_INFO*info=NULL;
			if(hand){
				st=gBS->HandleProtocol(
					hand,
					&gEfiPartitionInfoProtocolGuid,
					(VOID**)&pi
				);
				if(!EFI_ERROR(st)&&pi->Type==PARTITION_TYPE_GPT){
					ZeroMem(name,sizeof(name));
					UnicodeStrToAsciiStrS(
						pi->Info.Gpt.PartitionName,
						name,sizeof(name)-1
					);
					// disk info1 (gpt partition name)
					fi->info1=lv_label_create(fi->btn);
					lv_label_set_text(fi->info1,name);
					lv_obj_set_small_text_font(fi->info1,LV_PART_MAIN);
					lv_obj_set_grid_cell(
						fi->info1,
						LV_GRID_ALIGN_START,1,2,
						LV_GRID_ALIGN_CENTER,1,1
					);
					rs--;
				}
			}

			st=efi_file_get_info(root,&gEfiFileSystemInfoGuid,NULL,(VOID**)&info);
			if(!EFI_ERROR(st)){
				uint8_t percent=0;
				size_t sx=sizeof(name)/2;
				char*b1=name,*b2=name+sx;
				UINT64 used=info->VolumeSize-info->FreeSpace;
				make_readable_str_buf(b1,sx,used,1,0);
				make_readable_str_buf(b2,sx,info->VolumeSize,1,0);
				if(info->VolumeSize>0)percent=used*100/info->VolumeSize;
				// disk info2 (disk used, disk size)
				fi->info2=lv_label_create(fi->btn);
				lv_label_set_text_fmt(
					fi->info2,
					"%s/%s (%d%%)",
					b1,b2,percent
				);
				lv_obj_set_small_text_font(fi->info2,LV_PART_MAIN);
				lv_obj_set_grid_cell(
					fi->info2,
					LV_GRID_ALIGN_START,1,2,
					LV_GRID_ALIGN_CENTER,2,1
				);
				rs--;
			}

		}
		#else
		int fd=open(view->path,O_DIR);
		if(fd>=0&&fstatat(fd,name,&fi->st,AT_SYMLINK_NOFOLLOW)==0){
			mode_t m=fi->st.st_mode;
			char times[64]={0};
			strftime(times,63,"%Y/%m/%d %H:%M:%S",localtime(&fi->st.st_mtime));

			// file info1 (time and permission)
			fi->info1=lv_label_create(fi->btn);
			lv_obj_set_small_text_font(fi->info1,LV_PART_MAIN);
			lv_label_set_text_fmt(fi->info1,"%s %s",times,mode_string(fi->st.st_mode));
			lv_obj_set_grid_cell(
				fi->info1,
				LV_GRID_ALIGN_START,1,2,
				LV_GRID_ALIGN_CENTER,1,1
			);

			dev_t d=fi->st.st_dev;
			char dev[32]={0};
			if(S_ISCHR(m))snprintf(dev,31,"char[%d:%d] ",major(d),minor(d));
			if(S_ISBLK(m))snprintf(dev,31,"block[%d:%d] ",major(d),minor(d));
			// symbolic link target
			char tgt[128]={0};
			if(S_ISLNK(m))readlinkat(fd,name,tgt,127);

			// file info2 (owner/group, device node and symbolic link target)
			char owner[128]={0},group[128]={0};
			fi->info2=lv_label_create(fi->btn);
			lv_obj_align_to(fi->info2,fi->info1,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
			lv_obj_set_small_text_font(fi->info2,LV_PART_MAIN);
			lv_label_set_text_fmt(
				fi->info2,
				"%s:%s %s%s",
				get_username(fi->st.st_uid,owner,127),
				get_groupname(fi->st.st_gid,group,127),
				dev,tgt
			);
			lv_obj_set_grid_cell(
				fi->info2,
				LV_GRID_ALIGN_START,1,2,
				LV_GRID_ALIGN_CENTER,2,1
			);
			rs=1;
			close(fd);
		}
		#endif
	}
	lv_obj_set_grid_cell(
		fi->fn,
		LV_GRID_ALIGN_STRETCH,1,cs,
		LV_GRID_ALIGN_CENTER,0,rs
	);
	view->count++;
	if(fsext_is_multi)fi->letter=letter;
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
	lv_obj_set_width(view->info,lv_obj_get_width(view->view)-gui_font_size);
	lv_obj_set_style_text_align(view->info,LV_TEXT_ALIGN_CENTER,0);
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
			add_item(view,name,x);
		}
		return;
	}
	if(view->parent&&!fileview_is_top(view))
		add_item(view,"..",0);
	lv_fs_dir_t dir;
	int i;
	char fn[256],*fp;
	char path[PATH_MAX+4]={0};
	if(view->path[1]==':'){
		view->letter=view->path[0];
		strncpy(
			path,
			view->path,
			sizeof(path)-1
		);
	}else snprintf(
		path,
		sizeof(path)-1,
		"%c:%s",
		view->letter,
		view->path
	);
	lv_fs_res_t res=lv_fs_dir_open(&dir,path);
	if(res!=LV_FS_RES_OK){
		telog_warn(
			"open folder %s failed (%s)",
			view->path,
			lv_fs_res_to_string(res)
		);
		set_info(
			view,
			_("open dir failed: %s"),
			lv_fs_res_to_i18n_string(res)
		);
		return;
	}
	for(i=0;i<1024;i++){
		fp=fn;
		if(lv_fs_dir_read(&dir,fn)!=LV_FS_RES_OK)break;
		if(strlen(fn)==0)break;
		if(fp[0]=='/')fp++;
		if(fp[0]=='.'&&!view->hidden)continue;
		add_item(view,fp,0);
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
		if(strcmp(view->path,"/")==0&&fsext_is_multi)view->letter=0;
		if((!view->letter&&!view->path[1])||!fsext_is_multi){
			strcpy(
				view->full_path,
				view->path
			);
		}else if(view->path[1]==':'){
			view->letter=view->path[0];
			strncpy(
				view->full_path,
				view->path,
				sizeof(view->full_path)-1
			);
		}else snprintf(
			view->full_path,
			sizeof(view->full_path)-1,
			"%c:%s",
			view->letter,
			view->path
		);
		call_on_change_dir(view,oldpath);
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
	uint16_t checked=fileview_get_checked_count(view),num=0;
	size_t size=sizeof(char*)*(checked+1);
	char**arr=malloc(size);
	if(!arr)return NULL;
	memset(arr,0,size);
	if(checked>0){
		list*l=list_first(view->items);
		if(l)do{
			LIST_DATA_DECLARE(ffi,l,struct fileitem*);
			if(ffi&&lv_obj_is_checked(ffi->btn))
				arr[num++]=ffi->name;
		}while((l=l->next));
	}
	return arr;
}

void*fileview_get_data(struct fileview*view){
	return view?view->data:NULL;
}

char*fileview_get_path(struct fileview*view){
	return view?view->full_path:NULL;
}

char*fileview_get_lvgl_path(struct fileview*view){
	if(!view)return NULL;
	static char path[PATH_MAX];
	memset(path,0,sizeof(path));
	if(!view->letter)strcpy(path,"/");
	else if(view->path[1]==':')strncpy(path,view->path,sizeof(path)-1);
	else snprintf(path,sizeof(path)-1,"%c:%s",view->letter,view->path);
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

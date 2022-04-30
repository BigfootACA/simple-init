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
	lv_obj_t*view,*info,*last_btn;
	list*items;
	bool hidden,parent,verbose;
	int count;
	lv_coord_t bw,bh,margin;
	lv_group_t*grp;
	fileview_on_item_select on_select_item;
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

static void click_item(struct fileitem*fi){
	if(!fi)return;
	struct fileview*fv=fi->view;
	if(!fv)return;
	if(fi->letter){
		fv->letter=fi->letter;
		fileview_set_path(fv,(char[]){fi->letter,':','/',0});
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

static void item_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	click_item((struct fileitem*)lv_obj_get_user_data(obj));
}

static void check_item(struct fileitem*fi,bool checked){
	if(!fi||!fi->view||!fi->chk)return;
	if(strcmp(fi->name,"..")==0){
		fileview_go_back(fi->view);
		return;
	}
	if(fi->letter){
		lv_checkbox_set_checked(fi->chk,false);
		return;
	}
	lv_checkbox_set_checked(fi->chk,checked);
	lv_obj_set_checked(fi->btn,checked);
	call_on_select_item(
		fi->view,fi->name,fi->type,checked,
		fileview_get_checked_count(fi->view)
	);
}

static void item_check(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	struct fileitem*fi=(struct fileitem*)lv_obj_get_user_data(obj);
	if(!fi||!fi->chk)return;
	check_item(fi,lv_checkbox_is_checked(fi->chk));
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
	if(view->bw==0)view->bw=lv_page_get_scrl_width(view->view)-view->margin;
	if(view->bh==0)view->bh=gui_dpi/2;
	struct fileitem*fi=malloc(sizeof(struct fileitem));
	if(!fi){
		telog_error("cannot allocate fileitem");
		abort();
	}
	memset(fi,0,sizeof(struct fileitem));
	fi->view=view;
	strncpy(fi->name,name,sizeof(fi->name)-1);
	snprintf(
		fi->path,sizeof(fi->path)-1,"%s%s/%s",
		view->path[1]==':'?"":(char[]){view->letter,':',0},
		view->path,fi->name
	);
	if(!view->letter)fi->type=TYPE_DISK;
	else if(strcmp(name,"..")==0)fi->type=TYPE_DIR;
	else lv_fs_get_type(&fi->type,fi->path);
	lv_label_long_mode_t lm=confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT;

	// file item button
	fi->btn=lv_btn_create(view->view,NULL);
	lv_obj_set_size(fi->btn,view->bw,view->bh);
	lv_style_set_btn_item(fi->btn);
	lv_obj_set_click(fi->btn,false);
	lv_obj_align(
		fi->btn,view->last_btn,view->last_btn?
			LV_ALIGN_OUT_BOTTOM_LEFT:
			LV_ALIGN_IN_TOP_MID,
		0,view->margin/8+(view->last_btn?gui_dpi/20:0)
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
	lv_img_ext_t*x=lv_obj_get_ext_attr(fi->img);
	lv_img_set_src(fi->img,get_icon(fi));
	if(x->w<=0||x->h<=0)
		lv_img_set_src(fi->img,"inode-file"MIME_EXT);
	if(x->w>0&&x->h>0)lv_img_set_zoom(fi->img,(int)(((float)si/MAX(x->w,x->h))*256));
	lv_img_set_pivot(fi->img,0,0);

	// file name and checkbox
	fi->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(fi->chk,strcmp(fi->name,"..")==0?_("Parent folder"):name);
	lv_style_set_focus_checkbox(fi->chk);
	lv_obj_set_event_cb(fi->chk,item_check);
	lv_obj_set_user_data(fi->chk,fi);
	lv_obj_align(fi->chk,NULL,LV_ALIGN_IN_LEFT_MID,gui_font_size+si,view->verbose?-gui_font_size:0);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(fi->chk);
	lv_label_set_long_mode(e->label,lm);

	// add group
	if(view->grp){
		lv_group_add_obj(view->grp,fi->img);
		lv_group_add_obj(view->grp,fi->chk);
	}

	lv_coord_t lbl_w=view->bw-si-gui_font_size*2-lv_obj_get_width(e->bullet);
	if(lv_obj_get_width(e->label)>lbl_w)lv_obj_set_width(e->label,lbl_w);

	if(view->verbose){
		if(fi->type==TYPE_FILE){
			lv_fs_file_t f;
			uint32_t s=0;
			if(lv_fs_open(&f,fi->path,LV_FS_MODE_RD)==LV_FS_RES_OK){
				if(lv_fs_size(&f,&s)==LV_FS_RES_OK){
					char size[32]={0};
					fi->size=lv_label_create(line,NULL);
					lv_label_set_text(fi->size,make_readable_str_buf(size,31,s,1,0));
					lv_label_set_long_mode(fi->size,lm);
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
					lbl_w-=xs+gui_dpi/20;
					if(lv_obj_get_width(e->label)>lbl_w)
						lv_obj_set_width(e->label,lbl_w);
				}
				lv_fs_close(&f);
			}
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
					fi->info1=lv_label_create(line,NULL);
					lv_label_set_text(fi->info1,name);
					lv_obj_set_small_text_font(fi->info1,LV_LABEL_PART_MAIN);
					lv_obj_align(
						fi->info1,fi->chk,
						LV_ALIGN_OUT_BOTTOM_LEFT,
						0,gui_font_size
					);
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
				fi->info2=lv_label_create(line,NULL);
				lv_label_set_text_fmt(
					fi->info2,
					"%s/%s (%d%%)",
					b1,b2,percent
				);
				lv_obj_set_small_text_font(fi->info2,LV_LABEL_PART_MAIN);
				lv_obj_align_y(
					fi->info2,fi->chk,
					LV_ALIGN_OUT_BOTTOM_RIGHT,
					gui_font_size
				);
				lv_obj_align_x(
					fi->info2,fi->btn,
					LV_ALIGN_OUT_BOTTOM_RIGHT,
					-gui_font_size
				);
			}

		}
		#else
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
			close(fd);
		}
		#endif
	}
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
	view->last_btn=NULL,view->items=NULL;
	view->info=NULL,view->count=0;
	call_on_select_item(view,NULL,0,false,0);
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
	view->margin=gui_font_size;
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

void fileview_set_on_item_select(struct fileview*view,fileview_on_item_select cb){
	view->on_select_item=cb;
}

void fileview_set_data(struct fileview*view,void*data){
	if(view)view->data=data;
}

void fileview_set_margin(struct fileview*view,lv_coord_t margin){
	if(view)view->margin=margin;
}

void fileview_set_item_height(struct fileview*view,lv_coord_t height){
	if(view)view->bh=height;
}

void fileview_set_verbose(struct fileview*view,bool verbose){
	if(view)view->verbose=verbose;
}

uint16_t fileview_get_checked_count(struct fileview*view){
	uint16_t checked=0;
	list*l=list_first(view->items);
	if(l)do{
		LIST_DATA_DECLARE(ffi,l,struct fileitem*);
		if(ffi&&lv_checkbox_is_checked(ffi->chk))checked++;
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
			if(ffi&&lv_checkbox_is_checked(ffi->chk))arr[num++]=ffi->name;
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

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include"gui.h"
#include"aboot.h"
#include"logger.h"
#include"gui/fsext.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "abootimg"

struct abootimg{
	lv_obj_t*box,*ok,*cancel;
	lv_obj_t*opt_create,*opt_update,*opt_extract;
	lv_obj_t*btn_image,*btn_kernel,*btn_initrd,*btn_second;
	lv_obj_t*clr_image,*clr_kernel,*clr_initrd,*clr_second,*clr_cmdline;
	lv_obj_t*image,*kernel,*initrd,*second;
	lv_obj_t*name,*cmdline;
	aboot_image*img;
};

#ifdef ENABLE_UEFI
#define ABOOT_WRAP(path,call,ret,...){ \
                lv_fs_file_t f;\
		lv_fs_res_t res=lv_fs_open(&f,path,LV_FS_MODE_WR);\
		if(res==LV_FS_RES_OK){\
			ret=call##_fp(__VA_ARGS__ lv_fs_file_to_fp(&f));\
			lv_fs_close(&f);\
		}else tlog_warn(\
			"open file '%s' failed: %s",\
			path,lv_fs_res_to_string(res)\
		);\
	}
#else
#define ABOOT_WRAP(path,call,ret,...){\
		int fd=open(path,O_RDWR|O_CREAT,0644);\
                if(fd<0)fd=open(path,O_RDONLY);\
		if(fd>=0){\
			ret=call##_fd(__VA_ARGS__ fd);\
			close(fd);\
		}else telog_warn("open file '%s' failed",path);\
	}
#endif

static int abootimg_get_focus(struct gui_activity*d){
	struct abootimg*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->opt_create);
	lv_group_add_obj(gui_grp,am->opt_update);
	lv_group_add_obj(gui_grp,am->opt_extract);
	lv_group_add_obj(gui_grp,am->name);
	lv_group_add_obj(gui_grp,am->clr_image);
	lv_group_add_obj(gui_grp,am->image);
	lv_group_add_obj(gui_grp,am->btn_image);
	lv_group_add_obj(gui_grp,am->clr_kernel);
	lv_group_add_obj(gui_grp,am->kernel);
	lv_group_add_obj(gui_grp,am->btn_kernel);
	lv_group_add_obj(gui_grp,am->clr_initrd);
	lv_group_add_obj(gui_grp,am->initrd);
	lv_group_add_obj(gui_grp,am->btn_initrd);
	lv_group_add_obj(gui_grp,am->clr_second);
	lv_group_add_obj(gui_grp,am->second);
	lv_group_add_obj(gui_grp,am->btn_second);
	lv_group_add_obj(gui_grp,am->clr_cmdline);
	lv_group_add_obj(gui_grp,am->cmdline);
	lv_group_add_obj(gui_grp,am->ok);
	lv_group_add_obj(gui_grp,am->cancel);
	return 0;
}

static int abootimg_lost_focus(struct gui_activity*d){
	struct abootimg*am=d->data;
	if(!am)return 0;
	lv_group_remove_obj(am->opt_create);
	lv_group_remove_obj(am->opt_update);
	lv_group_remove_obj(am->opt_extract);
	lv_group_remove_obj(am->name);
	lv_group_remove_obj(am->clr_image);
	lv_group_remove_obj(am->image);
	lv_group_remove_obj(am->btn_image);
	lv_group_remove_obj(am->clr_kernel);
	lv_group_remove_obj(am->kernel);
	lv_group_remove_obj(am->btn_kernel);
	lv_group_remove_obj(am->clr_initrd);
	lv_group_remove_obj(am->initrd);
	lv_group_remove_obj(am->btn_initrd);
	lv_group_remove_obj(am->clr_second);
	lv_group_remove_obj(am->second);
	lv_group_remove_obj(am->btn_second);
	lv_group_remove_obj(am->clr_cmdline);
	lv_group_remove_obj(am->cmdline);
	lv_group_remove_obj(am->ok);
	lv_group_remove_obj(am->cancel);
	return 0;
}

static int open_image(struct abootimg*am){
	if(am->img)abootimg_free(am->img);
	const char*path=lv_textarea_get_text(am->image);
	ABOOT_WRAP(path,abootimg_load_from,am->img);
	return 0;
}

static int auto_open_image(struct abootimg*am){
	size_t s;
	char path[PATH_MAX];
	int r=open_image(am);
	if(am->img){
		if(!lv_textarea_get_text(am->name)[0])
			lv_textarea_set_text(am->name,abootimg_get_name(am->img));
		if(!lv_textarea_get_text(am->cmdline)[0])
			lv_textarea_set_text(am->cmdline,abootimg_get_cmdline(am->img));
		if(lv_checkbox_is_checked(am->opt_extract)){
			lv_textarea_set_text(am->name,abootimg_get_name(am->img));
			lv_textarea_set_text(am->cmdline,abootimg_get_cmdline(am->img));
			memset(path,0,sizeof(path));
			strncpy(path,lv_textarea_get_text(am->image),sizeof(path)-1);
			for(s=strlen(path);s>0;s--){
				if(path[s]=='/')break;
				path[s]=0;
			}
			s=strlen(path);
			if(abootimg_have_kernel(am->img)&&!lv_textarea_get_text(am->kernel)[0]){
				strcat(path,"zImage");
				lv_textarea_set_text(am->kernel,path);
				path[s]=0;
			}
			if(abootimg_have_ramdisk(am->img)&&!lv_textarea_get_text(am->initrd)[0]){
				strcat(path,"initrd.img");
				lv_textarea_set_text(am->initrd,path);
				path[s]=0;
			}
			if(abootimg_have_second(am->img)&&!lv_textarea_get_text(am->second)[0]){
				strcat(path,"stage2.img");
				lv_textarea_set_text(am->second,path);
				path[s]=0;
			}
		}
	}
	return r;
}

static void abootimg_cb(lv_obj_t*obj,lv_event_t e){
	struct abootimg*am=lv_obj_get_user_data(obj);
	lv_input_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	if(!am||obj!=am->image)return;
	auto_open_image(am);
}

#define DO_SAVE_FILE(tag,type)\
	static bool do_save_##tag(struct abootimg*am){\
		if(!am||!am->img)return false;\
		if(!abootimg_have_##tag(am->img))return true;\
		bool ret=false;\
		const char*path=lv_textarea_get_text(am->type);\
                if(!path[0])return true;\
		ABOOT_WRAP(path,abootimg_save_##tag##_to,ret,am->img,);\
		if(!ret)msgbox_alert("Save "#tag" failed");\
		return ret;\
	}\
	static bool do_load_##tag(struct abootimg*am){\
		if(!am||!am->img)return false;\
		bool ret=false;\
		const char*path=lv_textarea_get_text(am->type);\
		if(!path[0])return true;\
		ABOOT_WRAP(path,abootimg_load_##tag##_from,ret,am->img,);\
		if(!ret)msgbox_alert("Load "#tag" failed");\
		return ret;\
	}

DO_SAVE_FILE(kernel,kernel)
DO_SAVE_FILE(ramdisk,initrd)
DO_SAVE_FILE(second,second)

static bool image_open(struct abootimg*am){
	if(!am->img)open_image(am);
	if(!am->img){
		msgbox_alert("Load image failed");
		return false;
	}
	return true;
}

static bool image_save(struct abootimg*am){
	bool ret=false;
	const char*path=lv_textarea_get_text(am->image);
	if(!path[0])return true;
	ABOOT_WRAP(path,abootimg_save_to,ret,am->img,);
	if(!ret)msgbox_alert("Save image failed");
	return ret;
}

static void image_create(struct abootimg*am){
	char*buf;
	if(am->img)abootimg_free(am->img);
	if(!(am->img=abootimg_new_image())){
		msgbox_alert("Create new image failed");
		return;
	}
	if((buf=(char*)lv_textarea_get_text(am->name))&&buf[0])
		abootimg_set_name(am->img,buf);
	if((buf=(char*)lv_textarea_get_text(am->cmdline))&&buf[0])
		abootimg_set_cmdline(am->img,buf);
	if(!do_load_kernel(am))return;
	if(!do_load_ramdisk(am))return;
	if(!do_load_second(am))return;
	if(!image_save(am))return;
	msgbox_alert("Operation done");
}

static void image_update(struct abootimg*am){
	char*buf;
	if(!image_open(am))return;
	if((buf=(char*)lv_textarea_get_text(am->name))&&buf[0])
		abootimg_set_name(am->img,buf);
	if((buf=(char*)lv_textarea_get_text(am->cmdline))&&buf[0])
		abootimg_set_cmdline(am->img,buf);
	if(!do_load_kernel(am))return;
	if(!do_load_ramdisk(am))return;
	if(!do_load_second(am))return;
	if(!image_save(am))return;
	msgbox_alert("Operation done");
}

static void image_extract(struct abootimg*am){
	if(!image_open(am))return;
	if(!do_save_kernel(am))return;
	if(!do_save_ramdisk(am))return;
	if(!do_save_second(am))return;
	msgbox_alert("Operation done");
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct abootimg*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->ok)return;
	if(lv_checkbox_is_checked(am->opt_create))image_create(am);
	else if(lv_checkbox_is_checked(am->opt_update))image_update(am);
	else if(lv_checkbox_is_checked(am->opt_extract))image_extract(am);
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct abootimg*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->cancel)return;
	guiact_do_back();
}

static int init(struct gui_activity*act){
	struct abootimg*am=malloc(sizeof(struct abootimg));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct abootimg));
	act->data=am;
	#ifndef ENABLE_UEFI
	char*p=act->args;
	if(p&&p[0]!='/'){
		if(p[1]!=':')return -EINVAL;
		act->args+=2;
	}
	#endif
	return 0;
}

static void chk_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	struct abootimg*am=lv_obj_get_user_data(obj);
	if(!am)return;
	lv_checkbox_set_checked(am->opt_create,false);
	lv_checkbox_set_checked(am->opt_update,false);
	lv_checkbox_set_checked(am->opt_extract,false);
	lv_checkbox_set_checked(obj,true);
}

static int do_cleanup(struct gui_activity*act){
	struct abootimg*am=act->data;
	if(!am)return 0;
	if(am->img)abootimg_free(am->img);
	free(am);
	act->data=NULL;
	return 0;
}

static int draw_abootimg(struct gui_activity*act){
	struct abootimg*am=act->data;

	am->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(am->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(am->box,gui_sw/8*7);
	lv_obj_set_click(am->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(am->box);

	// Title
	lv_obj_t*title=lv_label_create(am->box,NULL);
	lv_label_set_text(title,_("Android Boot Image Tools"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	h+=gui_font_size;
	am->opt_create=lv_checkbox_create(am->box,NULL);
	lv_checkbox_set_text(am->opt_create,_("Create"));
	lv_checkbox_set_checked(am->opt_create,false);
	lv_obj_set_user_data(am->opt_create,am);
	lv_obj_set_event_cb(am->opt_create,chk_cb);
	lv_style_set_focus_radiobox(am->opt_create);
	lv_obj_align(am->opt_create,NULL,LV_ALIGN_IN_TOP_LEFT,gui_font_size/2,0);
	lv_obj_set_y(am->opt_create,h);

	am->opt_update=lv_checkbox_create(am->box,NULL);
	lv_checkbox_set_text(am->opt_update,_("Update"));
	lv_checkbox_set_checked(am->opt_update,false);
	lv_obj_set_user_data(am->opt_update,am);
	lv_obj_set_event_cb(am->opt_update,chk_cb);
	lv_style_set_focus_radiobox(am->opt_update);
	lv_obj_align(am->opt_update,NULL,LV_ALIGN_IN_TOP_RIGHT,-(gui_font_size*2),0);
	lv_obj_set_y(am->opt_update,h);
	h+=lv_obj_get_height(am->opt_update);

	h+=gui_font_size/2;
	am->opt_extract=lv_checkbox_create(am->box,NULL);
	lv_checkbox_set_text(am->opt_extract,_("Extract"));
	lv_checkbox_set_checked(am->opt_extract,true);
	lv_obj_set_user_data(am->opt_extract,am);
	lv_obj_set_event_cb(am->opt_extract,chk_cb);
	lv_style_set_focus_radiobox(am->opt_extract);
	lv_obj_align(am->opt_extract,NULL,LV_ALIGN_IN_TOP_LEFT,gui_font_size/2,0);
	lv_obj_set_y(am->opt_extract,h);
	h+=lv_obj_get_height(am->opt_extract);

	// Image name
	lv_obj_t*name=lv_draw_title(am->box,"Name:",&h);

	am->name=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->name,"");
	lv_textarea_set_one_line(am->name,true);
	lv_textarea_set_cursor_hidden(am->name,true);
	lv_obj_set_user_data(am->name,am);
	lv_obj_set_event_cb(am->name,lv_input_cb);
	lv_obj_set_width(am->name,w-lv_obj_get_width(name)-gui_font_size);
	lv_obj_align(am->name,name,LV_ALIGN_OUT_RIGHT_MID,gui_font_size/2,0);
	lv_obj_set_y(am->name,h);
	lv_obj_align(name,am->name,LV_ALIGN_OUT_LEFT_MID,-gui_font_size/2,0);
	h+=lv_obj_get_height(am->name);

	lv_draw_file_input(am->box,"Android Boot Image",     &h,&am->clr_image, &am->image, &am->btn_image);
	lv_draw_file_input(am->box,"Linux Kernel",           &h,&am->clr_kernel,&am->kernel,&am->btn_kernel);
	lv_draw_file_input(am->box,"Ramdisk (initramfs)",    &h,&am->clr_initrd,&am->initrd,&am->btn_initrd);
	lv_draw_file_input(am->box,"Second Stage Bootloader",&h,&am->clr_second,&am->second,&am->btn_second);

	lv_obj_set_user_data(am->image,am);
	lv_obj_set_event_cb(am->image,abootimg_cb);

	// Commandline
	lv_obj_t*cmdline=lv_draw_title(am->box,"Kernel Commandline",&h);
	am->clr_cmdline=lv_draw_side_clear_btn(am->box,&h,lv_obj_get_height(cmdline));

	h+=gui_font_size/2;
	am->cmdline=lv_textarea_create(am->box,NULL);
	lv_obj_set_user_data(am->clr_cmdline,am->cmdline);
	lv_textarea_set_text(am->cmdline,"");
	lv_textarea_set_cursor_hidden(am->cmdline,true);
	lv_obj_set_user_data(am->cmdline,am);
	lv_obj_set_event_cb(am->cmdline,lv_input_cb);
	lv_obj_set_width(am->cmdline,w);
	lv_obj_set_pos(am->cmdline,0,h);
	h+=lv_obj_get_height(am->cmdline);

	// OK Button
	h+=gui_font_size;
	am->ok=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->ok,true);
	lv_obj_set_size(am->ok,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(am->ok,NULL,LV_ALIGN_IN_TOP_LEFT,(gui_font_size/2),h);
	lv_obj_set_user_data(am->ok,am);
	lv_obj_set_event_cb(am->ok,ok_cb);
	lv_label_set_text(lv_label_create(am->ok,NULL),_("OK"));

	// Cancel Button
	am->cancel=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->cancel,true);
	lv_obj_set_size(am->cancel,w/2-gui_font_size,gui_font_size*2);
	lv_obj_align(am->cancel,NULL,LV_ALIGN_IN_TOP_RIGHT,-(gui_font_size/2),h);
	lv_obj_set_user_data(am->cancel,am);
	lv_obj_set_event_cb(am->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(am->cancel,NULL),_("Cancel"));
	h+=lv_obj_get_height(am->cancel);

	h+=gui_font_size*3;
	lv_obj_set_height(am->box,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(am->box,NULL,LV_ALIGN_CENTER,0,0);

	if(act->args){
		lv_textarea_set_text(am->image,act->args);
		auto_open_image(am);
	}
	return 0;
}

struct gui_register guireg_abootimg={
	.name="abootimg",
	.title="Android Boot Image",
	.icon="abootimg.svg",
	.show_app=true,
	.open_file=true,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=abootimg_get_focus,
	.lost_focus=abootimg_lost_focus,
	.draw=draw_abootimg,
	.back=true,
	.mask=true,
};
#endif

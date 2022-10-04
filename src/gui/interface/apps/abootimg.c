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
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
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
	am->img=abootimg_load_from_url_path(path);
	return am->img==NULL?(errno?:EINVAL):0;
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
		if(lv_obj_is_checked(am->opt_extract)){
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

static void abootimg_cb(lv_event_t*e){
	auto_open_image(e->user_data);
}

#define DO_SAVE_FILE(tag,type)\
	static bool do_save_##tag(struct abootimg*am){\
		if(!am||!am->img)return false;\
		if(!abootimg_have_##tag(am->img))return true;\
		bool ret=false;\
		const char*path=lv_textarea_get_text(am->type);\
                if(!path[0])return true;\
                ret=abootimg_save_##tag##_to_url_path(am->img,path);\
		if(!ret)msgbox_alert("Save "#tag" failed");\
		return ret;\
	}\
	static bool do_load_##tag(struct abootimg*am){\
		if(!am||!am->img)return false;\
		bool ret=false;\
		const char*path=lv_textarea_get_text(am->type);\
		if(!path[0])return true;\
                ret=abootimg_load_##tag##_from_url_path(am->img,path);\
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
	ret=abootimg_save_to_url_path(am->img,path);
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

static void ok_cb(lv_event_t*e){
	struct abootimg*am=e->user_data;
	if(lv_obj_has_flag(am->opt_create,LV_STATE_CHECKED))image_create(am);
	else if(lv_obj_has_flag(am->opt_update,LV_STATE_CHECKED))image_update(am);
	else if(lv_obj_has_flag(am->opt_extract,LV_STATE_CHECKED))image_extract(am);
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

static void chk_cb(lv_event_t*e){
	struct abootimg*am=e->user_data;
	lv_obj_set_checked(am->opt_create,false);
	lv_obj_set_checked(am->opt_update,false);
	lv_obj_set_checked(am->opt_extract,false);
	lv_obj_set_checked(e->target,true);
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
	am->box=lv_draw_dialog_box(act->page,NULL,"Android Boot Image Tools");
	am->opt_create=lv_draw_checkbox(am->box,_("Create"),false,chk_cb,am);
	am->opt_update=lv_draw_checkbox(am->box,_("Update"),false,chk_cb,am);
	am->opt_extract=lv_draw_checkbox(am->box,_("Extract"),true,chk_cb,am);
	lv_draw_input(am->box, "Name",                    NULL, NULL,             &am->name,    NULL);
	lv_draw_input(am->box, "Android Boot Image",      NULL, &am->clr_image,   &am->image,   &am->btn_image);
	lv_draw_input(am->box, "Linux Kernel",            NULL, &am->clr_kernel,  &am->kernel,  &am->btn_kernel);
	lv_draw_input(am->box, "Ramdisk (initramfs)",     NULL, &am->clr_initrd,  &am->initrd,  &am->btn_initrd);
	lv_draw_input(am->box, "Second Stage Bootloader", NULL, &am->clr_second,  &am->second,  &am->btn_second);
	lv_draw_input(am->box, "Kernel Commandline",      NULL, &am->clr_cmdline, &am->cmdline, NULL);
	lv_obj_add_event_cb(am->image,abootimg_cb,LV_EVENT_DEFOCUSED,am);
	lv_textarea_set_one_line(am->cmdline,false);
	lv_draw_btns_ok_cancel(am->box,&am->ok,&am->cancel,ok_cb,am);
	if(act->args){
		lv_textarea_set_text(am->image,act->args);
		auto_open_image(am);
	}
	return 0;
}

struct gui_register guireg_abootimg={
	.name="abootimg",
	.title="Android Boot Image",
	.show_app=true,
	.open_file=true,
	.open_regex=(char*[]){
		"^/dev/.+",
		"file:///dev/.+",
		".*\\.img$",
		".*/boot.*",
		".*/recovery.*",
		".*/twrp.*",
		NULL
	},
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=abootimg_get_focus,
	.lost_focus=abootimg_lost_focus,
	.draw=draw_abootimg,
	.back=true,
	.mask=true,
};
#endif

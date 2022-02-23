/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#define _GNU_SOURCE
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<Library/BaseLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"linux_boot.h"
#include"gui/fsext.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "linux-boot"

static const char*base="gui.boot_linux";
static linux_config*boot_cfg=NULL;
struct boot_linux{
	lv_obj_t*box,*ok,*cancel;
	lv_obj_t*btn_abootimg,*btn_kernel,*btn_initrd,*btn_dtb;
	lv_obj_t*clr_abootimg,*clr_kernel,*clr_initrd,*clr_dtb,*clr_cmdline;
	lv_obj_t*abootimg,*kernel,*initrd,*dtb;
	lv_obj_t*cmdline,*use_uefi;
};

static int boot_linux_get_focus(struct gui_activity*d){
	struct boot_linux*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->clr_abootimg);
	lv_group_add_obj(gui_grp,am->abootimg);
	lv_group_add_obj(gui_grp,am->btn_abootimg);
	lv_group_add_obj(gui_grp,am->clr_kernel);
	lv_group_add_obj(gui_grp,am->kernel);
	lv_group_add_obj(gui_grp,am->btn_kernel);
	lv_group_add_obj(gui_grp,am->clr_initrd);
	lv_group_add_obj(gui_grp,am->initrd);
	lv_group_add_obj(gui_grp,am->btn_initrd);
	lv_group_add_obj(gui_grp,am->clr_dtb);
	lv_group_add_obj(gui_grp,am->dtb);
	lv_group_add_obj(gui_grp,am->btn_dtb);
	lv_group_add_obj(gui_grp,am->clr_cmdline);
	lv_group_add_obj(gui_grp,am->cmdline);
	lv_group_add_obj(gui_grp,am->use_uefi);
	lv_group_add_obj(gui_grp,am->ok);
	lv_group_add_obj(gui_grp,am->cancel);
	return 0;
}

static int boot_linux_lost_focus(struct gui_activity*d){
	struct boot_linux*am=d->data;
	if(!am)return 0;
	lv_group_remove_obj(am->clr_abootimg);
	lv_group_remove_obj(am->abootimg);
	lv_group_remove_obj(am->btn_abootimg);
	lv_group_remove_obj(am->clr_kernel);
	lv_group_remove_obj(am->kernel);
	lv_group_remove_obj(am->btn_kernel);
	lv_group_remove_obj(am->clr_initrd);
	lv_group_remove_obj(am->initrd);
	lv_group_remove_obj(am->btn_initrd);
	lv_group_remove_obj(am->clr_dtb);
	lv_group_remove_obj(am->dtb);
	lv_group_remove_obj(am->btn_dtb);
	lv_group_remove_obj(am->clr_cmdline);
	lv_group_remove_obj(am->cmdline);
	lv_group_remove_obj(am->use_uefi);
	lv_group_remove_obj(am->ok);
	lv_group_remove_obj(am->cancel);
	return 0;
}

extern int guiapp_main(int argc,char**argv);

static void do_boot(){
	linux_boot*boot;
	if(!boot_cfg)return;
	if(!(boot=linux_boot_new(boot_cfg)))return;
	if((linux_boot_prepare(boot))!=0)return;
	if((linux_boot_execute(boot))!=0)return;
	linux_boot_free(boot);
	return;
}

static int after_exit(void*d __attribute__((unused))){
	gui_run=true;
	logger_set_console(confd_get_boolean("boot.console_log",true));
	do_boot();
	logger_set_console(false);
	tlog_warn("boot failed");
	gBS->Stall(10*1000*1000);
	lv_obj_invalidate(lv_scr_act());
	return guiapp_main(1,(char*[]){"guiapp",NULL});
}

static bool set_path(lv_obj_t*ta,linux_load_from*from,const char*key){
	lv_fs_res_t r;
	lv_fs_file_t f;
	const char*txt;
	if(!ta||!from)return false;
	if(!(txt=lv_textarea_get_text(ta)))return false;
	if(!txt[0])return true;
	confd_set_string_base(base,key,(char*)txt);
	r=lv_fs_open(&f,txt,LV_FS_MODE_RD);
	if(r!=LV_FS_RES_OK){
		msgbox_alert(
			"Load file '%s' failed: %s",
			txt,lv_fs_res_to_i18n_string(r)
		);
		tlog_warn(
			"load file '%s' failed: %s",
			txt,lv_fs_res_to_string(r)
		);
		return false;
	}
	from->enabled=true;
	from->type=FROM_FILE_PROTOCOL;
	from->file_proto=lv_fs_file_to_fp(&f);
	tlog_debug(
		"loaded %s from '%s' as file protocol %p",
		key,txt,from->file_proto
	);
	return true;
}


static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct boot_linux*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->ok)return;
	linux_config*cfg=linux_config_new();
	if(!cfg)return;
	AsciiStrCpyS(cfg->tag,sizeof(cfg->tag)-1,"<FROM_GUI>");
	set_path(am->abootimg,&cfg->abootimg,"abootimg");
	set_path(am->kernel,&cfg->kernel,"kernel");
	set_path(am->initrd,&cfg->initrd,"initrd");
	set_path(am->dtb,&cfg->dtb,"dtb");
	const char*cmdline=lv_textarea_get_text(am->cmdline);
	if(cmdline&&cmdline[0]){
		confd_set_string_base(base,"cmdline",(char*)cmdline);
		strncpy(cfg->cmdline,cmdline,sizeof(cfg->cmdline)-1);
	}
	cfg->use_uefi=lv_checkbox_is_checked(am->use_uefi);
	confd_set_boolean_base(base,"use_uefi",cfg->use_uefi);
	if(!cfg->abootimg.enabled&&!cfg->kernel.enabled){
		msgbox_alert("No kernel specified");
		linux_config_free(cfg);
		return;
	}
	if(cfg->abootimg.enabled&&cfg->kernel.enabled){
		msgbox_alert("Conflict options");
		linux_config_free(cfg);
		return;
	}
	boot_cfg=cfg;
	gui_run_and_exit(after_exit);
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct boot_linux*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->cancel)return;
	guiact_do_back();
}

static int do_cleanup(struct gui_activity*act){
	struct boot_linux*am=act->data;
	if(!am)return 0;
	free(am);
	act->data=NULL;
	return 0;
}

static lv_obj_t*draw_title(struct boot_linux*am,char*title,lv_coord_t*h){
	(*h)+=gui_font_size;
	lv_obj_t*label=lv_label_create(am->box,NULL);
	lv_label_set_text(label,_(title));
	lv_obj_set_y(label,(*h));
	return label;
}

static int draw_boot_linux(struct gui_activity*act){
	char path[PATH_MAX];
	struct boot_linux*am=malloc(sizeof(struct boot_linux));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct boot_linux));
	act->data=am;

	am->box=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_pad_all(am->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,gui_font_size);
	lv_obj_set_width(am->box,gui_sw/8*7);
	lv_obj_set_click(am->box,false);
	lv_coord_t h=0;
	lv_coord_t w=lv_page_get_scrl_width(am->box);

	// Title
	lv_obj_t*title=lv_label_create(am->box,NULL);
	lv_label_set_text(title,_("Boot Linux"));
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(title,w);
	lv_obj_set_y(title,h);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	h+=lv_obj_get_height(title);

	lv_draw_file_input(am->box,"Android Boot Image",     &h,&am->clr_abootimg,&am->abootimg,&am->btn_abootimg);
	lv_draw_file_input(am->box,"Linux Kernel",           &h,&am->clr_kernel,  &am->kernel,  &am->btn_kernel);
	lv_draw_file_input(am->box,"Ramdisk (initramfs)",    &h,&am->clr_initrd,  &am->initrd,  &am->btn_initrd);
	lv_draw_file_input(am->box,"Device Tree Blob (dtb)", &h,&am->clr_dtb,     &am->dtb,     &am->btn_dtb);

	// Commandline
	lv_obj_t*cmdline=draw_title(am,"Kernel Commandline",&h);
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

	h+=gui_font_size;
	am->use_uefi=lv_checkbox_create(am->box,NULL);
	lv_style_set_focus_checkbox(am->use_uefi);
	lv_obj_align(am->use_uefi,NULL,LV_ALIGN_IN_TOP_LEFT,(gui_font_size/2),h);
	lv_obj_set_user_data(am->use_uefi,am);
	lv_checkbox_set_text(am->use_uefi,_("Boot with UEFI runtime"));
	h+=lv_obj_get_height(am->use_uefi);

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

	#define set_text(tag) \
	lv_textarea_set_text(\
		am->tag,\
		confd_get_sstring_base(\
			base,#tag,"",\
			path,sizeof(path)\
		)\
	);
	set_text(abootimg);
	set_text(kernel);
	set_text(initrd);
	set_text(dtb);
	set_text(cmdline);
	lv_checkbox_set_checked(
		am->use_uefi,
		confd_get_boolean_base(
			base,"use_uefi",false
		)
	);
	return 0;
}

struct gui_register guireg_boot_linux={
	.name="boot-linux",
	.title="Boot Linux",
	.icon="linux.svg",
	.show_app=true,
	.open_file=false,
	.quiet_exit=do_cleanup,
	.get_focus=boot_linux_get_focus,
	.lost_focus=boot_linux_lost_focus,
	.draw=draw_boot_linux,
	.back=true,
	.mask=true,
};
#endif
#endif

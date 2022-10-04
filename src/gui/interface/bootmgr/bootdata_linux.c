/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootdata_linux_get_focus(struct gui_activity*d){
	struct bootdata_linux*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_abootimg);
	lv_group_add_obj(gui_grp,bi->txt_kernel);
	lv_group_add_obj(gui_grp,bi->initrd.btn_edit);
	lv_group_add_obj(gui_grp,bi->initrd.btn_del);
	lv_group_add_obj(gui_grp,bi->initrd.sel);
	lv_group_add_obj(gui_grp,bi->dtbo.btn_edit);
	lv_group_add_obj(gui_grp,bi->dtbo.btn_del);
	lv_group_add_obj(gui_grp,bi->dtbo.sel);
	lv_group_add_obj(gui_grp,bi->txt_dtb);
	lv_group_add_obj(gui_grp,bi->txt_cmdline);
	lv_group_add_obj(gui_grp,bi->btn_splash);
	lv_group_add_obj(gui_grp,bi->btn_memory);
	lv_group_add_obj(gui_grp,bi->chk_use_efi);
	lv_group_add_obj(gui_grp,bi->chk_skip_dtb);
	lv_group_add_obj(gui_grp,bi->chk_skip_dtbo);
	lv_group_add_obj(gui_grp,bi->chk_skip_initrd);
	lv_group_add_obj(gui_grp,bi->chk_skip_efi_memory_map);
	lv_group_add_obj(gui_grp,bi->chk_skip_kernel_fdt_memory);
	lv_group_add_obj(gui_grp,bi->chk_skip_kernel_fdt_cmdline);
	lv_group_add_obj(gui_grp,bi->chk_match_kfdt_model);
	lv_group_add_obj(gui_grp,bi->chk_ignore_dtbo_error);
	lv_group_add_obj(gui_grp,bi->chk_update_splash);
	lv_group_add_obj(gui_grp,bi->chk_load_custom_address);
	lv_group_add_obj(gui_grp,bi->btn_address_load);
	lv_group_add_obj(gui_grp,bi->btn_address_kernel);
	lv_group_add_obj(gui_grp,bi->btn_address_initrd);
	lv_group_add_obj(gui_grp,bi->btn_address_dtb);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	(void)bootmgr_base;
	return 0;
}

static int bootdata_linux_lost_focus(struct gui_activity*d){
	struct bootdata_linux*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_abootimg);
	lv_group_remove_obj(bi->txt_kernel);
	lv_group_remove_obj(bi->initrd.btn_edit);
	lv_group_remove_obj(bi->initrd.btn_del);
	lv_group_remove_obj(bi->initrd.sel);
	lv_group_remove_obj(bi->dtbo.btn_edit);
	lv_group_remove_obj(bi->dtbo.btn_del);
	lv_group_remove_obj(bi->dtbo.sel);
	lv_group_remove_obj(bi->txt_dtb);
	lv_group_remove_obj(bi->txt_cmdline);
	lv_group_remove_obj(bi->btn_splash);
	lv_group_remove_obj(bi->btn_memory);
	lv_group_remove_obj(bi->chk_use_efi);
	lv_group_remove_obj(bi->chk_skip_dtb);
	lv_group_remove_obj(bi->chk_skip_dtbo);
	lv_group_remove_obj(bi->chk_skip_initrd);
	lv_group_remove_obj(bi->chk_skip_efi_memory_map);
	lv_group_remove_obj(bi->chk_skip_kernel_fdt_memory);
	lv_group_remove_obj(bi->chk_skip_kernel_fdt_cmdline);
	lv_group_remove_obj(bi->chk_match_kfdt_model);
	lv_group_remove_obj(bi->chk_ignore_dtbo_error);
	lv_group_remove_obj(bi->chk_update_splash);
	lv_group_remove_obj(bi->chk_load_custom_address);
	lv_group_remove_obj(bi->btn_address_load);
	lv_group_remove_obj(bi->btn_address_kernel);
	lv_group_remove_obj(bi->btn_address_initrd);
	lv_group_remove_obj(bi->btn_address_dtb);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_linux*bi){
	if(!bi->name[0])return false;
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_linux*bi=malloc(sizeof(struct bootdata_linux));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_linux));
	act->data=bi,bi->act=act;
	return 0;
}

static void add_dropdown(lv_obj_t*drop,list*lst){
	list*item;
	uint16_t pos=0;
	lv_dropdown_clear_options(drop);
	lv_dropdown_add_option(drop,_("(click button to add new)"),pos++);
	if((item=list_first(lst)))do{
		LIST_DATA_DECLARE(d,item,char*);
		if(d)lv_dropdown_add_option(drop,d,pos++);
	}while((item=item->next));
	lv_dropdown_set_selected(drop,pos-1);
}

static void init_dropdown(struct bootdata_linux*bi,lv_obj_t*drop,const char*base){
	char path[PATH_MAX],*buf,**bs;
	if(!drop||!base||!bi)return;
	list**lst=lv_obj_get_user_data(drop);
	if(!lst)return;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,"%s.%s.extra.%s",bootmgr_base,bi->name,base);
	list_free_all_def(*lst);
	*lst=NULL;
	switch(confd_get_type(path)){
		case TYPE_KEY:
			if(!(bs=confd_ls(path)))break;
			for(size_t i=0;(buf=bs[i]);i++)
				if(*buf)list_obj_add_new_strdup(lst,buf);
			if(bs[0])free(bs[0]);
			if(bs)free(bs);
		break;
		case TYPE_STRING:
			if((buf=confd_get_string(path,NULL)))
				list_obj_add_new(lst,buf);
		break;
		default:;
	}
	add_dropdown(drop,*lst);
}

static void init_memory_region(struct bootdata_linux*bi,lv_obj_t*obj,const char*name){
	char path[PATH_MAX];
	if(!obj||!name)return;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,"%s.%s.extra.%s",bootmgr_base,bi->name,name);
	int64_t base=confd_get_integer_base(path,"base",-1);
	int64_t start=confd_get_integer_base(path,"start",-1);
	int64_t size=confd_get_integer_base(path,"size",-1);
	int64_t end=confd_get_integer_base(path,"end",-1);
	if(start>0){
		if(base>0&&start!=base)tlog_warn("base and start mismatched");
		base=start;
	}
	if(end>0){
		if(size>0&&end!=base+size)tlog_warn("size and end mismatched");
		size=end-base;
	}
	if(base<0)base=0;
	if(size<0)size=0;
	lv_label_set_text_fmt(
		obj,"0x%llX - 0x%llX",
		(unsigned long long)base,
		(unsigned long long)base+size
	);
}

static int do_load(struct gui_activity*act){
	struct bootdata_linux*bi=act->data;
	if(!bi||!act->args)return 0;
	strncpy(bi->name,act->args,sizeof(bi->name)-1);
	char*abootimg=confd_get_string_dict(bootmgr_base,bi->name,"extra.abootimg",NULL);
	char*cmdline=confd_get_string_dict(bootmgr_base,bi->name,"extra.cmdline",NULL);
	char*kernel=confd_get_string_dict(bootmgr_base,bi->name,"extra.kernel",NULL);
	char*dtb=confd_get_string_dict(bootmgr_base,bi->name,"extra.dtb",NULL);
	bool use_uefi=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.use_uefi",true);
	bool skip_dtb=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_dtb",false);
	bool skip_dtbo=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_dtbo",false);
	bool skip_initrd=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_initrd",false);
	bool skip_mmap=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_efi_memory_map",false);
	bool skip_kfmem=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_kernel_fdt_memory",false);
	bool skip_kfcmd=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.skip_kernel_fdt_cmdline",false);
	bool upd_splash=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.update_splash",true);
	bool use_custom=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.load_custom_address",false);
	bool match_kfmodl=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.match_kernel_fdt_model",false);
	bool ignore_dtbo_err=confd_get_boolean_dict(bootmgr_base,bi->name,"extra.ignore_dtbo_error",false);
	if(abootimg)lv_textarea_set_text(bi->txt_abootimg,abootimg);
	if(cmdline)lv_textarea_set_text(bi->txt_cmdline,cmdline);
	if(kernel)lv_textarea_set_text(bi->txt_kernel,kernel);
	if(dtb)lv_textarea_set_text(bi->txt_dtb,dtb);
	lv_obj_set_checked(bi->chk_use_efi,use_uefi);
	lv_obj_set_checked(bi->chk_skip_dtb,skip_dtb);
	lv_obj_set_checked(bi->chk_skip_dtbo,skip_dtbo);
	lv_obj_set_checked(bi->chk_skip_initrd,skip_initrd);
	lv_obj_set_checked(bi->chk_skip_efi_memory_map,skip_mmap);
	lv_obj_set_checked(bi->chk_skip_kernel_fdt_memory,skip_kfmem);
	lv_obj_set_checked(bi->chk_skip_kernel_fdt_cmdline,skip_kfcmd);
	lv_obj_set_checked(bi->chk_update_splash,upd_splash);
	lv_obj_set_checked(bi->chk_load_custom_address,use_custom);
	lv_obj_set_checked(bi->chk_match_kfdt_model,match_kfmodl);
	lv_obj_set_checked(bi->chk_ignore_dtbo_error,ignore_dtbo_err);
	init_dropdown(bi,bi->initrd.sel,"initrd");
	init_dropdown(bi,bi->dtbo.sel,"dtbo");
	init_memory_region(bi,bi->txt_splash,"splash");
	init_memory_region(bi,bi->txt_memory,"memory");
	init_memory_region(bi,bi->txt_address_load,"address.load");
	init_memory_region(bi,bi->txt_address_kernel,"address.kernel");
	init_memory_region(bi,bi->txt_address_initrd,"address.initrd");
	init_memory_region(bi,bi->txt_address_dtb,"address.dtb");
	strncpy(bi->name,act->args,sizeof(bi->name)-1);
	if(abootimg)free(abootimg);
	if(cmdline)free(cmdline);
	if(kernel)free(kernel);
	if(dtb)free(dtb);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_linux*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_linux));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_event_t*e){
	if(do_save(e->user_data))guiact_do_back();
}

static void delete_cb(lv_event_t*e){
	list**lst,*item;
	char path[PATH_MAX];
	lv_obj_t*sel=e->user_data;
	if(!(lst=lv_obj_get_user_data(sel)))return;
	memset(path,0,sizeof(path));
	lv_dropdown_get_selected_str(sel,path,sizeof(path)-1);
	if((item=list_search_string(*lst,path)))
		list_obj_del(lst,item,list_default_free);
	add_dropdown(sel,*lst);
}

static bool edit_apply_cb(bool ok,const char*content,void*user_data){
	lv_obj_t*sel=user_data;
	list**lst,*item;
	char path[PATH_MAX];
	if(!ok)return false;
	if(!sel||!(lst=lv_obj_get_user_data(sel)))return true;
	memset(path,0,sizeof(path));
	if(lv_dropdown_get_selected(sel)==0){
		lv_dropdown_add_option(
			sel,content,
			lv_dropdown_get_option_cnt(sel)
		);
		list_obj_add_new_strdup(lst,content);
	}else{
		lv_dropdown_get_selected_str(sel,path,sizeof(path)-1);
		if((item=list_search_string(*lst,path))){
			free(item->data);
			item->data=strdup(content);
		}
		add_dropdown(sel,*lst);
	}
	return false;
}

static void edit_cb(lv_event_t*e){
	lv_obj_t*sel=e->user_data;
	struct inputbox*ib=inputbox_create(edit_apply_cb," ");
	inputbox_set_user_data(ib,sel);
	if(lv_dropdown_get_selected(sel)!=0){
		char path[PATH_MAX];
		memset(path,0,sizeof(path));
		lv_dropdown_get_selected_str(sel,path,sizeof(path)-1);
		inputbox_set_content(ib,"%s",path);
		inputbox_set_title(ib,_("Edit item"));
	}else inputbox_set_title(ib,_("Create new item"));
}

static void region_cb(lv_event_t*e){
	static char path[PATH_MAX],*base;
	struct bootdata_linux*bi=e->user_data;
	if(e->target==bi->btn_address_load)base="address.load";
	else if(e->target==bi->btn_address_kernel)base="address.kernel";
	else if(e->target==bi->btn_address_initrd)base="address.initrd";
	else if(e->target==bi->btn_address_dtb)base="address.dtb";
	else if(e->target==bi->btn_splash)base="splash";
	else if(e->target==bi->btn_memory)base="memory";
	else return;
	memset(path,0,sizeof(path));
	snprintf(
		path,sizeof(path)-1,
		"%s.%s.extra.%s",
		bootmgr_base,bi->name,base
	);
	guiact_start_activity(&guireg_bootdata_memreg,path);
}

static lv_obj_t*draw_multi_item(lv_obj_t*parent,char*title,struct bootdata_multi_item*item){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	if(!item)return NULL;
	item->box=lv_draw_line_wrapper(parent,grid_col,grid_row);

	item->lbl=lv_label_create(item->box);
	lv_label_set_text(item->lbl,_(title));
	lv_obj_set_grid_cell(item->lbl,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,0,1);

	item->btn_edit=lv_btn_create(item->box);
	lv_obj_set_style_radius(item->btn_edit,LV_RADIUS_CIRCLE,0);
	lv_obj_add_event_cb(item->btn_edit,edit_cb,LV_EVENT_CLICKED,item->sel);
	lv_obj_set_grid_cell(item->btn_edit,LV_GRID_ALIGN_CENTER,1,1,LV_GRID_ALIGN_CENTER,0,1);

	item->lbl_edit=lv_label_create(item->btn_edit);
	lv_label_set_text(item->lbl_edit,LV_SYMBOL_EDIT);
	lv_obj_center(item->lbl_edit);

	item->btn_del=lv_btn_create(item->box);
	lv_obj_set_style_radius(item->btn_del,LV_RADIUS_CIRCLE,0);
	lv_obj_add_event_cb(item->btn_del,delete_cb,LV_EVENT_CLICKED,item->sel);
	lv_obj_set_grid_cell(item->btn_del,LV_GRID_ALIGN_CENTER,2,1,LV_GRID_ALIGN_CENTER,0,1);

	item->lbl_del=lv_label_create(item->btn_del);
	lv_label_set_text(item->lbl_del,LV_SYMBOL_CLOSE);
	lv_obj_center(item->lbl_del);

	item->sel=lv_dropdown_create(item->box);
	lv_obj_set_user_data(item->sel,item->lst);
	lv_obj_add_event_cb(item->sel,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_grid_cell(item->sel,LV_GRID_ALIGN_STRETCH,0,3,LV_GRID_ALIGN_STRETCH,1,1);

	return item->box;
}

static lv_obj_t*draw_memreg(
	struct bootdata_linux*bi,
	char*title,
	lv_obj_t**btn,
	lv_obj_t**txt
){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	};
	if(!btn||!txt)return NULL;
	lv_obj_t*box=lv_draw_line_wrapper(bi->box,grid_col,grid_row);

	lv_obj_t*lbl=lv_label_create(box);
	lv_label_set_text(lbl,_(title));
	lv_obj_set_grid_cell(lbl,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,0,1);

	*txt=lv_label_create(box);
	lv_obj_set_grid_cell(*txt,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);

	*btn=lv_btn_create(box);
	lv_obj_add_event_cb(*btn,region_cb,LV_EVENT_CLICKED,bi);
	lv_obj_set_grid_cell(*btn,LV_GRID_ALIGN_CENTER,1,1,LV_GRID_ALIGN_CENTER,0,2);
	lv_obj_t*btn_edit=lv_label_create(*btn);
	lv_label_set_text(btn_edit,LV_SYMBOL_EDIT);
	lv_obj_center(btn_edit);

	return box;
}

static int draw_bootdata_linux(struct gui_activity*act){
	struct bootdata_linux*bi=act->data;
	bi->box=lv_draw_dialog_box(act->page,&bi->title,"Edit Linux Boot Item");
	lv_draw_input(bi->box,"Android Boot Image", NULL, &bi->clr_abootimg, &bi->txt_abootimg, NULL);
	lv_draw_input(bi->box,"Linux Kernel",       NULL, &bi->clr_kernel,   &bi->txt_kernel,   NULL);
	draw_multi_item(bi->box,"Ramdisk (initramfs)",        &bi->initrd);
	draw_multi_item(bi->box,"Device Tree Overlay (dtbo)", &bi->dtbo);
	lv_draw_input(bi->box,"Device Tree Blob (dtb)", NULL, &bi->clr_dtb,     &bi->txt_dtb,     NULL);
	lv_draw_input(bi->box,"Kernel Commandline",     NULL, &bi->clr_cmdline, &bi->txt_cmdline, NULL);
	lv_textarea_set_one_line(bi->txt_cmdline,false);
	draw_memreg(bi,"Screen Framebuffer", &bi->btn_splash,   &bi->txt_splash);
	draw_memreg(bi,"System Memory",      &bi->btn_memory,   &bi->txt_memory);
	bi->chk_use_efi                 =lv_draw_checkbox(bi->box,_("Boot with UEFI runtime"),          false,NULL,NULL);
	bi->chk_skip_dtb                =lv_draw_checkbox(bi->box,_("Skip Device Tree Blob (dtb)"),     false,NULL,NULL);
	bi->chk_skip_dtbo               =lv_draw_checkbox(bi->box,_("Skip Device Tree Overlay (dtbo)"), false,NULL,NULL);
	bi->chk_skip_initrd             =lv_draw_checkbox(bi->box,_("Skip Initramfs"),                  false,NULL,NULL);
	bi->chk_skip_efi_memory_map     =lv_draw_checkbox(bi->box,_("Skip UEFI Memory Map"),            false,NULL,NULL);
	bi->chk_skip_kernel_fdt_memory  =lv_draw_checkbox(bi->box,_("Skip KernelFdtDxe System Memory"), false,NULL,NULL);
	bi->chk_skip_kernel_fdt_cmdline =lv_draw_checkbox(bi->box,_("Skip KernelFdtDxe Commandline"),   false,NULL,NULL);
	bi->chk_match_kfdt_model        =lv_draw_checkbox(bi->box,_("Match KernelFdtDxe Model"),        false,NULL,NULL);
	bi->chk_ignore_dtbo_error       =lv_draw_checkbox(bi->box,_("Ignore DTBO Error"),               false,NULL,NULL);
	bi->chk_update_splash           =lv_draw_checkbox(bi->box,_("Update Screen Framebuffer"),       false,NULL,NULL);
	bi->chk_load_custom_address     =lv_draw_checkbox(bi->box,_("Use Custom Load Addresses"),       false,NULL,NULL);
	draw_memreg(bi,"Default Load Address",     &bi->btn_address_load,   &bi->txt_address_load);
	draw_memreg(bi,"Kernel Load Address",      &bi->btn_address_kernel, &bi->txt_address_kernel);
	draw_memreg(bi,"Initramfs Load Address",   &bi->btn_address_initrd, &bi->txt_address_initrd);
	draw_memreg(bi,"Device Tree Load Address", &bi->btn_address_dtb,    &bi->txt_address_dtb);
	lv_draw_btns_ok_cancel(bi->box,&bi->ok,&bi->cancel,ok_cb,bi);
	return 0;
}

struct gui_register guireg_bootdata_linux={
	.name="bootdata-linux",
	.title="Boot Item Extra Data Editor",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_linux_get_focus,
	.lost_focus=bootdata_linux_lost_focus,
	.draw=draw_bootdata_linux,
	.data_load=do_load,
	.back=true,
	.mask=true,
};
#endif

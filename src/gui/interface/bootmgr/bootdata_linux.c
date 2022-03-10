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
	lv_group_add_obj(gui_grp,bi->btn_initrd_edit);
	lv_group_add_obj(gui_grp,bi->btn_initrd_del);
	lv_group_add_obj(gui_grp,bi->sel_initrd);
	lv_group_add_obj(gui_grp,bi->btn_dtbo_edit);
	lv_group_add_obj(gui_grp,bi->btn_dtbo_del);
	lv_group_add_obj(gui_grp,bi->sel_dtbo);
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
	lv_group_remove_obj(bi->btn_initrd_edit);
	lv_group_remove_obj(bi->btn_initrd_del);
	lv_group_remove_obj(bi->sel_initrd);
	lv_group_remove_obj(bi->btn_dtbo_edit);
	lv_group_remove_obj(bi->btn_dtbo_del);
	lv_group_remove_obj(bi->sel_dtbo);
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
	if(abootimg)lv_textarea_set_text(bi->txt_abootimg,abootimg);
	if(cmdline)lv_textarea_set_text(bi->txt_cmdline,cmdline);
	if(kernel)lv_textarea_set_text(bi->txt_kernel,kernel);
	if(dtb)lv_textarea_set_text(bi->txt_dtb,dtb);
	lv_checkbox_set_checked(bi->chk_use_efi,use_uefi);
	lv_checkbox_set_checked(bi->chk_skip_dtb,skip_dtb);
	lv_checkbox_set_checked(bi->chk_skip_dtbo,skip_dtbo);
	lv_checkbox_set_checked(bi->chk_skip_initrd,skip_initrd);
	lv_checkbox_set_checked(bi->chk_skip_efi_memory_map,skip_mmap);
	lv_checkbox_set_checked(bi->chk_skip_kernel_fdt_memory,skip_kfmem);
	lv_checkbox_set_checked(bi->chk_skip_kernel_fdt_cmdline,skip_kfcmd);
	lv_checkbox_set_checked(bi->chk_update_splash,upd_splash);
	lv_checkbox_set_checked(bi->chk_load_custom_address,use_custom);
	init_dropdown(bi,bi->sel_initrd,"initrd");
	init_dropdown(bi,bi->sel_dtbo,"dtbo");
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

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_linux*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->ok||e!=LV_EVENT_CLICKED)return;
	if(do_save(bi))guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	struct bootdata_linux*bi=lv_obj_get_user_data(obj);
	if(!bi||obj!=bi->cancel||e!=LV_EVENT_CLICKED)return;
	guiact_do_back();
}

static void delete_cb(lv_obj_t*obj,lv_event_t e){
	lv_obj_t*sel;
	list**lst,*item;
	char path[PATH_MAX];
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(!(sel=lv_obj_get_user_data(obj)))return;
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

static void edit_cb(lv_obj_t*obj,lv_event_t e){
	lv_obj_t*sel;
	if(!obj||e!=LV_EVENT_CLICKED)return;
	if(!(sel=lv_obj_get_user_data(obj)))return;
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

static void region_cb(lv_obj_t*obj,lv_event_t e){
	static char path[PATH_MAX],*base;
	struct bootdata_linux*bi=lv_obj_get_user_data(obj);
	if(!bi||e!=LV_EVENT_CLICKED)return;
	if(obj==bi->btn_address_load)base="address.load";
	else if(obj==bi->btn_address_kernel)base="address.kernel";
	else if(obj==bi->btn_address_initrd)base="address.initrd";
	else if(obj==bi->btn_address_dtb)base="address.dtb";
	else if(obj==bi->btn_splash)base="splash";
	else if(obj==bi->btn_memory)base="memory";
	else return;
	memset(path,0,sizeof(path));
	snprintf(
		path,sizeof(path)-1,
		"%s.%s.extra.%s",
		bootmgr_base,bi->name,base
	);
	guiact_start_activity(&guireg_bootdata_memreg,path);
}

static int do_resize(struct gui_activity*act){
	lv_coord_t h=0,w=act->w/8*7,x=0,s;
	struct bootdata_linux*bi=act->data;
	if(!bi)return 0;
	lv_obj_set_style_local_pad_all(
		bi->box,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		gui_font_size
	);
	lv_obj_set_width(bi->page,w);
	w=lv_page_get_scrl_width(bi->page);
	lv_obj_set_width(bi->box,w);

	lv_obj_set_width(bi->title,w);
	lv_obj_set_pos(bi->title,x,h);
	lv_label_set_align(
		bi->title,
		LV_LABEL_ALIGN_CENTER
	);
	h+=lv_obj_get_height(bi->title);

	// abootimg title
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_abootimg,x,h);
	h+=lv_obj_get_height(bi->lbl_abootimg);

	// abootimg path text
	lv_obj_set_pos(bi->txt_abootimg,x,h);
	lv_obj_set_width(bi->txt_abootimg,w);
	h+=lv_obj_get_height(bi->txt_abootimg);

	// kernel title
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_kernel,x,h);
	h+=lv_obj_get_height(bi->lbl_kernel);

	// kernel path text
	lv_obj_set_pos(bi->txt_kernel,x,h);
	lv_obj_set_width(bi->txt_kernel,w);
	h+=lv_obj_get_height(bi->txt_kernel);

	// initramfs title and add del button
	h+=gui_font_size/2;
	s=lv_obj_get_height(bi->sel_initrd);
	lv_obj_set_size(bi->btn_initrd_edit,s,s);
	lv_obj_set_size(bi->btn_initrd_del,s,s);
	lv_obj_align(
		bi->btn_initrd_del,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->btn_initrd_edit,
		bi->btn_initrd_del,
		LV_ALIGN_OUT_LEFT_MID,
		-(gui_font_size/2),0
	);
	lv_obj_align(
		bi->lbl_initrd,
		bi->btn_initrd_del,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_initrd,x);
	h+=lv_obj_get_height(bi->btn_initrd_del)+(gui_font_size/2);

	// initramfs path dropdown
	lv_obj_set_pos(bi->sel_initrd,x,h);
	lv_obj_set_width(bi->sel_initrd,w);
	h+=lv_obj_get_height(bi->sel_initrd);

	// dtbo title and add del button
	h+=gui_font_size/2;
	s=lv_obj_get_height(bi->sel_dtbo);
	lv_obj_set_size(bi->btn_dtbo_edit,s,s);
	lv_obj_set_size(bi->btn_dtbo_del,s,s);
	lv_obj_align(
		bi->btn_dtbo_del,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->btn_dtbo_edit,
		bi->btn_dtbo_del,
		LV_ALIGN_OUT_LEFT_MID,
		-(gui_font_size/2),0
	);
	lv_obj_align(
		bi->lbl_dtbo,
		bi->btn_dtbo_del,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_dtbo,x);
	h+=lv_obj_get_height(bi->btn_dtbo_del)+(gui_font_size/2);

	// dtbo path dropdown
	lv_obj_set_pos(bi->sel_dtbo,x,h);
	lv_obj_set_width(bi->sel_dtbo,w);
	h+=lv_obj_get_height(bi->sel_dtbo);

	// dtb title
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_dtb,x,h);
	h+=lv_obj_get_height(bi->lbl_dtb);

	// dtb path text
	lv_obj_set_pos(bi->txt_dtb,x,h);
	lv_obj_set_width(bi->txt_dtb,w);
	h+=lv_obj_get_height(bi->txt_dtb);

	// cmdline title
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->lbl_cmdline,x,h);
	h+=lv_obj_get_height(bi->lbl_cmdline);

	// cmdline path text
	lv_obj_set_pos(bi->txt_cmdline,x,h);
	lv_obj_set_width(bi->txt_cmdline,w);
	h+=lv_obj_get_height(bi->txt_cmdline);

	// splash title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_splash,s,s);
	lv_obj_align(
		bi->btn_splash,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_splash,
		bi->btn_splash,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_splash,x);
	h+=lv_obj_get_height(bi->btn_splash);
	lv_obj_set_pos(bi->txt_splash,x,h);
	lv_obj_set_width(bi->txt_splash,w);
	h+=lv_obj_get_height(bi->txt_splash)+(gui_font_size/2);

	// system memory title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_memory,s,s);
	lv_obj_align(
		bi->btn_memory,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_memory,
		bi->btn_memory,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_memory,x);
	h+=lv_obj_get_height(bi->btn_memory);
	lv_obj_set_pos(bi->txt_memory,x,h);
	lv_obj_set_width(bi->txt_memory,w);
	h+=lv_obj_get_height(bi->txt_memory)+(gui_font_size/2);

	// use uefi checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_use_efi,x,h);
	h+=lv_obj_get_height(bi->chk_use_efi);

	// skip dtb checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_dtb,x,h);
	h+=lv_obj_get_height(bi->chk_skip_dtb);

	// skip dtbo checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_dtbo,x,h);
	h+=lv_obj_get_height(bi->chk_skip_dtbo);

	// skip initramfs checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_initrd,x,h);
	h+=lv_obj_get_height(bi->chk_skip_initrd);

	// skip uefi memory map checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_efi_memory_map,x,h);
	h+=lv_obj_get_height(bi->chk_skip_efi_memory_map);

	// skip KernelFdtDxe system memory region checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_kernel_fdt_memory,x,h);
	h+=lv_obj_get_height(bi->chk_skip_kernel_fdt_memory);

	// skip KernelFdtDxe kernel command line checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_skip_kernel_fdt_cmdline,x,h);
	h+=lv_obj_get_height(bi->chk_skip_kernel_fdt_cmdline);

	// update splash checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_update_splash,x,h);
	h+=lv_obj_get_height(bi->chk_update_splash);

	// use custom load addresses checkbox
	h+=gui_font_size/2;
	lv_obj_set_pos(bi->chk_load_custom_address,x,h);
	h+=lv_obj_get_height(bi->chk_load_custom_address);

	// load address title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_address_load,s,s);
	lv_obj_align(
		bi->btn_address_load,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_address_load,
		bi->btn_address_load,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_address_load,x);
	h+=lv_obj_get_height(bi->btn_address_load);
	lv_obj_set_pos(bi->txt_address_load,x,h);
	lv_obj_set_width(bi->txt_address_load,w);
	h+=lv_obj_get_height(bi->txt_address_load)+(gui_font_size/2);

	// kernel load address title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_address_kernel,s,s);
	lv_obj_align(
		bi->btn_address_kernel,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_address_kernel,
		bi->btn_address_kernel,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_address_kernel,x);
	h+=lv_obj_get_height(bi->btn_address_kernel);
	lv_obj_set_pos(bi->txt_address_kernel,x,h);
	lv_obj_set_width(bi->txt_address_kernel,w);
	h+=lv_obj_get_height(bi->txt_address_kernel)+(gui_font_size/2);

	// initramfs load address title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_address_initrd,s,s);
	lv_obj_align(
		bi->btn_address_initrd,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_address_initrd,
		bi->btn_address_initrd,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_address_initrd,x);
	h+=lv_obj_get_height(bi->btn_address_initrd);
	lv_obj_set_pos(bi->txt_address_initrd,x,h);
	lv_obj_set_width(bi->txt_address_initrd,w);
	h+=lv_obj_get_height(bi->txt_address_initrd)+(gui_font_size/2);

	// device tree load address title and edit button
	h+=gui_font_size/2;
	lv_obj_set_size(bi->btn_address_dtb,s,s);
	lv_obj_align(
		bi->btn_address_dtb,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	lv_obj_align(
		bi->lbl_address_dtb,
		bi->btn_address_dtb,
		LV_ALIGN_OUT_LEFT_MID,
		0,0
	);
	lv_obj_set_x(bi->lbl_address_dtb,x);
	h+=lv_obj_get_height(bi->btn_address_dtb);
	lv_obj_set_pos(bi->txt_address_dtb,x,h);
	lv_obj_set_width(bi->txt_address_dtb,w);
	h+=lv_obj_get_height(bi->txt_address_dtb)+(gui_font_size/2);

	// ok button
	h+=gui_font_size;
	lv_obj_set_size(
		bi->ok,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->ok,NULL,
		LV_ALIGN_IN_TOP_LEFT,
		(x+(gui_font_size/2)),h
	);

	// cancel button
	lv_obj_set_size(
		bi->cancel,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		bi->cancel,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(x+(gui_font_size/2)),h
	);
	h+=lv_obj_get_height(bi->cancel);

	h+=gui_font_size/2;
	lv_obj_set_height(bi->box,h);
	h+=gui_font_size*2;
	lv_obj_set_height(bi->page,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(bi->page,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int draw_bootdata_linux(struct gui_activity*act){
	struct bootdata_linux*bi=act->data;

	bi->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(bi->page,false);

	bi->box=lv_obj_create(bi->page,NULL);
	lv_obj_set_click(bi->box,false);
	lv_obj_set_style_local_border_width(bi->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	bi->title=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->title,_("Edit Linux Boot Item"));
	lv_label_set_long_mode(bi->title,LV_LABEL_LONG_BREAK);

	bi->lbl_abootimg=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_abootimg,_("Android Boot Image"));

	bi->txt_abootimg=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_abootimg,"");
	lv_textarea_set_one_line(bi->txt_abootimg,true);
	lv_textarea_set_cursor_hidden(bi->txt_abootimg,true);
	lv_obj_set_user_data(bi->txt_abootimg,bi);
	lv_obj_set_event_cb(bi->txt_abootimg,lv_input_cb);

	bi->lbl_kernel=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_kernel,_("Linux Kernel"));

	bi->txt_kernel=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_kernel,"");
	lv_textarea_set_one_line(bi->txt_kernel,true);
	lv_textarea_set_cursor_hidden(bi->txt_kernel,true);
	lv_obj_set_user_data(bi->txt_kernel,bi);
	lv_obj_set_event_cb(bi->txt_kernel,lv_input_cb);

	bi->lbl_initrd=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_initrd,_("Ramdisk (initramfs)"));

	bi->sel_initrd=lv_dropdown_create(bi->box,NULL);
	lv_obj_set_user_data(bi->sel_initrd,&bi->initrd);
	lv_obj_set_event_cb(bi->sel_initrd,lv_default_dropdown_cb);

	bi->btn_initrd_edit=lv_btn_create(bi->box,NULL);
	lv_obj_set_event_cb(bi->btn_initrd_edit,edit_cb);
	lv_obj_set_user_data(bi->btn_initrd_edit,bi->sel_initrd);
	lv_style_set_action_button(bi->btn_initrd_edit,true);
	lv_obj_set_style_local_radius(bi->btn_initrd_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_initrd_edit,NULL),LV_SYMBOL_EDIT);

	bi->btn_initrd_del=lv_btn_create(bi->box,NULL);
	lv_obj_set_event_cb(bi->btn_initrd_del,delete_cb);
	lv_obj_set_user_data(bi->btn_initrd_del,bi->sel_initrd);
	lv_style_set_action_button(bi->btn_initrd_del,true);
	lv_obj_set_style_local_radius(bi->btn_initrd_del,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_initrd_del,NULL),LV_SYMBOL_CLOSE);

	bi->lbl_dtbo=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_dtbo,_("Device Tree Overlay (dtbo)"));

	bi->sel_dtbo=lv_dropdown_create(bi->box,NULL);
	lv_obj_set_user_data(bi->sel_dtbo,&bi->dtbo);
	lv_obj_set_event_cb(bi->sel_dtbo,lv_default_dropdown_cb);

	bi->btn_dtbo_edit=lv_btn_create(bi->box,NULL);
	lv_obj_set_event_cb(bi->btn_dtbo_edit,edit_cb);
	lv_obj_set_user_data(bi->btn_dtbo_edit,bi->sel_dtbo);
	lv_style_set_action_button(bi->btn_dtbo_edit,true);
	lv_obj_set_style_local_radius(bi->btn_dtbo_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_dtbo_edit,NULL),LV_SYMBOL_EDIT);

	bi->btn_dtbo_del=lv_btn_create(bi->box,NULL);
	lv_obj_set_event_cb(bi->btn_dtbo_del,delete_cb);
	lv_obj_set_user_data(bi->btn_dtbo_del,bi->sel_dtbo);
	lv_style_set_action_button(bi->btn_dtbo_del,true);
	lv_obj_set_style_local_radius(bi->btn_dtbo_del,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_dtbo_del,NULL),LV_SYMBOL_CLOSE);

	bi->lbl_dtb=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_dtb,_("Device Tree Blob (dtb)"));

	bi->txt_dtb=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_dtb,"");
	lv_textarea_set_one_line(bi->txt_dtb,true);
	lv_textarea_set_cursor_hidden(bi->txt_dtb,true);
	lv_obj_set_user_data(bi->txt_dtb,bi);
	lv_obj_set_event_cb(bi->txt_dtb,lv_input_cb);

	bi->lbl_cmdline=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_cmdline,_("Kernel Commandline"));

	bi->txt_cmdline=lv_textarea_create(bi->box,NULL);
	lv_textarea_set_text(bi->txt_cmdline,"");
	lv_textarea_set_cursor_hidden(bi->txt_cmdline,true);
	lv_obj_set_user_data(bi->txt_cmdline,bi);
	lv_obj_set_event_cb(bi->txt_cmdline,lv_input_cb);

	bi->lbl_splash=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_splash,_("Screen Framebuffer"));

	bi->btn_splash=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_splash,bi);
	lv_obj_set_event_cb(bi->btn_splash,region_cb);
	lv_style_set_action_button(bi->btn_splash,true);
	lv_obj_set_style_local_radius(bi->btn_splash,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_splash,NULL),LV_SYMBOL_EDIT);

	bi->txt_splash=lv_label_create(bi->box,NULL);

	bi->lbl_memory=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_memory,_("System Memory"));

	bi->btn_memory=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_memory,bi);
	lv_obj_set_event_cb(bi->btn_memory,region_cb);
	lv_style_set_action_button(bi->btn_memory,true);
	lv_obj_set_style_local_radius(bi->btn_memory,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_memory,NULL),LV_SYMBOL_EDIT);

	bi->txt_memory=lv_label_create(bi->box,NULL);

	bi->chk_use_efi=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_use_efi,bi);
	lv_obj_set_drag_parent(bi->chk_use_efi,true);
	lv_checkbox_set_text(bi->chk_use_efi,_("Boot with UEFI runtime"));
	lv_checkbox_set_checked(bi->chk_use_efi,true);
	lv_style_set_focus_checkbox(bi->chk_use_efi);

	bi->chk_skip_dtb=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_dtb,bi);
	lv_obj_set_drag_parent(bi->chk_skip_dtb,true);
	lv_checkbox_set_text(bi->chk_skip_dtb,_("Skip Device Tree Blob (dtb)"));
	lv_checkbox_set_checked(bi->chk_skip_dtb,true);
	lv_style_set_focus_checkbox(bi->chk_skip_dtb);

	bi->chk_skip_dtbo=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_dtbo,bi);
	lv_obj_set_drag_parent(bi->chk_skip_dtbo,true);
	lv_checkbox_set_text(bi->chk_skip_dtbo,_("Skip Device Tree Overlay (dtbo)"));
	lv_checkbox_set_checked(bi->chk_skip_dtbo,true);
	lv_style_set_focus_checkbox(bi->chk_skip_dtbo);

	bi->chk_skip_initrd=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_initrd,bi);
	lv_obj_set_drag_parent(bi->chk_skip_initrd,true);
	lv_checkbox_set_text(bi->chk_skip_initrd,_("Skip Initramfs"));
	lv_checkbox_set_checked(bi->chk_skip_initrd,true);
	lv_style_set_focus_checkbox(bi->chk_skip_initrd);

	bi->chk_skip_efi_memory_map=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_efi_memory_map,bi);
	lv_obj_set_drag_parent(bi->chk_skip_efi_memory_map,true);
	lv_checkbox_set_text(bi->chk_skip_efi_memory_map,_("Skip UEFI Memory Map"));
	lv_checkbox_set_checked(bi->chk_skip_efi_memory_map,true);
	lv_style_set_focus_checkbox(bi->chk_skip_efi_memory_map);

	bi->chk_skip_kernel_fdt_memory=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_kernel_fdt_memory,bi);
	lv_obj_set_drag_parent(bi->chk_skip_kernel_fdt_memory,true);
	lv_checkbox_set_text(bi->chk_skip_kernel_fdt_memory,_("Skip KernelFdtDxe System Memory"));
	lv_checkbox_set_checked(bi->chk_skip_kernel_fdt_memory,true);
	lv_style_set_focus_checkbox(bi->chk_skip_kernel_fdt_memory);

	bi->chk_skip_kernel_fdt_cmdline=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_skip_kernel_fdt_cmdline,bi);
	lv_obj_set_drag_parent(bi->chk_skip_kernel_fdt_cmdline,true);
	lv_checkbox_set_text(bi->chk_skip_kernel_fdt_cmdline,_("Skip KernelFdtDxe Commandline"));
	lv_checkbox_set_checked(bi->chk_skip_kernel_fdt_cmdline,true);
	lv_style_set_focus_checkbox(bi->chk_skip_kernel_fdt_cmdline);

	bi->chk_update_splash=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_update_splash,bi);
	lv_obj_set_drag_parent(bi->chk_update_splash,true);
	lv_checkbox_set_text(bi->chk_update_splash,_("Update Screen Framebuffer"));
	lv_checkbox_set_checked(bi->chk_update_splash,true);
	lv_style_set_focus_checkbox(bi->chk_update_splash);

	bi->chk_load_custom_address=lv_checkbox_create(bi->box,NULL);
	lv_obj_set_user_data(bi->chk_load_custom_address,bi);
	lv_obj_set_drag_parent(bi->chk_load_custom_address,true);
	lv_checkbox_set_text(bi->chk_load_custom_address,_("Use Custom Load Addresses"));
	lv_checkbox_set_checked(bi->chk_load_custom_address,true);
	lv_style_set_focus_checkbox(bi->chk_load_custom_address);

	bi->lbl_address_load=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_address_load,_("Default Load Address"));

	bi->btn_address_load=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_address_load,bi);
	lv_obj_set_event_cb(bi->btn_address_load,region_cb);
	lv_style_set_action_button(bi->btn_address_load,true);
	lv_obj_set_style_local_radius(bi->btn_address_load,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_address_load,NULL),LV_SYMBOL_EDIT);

	bi->txt_address_load=lv_label_create(bi->box,NULL);

	bi->lbl_address_kernel=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_address_kernel,_("Kernel Load Address"));

	bi->btn_address_kernel=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_address_kernel,bi);
	lv_obj_set_event_cb(bi->btn_address_kernel,region_cb);
	lv_style_set_action_button(bi->btn_address_kernel,true);
	lv_obj_set_style_local_radius(bi->btn_address_kernel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_address_kernel,NULL),LV_SYMBOL_EDIT);

	bi->txt_address_kernel=lv_label_create(bi->box,NULL);

	bi->lbl_address_initrd=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_address_initrd,_("Initramfs Load Address"));

	bi->btn_address_initrd=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_address_initrd,bi);
	lv_obj_set_event_cb(bi->btn_address_initrd,region_cb);
	lv_style_set_action_button(bi->btn_address_initrd,true);
	lv_obj_set_style_local_radius(bi->btn_address_initrd,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_address_initrd,NULL),LV_SYMBOL_EDIT);

	bi->txt_address_initrd=lv_label_create(bi->box,NULL);

	bi->lbl_address_dtb=lv_label_create(bi->box,NULL);
	lv_label_set_text(bi->lbl_address_dtb,_("Device Tree Load Address"));

	bi->btn_address_dtb=lv_btn_create(bi->box,NULL);
	lv_obj_set_user_data(bi->btn_address_dtb,bi);
	lv_obj_set_event_cb(bi->btn_address_dtb,region_cb);
	lv_style_set_action_button(bi->btn_address_dtb,true);
	lv_obj_set_style_local_radius(bi->btn_address_dtb,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_RADIUS_CIRCLE);
	lv_label_set_text(lv_label_create(bi->btn_address_dtb,NULL),LV_SYMBOL_EDIT);

	bi->txt_address_dtb=lv_label_create(bi->box,NULL);

	// OK Button
	bi->ok=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->ok,true);
	lv_obj_set_user_data(bi->ok,bi);
	lv_obj_set_event_cb(bi->ok,ok_cb);
	lv_label_set_text(lv_label_create(bi->ok,NULL),_("OK"));

	// Cancel Button
	bi->cancel=lv_btn_create(bi->box,NULL);
	lv_style_set_action_button(bi->cancel,true);
	lv_obj_set_user_data(bi->cancel,bi);
	lv_obj_set_event_cb(bi->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(bi->cancel,NULL),_("Cancel"));
	return 0;
}

struct gui_register guireg_bootdata_linux={
	.name="bootdata-linux",
	.title="Boot Item Extra Data Editor",
	.icon="bootmgr.svg",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_linux_get_focus,
	.lost_focus=bootdata_linux_lost_focus,
	.draw=draw_bootdata_linux,
	.data_load=do_load,
	.resize=do_resize,
	.back=true,
	.mask=true,
};
#endif

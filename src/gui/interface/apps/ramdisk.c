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
#include<ctype.h>
#include<stddef.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Protocol/DevicePath.h>
#include<Protocol/RamDisk.h>
#include"str.h"
#include"gui.h"
#include"uefi.h"
#include"logger.h"
#include"system.h"
#include"compatible.h"
#include"gui/fsext.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "ramdisk"

struct add_ramdisk{
	bool size_changed;
	lv_obj_t*page,*box,*ok,*cancel;
	lv_obj_t*btn_source,*btn_size;
	lv_obj_t*lbl_title,*lbl_source;
	lv_obj_t*lbl_size,*lbl_mem_type,*lbl_type;
	lv_obj_t*source,*size,*mem_type,*type;
};

static int add_ramdisk_get_focus(struct gui_activity*d){
	struct add_ramdisk*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->source);
	lv_group_add_obj(gui_grp,am->btn_source);
	lv_group_add_obj(gui_grp,am->size);
	lv_group_add_obj(gui_grp,am->btn_size);
	lv_group_add_obj(gui_grp,am->mem_type);
	lv_group_add_obj(gui_grp,am->type);
	lv_group_add_obj(gui_grp,am->ok);
	lv_group_add_obj(gui_grp,am->cancel);
	return 0;
}

static int add_ramdisk_lost_focus(struct gui_activity*d){
	struct add_ramdisk*am=d->data;
	if(!am)return 0;
	lv_group_remove_obj(am->source);
	lv_group_remove_obj(am->btn_source);
	lv_group_remove_obj(am->size);
	lv_group_remove_obj(am->btn_size);
	lv_group_remove_obj(am->mem_type);
	lv_group_remove_obj(am->type);
	lv_group_remove_obj(am->ok);
	lv_group_remove_obj(am->cancel);
	return 0;
}

static void load_size(struct add_ramdisk*am){
	char size[64];
	lv_fs_file_t file;
	const char*source;
	EFI_FILE_PROTOCOL*fp;
	EFI_FILE_INFO*info=NULL;
	if(am->size_changed)return;
	source=lv_textarea_get_text(am->source);
	if(!source||!*source)return;
	if(lv_fs_open(&file,source,LV_FS_MODE_RD)!=LV_FS_RES_OK)return;
	ZeroMem(size,sizeof(size));
	if(
		(fp=lv_fs_file_to_fp(&file))&&
		!EFI_ERROR(efi_file_get_file_info(fp,NULL,&info))&&info
	){
		AsciiSPrint(
			size,sizeof(size)-1,
			"0x%llx",info->FileSize
		);
		FreePool(info);
		lv_textarea_set_text(am->size,size);
	}
	lv_fs_close(&file);
}

static bool select_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	lv_textarea_set_text(user_data,path[0]);
	load_size(lv_obj_get_user_data(user_data));
	return false;
}

static void sel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct filepicker*fp=filepicker_create(select_cb,"Select item");
	filepicker_set_user_data(fp,lv_obj_get_user_data(obj));
	filepicker_set_max_item(fp,1);
}

static void ok_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	EFI_GUID*dt;
	EFI_STATUS st;
	EFI_MEMORY_TYPE mt;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_RAM_DISK_PROTOCOL*ramdisk=NULL;
	struct add_ramdisk*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->ok)return;
	uint8_t type=lv_dropdown_get_selected(am->type);
	uint8_t mem_type=lv_dropdown_get_selected(am->mem_type);
	const char*source=lv_textarea_get_text(am->source);
	const char*size=lv_textarea_get_text(am->size);
	if(source&&!source[0])source=NULL;
	if(size&&!size[0])size=NULL;
	errno=0;
	char*end;
	long long mem=strtoll(size,&end,0);
	if(*end||size==end||errno!=0||mem<=0){
		msgbox_alert("Invalid memory size");
		return;
	}
	switch(mem_type){
		case 0:mt=EfiBootServicesData;break;
		case 1:mt=EfiReservedMemoryType;break;
		case 2:mt=EfiConventionalMemory;break;
		default:msgbox_alert("Invalid memory type");return;
	}
	switch(type){
		case 0:dt=&gEfiVirtualCdGuid;break;
		case 1:dt=&gEfiVirtualDiskGuid;break;
		case 2:dt=&gEfiPersistentVirtualCdGuid;break;
		case 3:dt=&gEfiPersistentVirtualDiskGuid;break;
		default:msgbox_alert("Invalid disk type");return;
	}
	st=gBS->LocateProtocol(
		&gEfiRamDiskProtocolGuid,
		NULL,(VOID**)&ramdisk
	);
	if(EFI_ERROR(st)||!ramdisk){
		msgbox_alert(
			"Locate RamDisk protocol failed: %s",
			_(efi_status_to_string(st))
		);
		return;
	}
	VOID*buffer=NULL;
	UINTN ms=ALIGN_VALUE(mem,4096);
	st=gBS->AllocatePool(mt,ms,&buffer);
	if(!buffer||EFI_ERROR(st)){
		msgbox_alert(
			"Failed to allocate memory: %s",
			_(efi_status_to_string(st))
		);
		return;
	}
	ZeroMem(buffer,ms);
	if(source){
		lv_fs_file_t file;
		lv_res_t r=lv_fs_open(&file,source,LV_FS_MODE_RD);
		if(r!=LV_FS_RES_OK){
			gBS->FreePool(buffer);
			msgbox_alert(
				"Failed to open file %s: %s",
				lv_fs_res_to_i18n_string(r),source
			);
			return;
		}
		EFI_FILE_PROTOCOL*fp=lv_fs_file_to_fp(&file);
		if(!fp){
			lv_fs_close(&file);
			gBS->FreePool(buffer);
			return;
		}
		EFI_FILE_INFO*info=NULL;
		st=efi_file_get_file_info(fp,NULL,&info);
		if(EFI_ERROR(st)||!info){
			lv_fs_close(&file);
			gBS->FreePool(buffer);
			msgbox_alert(
				"Get file info failed: %s",
				_(efi_status_to_string(st))
			);
			return;
		}
		if(ms<info->FileSize){
			FreePool(info);
			lv_fs_close(&file);
			gBS->FreePool(buffer);
			msgbox_alert("Memory too small to read file");
			return;
		}
		UINTN size=info->FileSize;
		st=fp->Read(fp,&size,buffer);
		if(EFI_ERROR(st)||size!=info->FileSize){
			FreePool(info);
			lv_fs_close(&file);
			gBS->FreePool(buffer);
			msgbox_alert(
				"Read file failed: %s",
				_(efi_status_to_string(st))
			);
			return;
		}
		FreePool(info);
		lv_fs_close(&file);
	}
	st=ramdisk->Register((UINT64)(UINTN)buffer,ms,dt,NULL,&dp);
	if(EFI_ERROR(st)){
		gBS->FreePool(buffer);
		msgbox_alert(
			"Failed to register ramdisk: %s",
			_(efi_status_to_string(st))
		);
		return;
	}
	guiact_do_back();
}

static void cancel_cb(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED)return;
	struct add_ramdisk*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->cancel)return;
	guiact_do_back();
}

static int init(struct gui_activity*act){
	struct add_ramdisk*am=malloc(sizeof(struct add_ramdisk));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct add_ramdisk));
	act->data=am;
	return 0;
}

static void source_cb(lv_obj_t*obj,lv_event_t e){
	lv_input_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	struct add_ramdisk*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->source)return;
	load_size(am);
}

static void size_cb(lv_obj_t*obj,lv_event_t e){
	lv_input_cb(obj,e);
	if(e!=LV_EVENT_DEFOCUSED)return;
	struct add_ramdisk*am=lv_obj_get_user_data(obj);
	if(!am||obj!=am->size)return;
	am->size_changed=true;
}

static int do_cleanup(struct gui_activity*act){
	struct add_ramdisk*am=act->data;
	if(!am)return 0;
	free(am);
	act->data=NULL;
	return 0;
}

static int add_ramdisk_resize(struct gui_activity*act){
	lv_coord_t w=act->w/8*7,h=0;
	struct add_ramdisk*am=act->data;
	lv_obj_set_style_local_pad_all(
		am->box,
		LV_PAGE_PART_BG,
		LV_STATE_DEFAULT,
		gui_font_size
	);
	lv_obj_set_width(am->page,w);
	w=lv_page_get_scrl_width(am->page);
	lv_obj_set_width(am->box,w);

	lv_obj_align_x(am->lbl_title,NULL,LV_ALIGN_CENTER,0);
	lv_obj_set_y(am->lbl_title,h);
	h+=lv_obj_get_height(am->lbl_title)+(gui_font_size/2);
	lv_obj_set_y(am->lbl_source,h);
	lv_obj_align_x(
		am->source,am->lbl_source,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->source,h);
	lv_obj_align(
		am->lbl_source,am->source,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_style_local_radius(
		am->btn_source,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_font_size/2
	);
	lv_obj_set_size(
		am->btn_source,
		gui_font_size*3,
		lv_obj_get_height(am->source)
	);
	lv_obj_set_width(
		am->source,
		w-lv_obj_get_width(am->btn_source)-
		lv_obj_get_width(am->lbl_source)-
		gui_font_size
	);
	lv_obj_align(
		am->btn_source,am->source,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/4,0
	);
	h+=lv_obj_get_height(am->btn_source)+gui_font_size;
	lv_obj_set_y(am->lbl_size,h);
	lv_obj_align_x(
		am->size,am->lbl_size,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->size,h);
	lv_obj_align(
		am->lbl_size,am->size,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	lv_obj_set_style_local_radius(
		am->btn_size,
		LV_BTN_PART_MAIN,
		LV_STATE_DEFAULT,
		gui_font_size/2
	);
	lv_obj_set_size(
		am->btn_size,
		gui_font_size*3,
		lv_obj_get_height(am->size)
	);
	lv_obj_set_width(
		am->size,
		w-lv_obj_get_width(am->btn_size)-
		lv_obj_get_width(am->lbl_size)-
		gui_font_size
	);
	lv_obj_align(
		am->btn_size,am->size,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/4,0
	);
	h+=lv_obj_get_height(am->btn_size)+gui_font_size;
	lv_obj_set_y(am->lbl_mem_type,h);
	lv_obj_set_width(
		am->mem_type,
		w-lv_obj_get_width(am->lbl_mem_type)-
		gui_font_size
	);
	lv_obj_align_x(
		am->mem_type,am->lbl_mem_type,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->mem_type,h);
	lv_obj_align(
		am->lbl_mem_type,am->mem_type,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	h+=lv_obj_get_height(am->mem_type)+gui_font_size;
	lv_obj_set_y(am->lbl_type,h);
	lv_obj_set_width(
		am->type,
		w-lv_obj_get_width(am->lbl_type)-
		gui_font_size
	);
	lv_obj_align_x(
		am->type,am->lbl_type,
		LV_ALIGN_OUT_RIGHT_MID,
		gui_font_size/2
	);
	lv_obj_set_y(am->type,h);
	lv_obj_align(
		am->lbl_type,am->type,
		LV_ALIGN_OUT_LEFT_MID,
		-gui_font_size/2,0
	);
	h+=lv_obj_get_height(am->type)+gui_font_size;
	lv_obj_set_size(
		am->ok,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		am->ok,NULL,
		LV_ALIGN_IN_TOP_LEFT,
		(gui_font_size/2),h
	);
	lv_obj_set_size(
		am->cancel,
		w/2-gui_font_size,
		gui_font_size*2
	);
	lv_obj_align(
		am->cancel,NULL,
		LV_ALIGN_IN_TOP_RIGHT,
		-(gui_font_size/2),h
	);
	h+=lv_obj_get_height(am->cancel)+(gui_font_size*2);
	lv_obj_set_height(am->box,h);
	h+=gui_font_size*2;
	lv_obj_set_height(am->page,MIN(h,(lv_coord_t)gui_sh/6*5));
	lv_obj_align(am->page,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

static int draw_add_ramdisk(struct gui_activity*act){
	char point[256];
	struct add_ramdisk*am=act->data;

	am->page=lv_page_create(act->page,NULL);
	lv_obj_set_click(am->page,false);

	am->box=lv_obj_create(am->page,NULL);
	lv_obj_set_click(am->box,false);
	lv_obj_set_style_local_border_width(am->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);

	// Title
	am->lbl_title=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_title,_("Add Ramdisk"));
	lv_label_set_long_mode(am->lbl_title,LV_LABEL_LONG_BREAK);
	lv_label_set_align(am->lbl_title,LV_LABEL_ALIGN_CENTER);

	// Source
	am->lbl_source=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_source,_("Image:"));

	am->source=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_textarea_set_one_line(am->source,true);
	lv_textarea_set_cursor_hidden(am->source,true);
	lv_obj_set_user_data(am->source,am);
	lv_obj_set_event_cb(am->source,source_cb);

	am->btn_source=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->btn_source,true);
	lv_obj_set_user_data(am->btn_source,am->source);
	lv_obj_set_event_cb(am->btn_source,sel_cb);
	lv_label_set_text(lv_label_create(am->btn_source,NULL),"...");

	// Size
	am->lbl_size=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_size,_("Memory Size:"));

	am->size=lv_textarea_create(am->box,NULL);
	lv_textarea_set_text(am->size,point);
	lv_textarea_set_one_line(am->size,true);
	lv_textarea_set_cursor_hidden(am->size,true);
	lv_textarea_set_accepted_chars(am->size,HEX"x");
	lv_textarea_set_text(am->size,"0");
	lv_obj_set_user_data(am->size,am);
	lv_obj_set_event_cb(am->size,size_cb);

	am->btn_size=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->btn_size,true);
	lv_obj_set_user_data(am->btn_size,am->size);
	lv_obj_set_event_cb(am->btn_size,sel_cb);
	lv_label_set_text(lv_label_create(am->btn_size,NULL),"...");

	// Memory Type
	am->lbl_mem_type=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_mem_type,_("Memory Type:"));

	am->mem_type=lv_dropdown_create(am->box,NULL);
	lv_obj_set_user_data(am->mem_type,am);
	lv_obj_set_event_cb(am->mem_type,lv_default_dropdown_cb);
	lv_dropdown_clear_options(am->mem_type);
	lv_dropdown_add_option(am->mem_type,_("Boot Services Data"),0);
	lv_dropdown_add_option(am->mem_type,_("Reserved Memory"),1);
	lv_dropdown_add_option(am->mem_type,_("Conventional Memory"),2);

	// Disk Type
	am->lbl_type=lv_label_create(am->box,NULL);
	lv_label_set_text(am->lbl_type,_("Disk Type:"));

	am->type=lv_dropdown_create(am->box,NULL);
	lv_obj_set_user_data(am->type,am);
	lv_obj_set_event_cb(am->type,lv_default_dropdown_cb);
	lv_dropdown_clear_options(am->type);
	lv_dropdown_add_option(am->type,_("Virtual CD"),0);
	lv_dropdown_add_option(am->type,_("Virtual Disk"),1);
	lv_dropdown_add_option(am->type,_("Persistent Virtual CD"),2);
	lv_dropdown_add_option(am->type,_("Persistent Virtual Disk"),3);
	lv_dropdown_set_selected(am->type,1);

	// OK Button
	am->ok=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->ok,true);
	lv_obj_set_user_data(am->ok,am);
	lv_obj_set_event_cb(am->ok,ok_cb);
	lv_label_set_text(lv_label_create(am->ok,NULL),_("OK"));

	// Cancel Button
	am->cancel=lv_btn_create(am->box,NULL);
	lv_style_set_action_button(am->cancel,true);
	lv_obj_set_user_data(am->cancel,am);
	lv_obj_set_event_cb(am->cancel,cancel_cb);
	lv_label_set_text(lv_label_create(am->cancel,NULL),_("Cancel"));

	if(act->args){
		load_size(am);
		size_t s=strlen(act->args);
		if(s>4&&strncasecmp(act->args+s-4,".iso",4)==0)
			lv_dropdown_set_selected(am->type,0);
	}
	return 0;
}

struct gui_register guireg_add_ramdisk={
	.name="add-ramdisk",
	.title="Add Ramdisk",
	.icon="cdrom.svg",
	.show_app=true,
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.img$",
		".*\\.raw$",
		".*\\.iso$",
		NULL
	},
	.quiet_exit=do_cleanup,
	.init=init,
	.resize=add_ramdisk_resize,
	.get_focus=add_ramdisk_get_focus,
	.lost_focus=add_ramdisk_lost_focus,
	.draw=draw_add_ramdisk,
	.back=true,
	.mask=true,
};
#endif
#endif

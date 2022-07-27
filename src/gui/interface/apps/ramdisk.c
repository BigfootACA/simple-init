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
	lv_obj_t*box,*ok,*cancel;
	lv_obj_t*btn_source;
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

static void ok_cb(lv_event_t*e){
	EFI_GUID*dt;
	EFI_STATUS st;
	EFI_MEMORY_TYPE mt;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_RAM_DISK_PROTOCOL*ramdisk=NULL;
	struct add_ramdisk*am=e->user_data;
	if(!am||e->target!=am->ok)return;
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

static void cancel_cb(lv_event_t*e){
	struct add_ramdisk*am=e->user_data;
	if(!am||e->target!=am->cancel)return;
	guiact_do_back();
}

static int init(struct gui_activity*act){
	struct add_ramdisk*am=malloc(sizeof(struct add_ramdisk));
	if(!am)return -ENOMEM;
	memset(am,0,sizeof(struct add_ramdisk));
	act->data=am;
	return 0;
}

static void source_cb(lv_event_t*e){
	struct add_ramdisk*am=e->user_data;
	if(!am||e->target!=am->source)return;
	load_size(am);
}

static void size_cb(lv_event_t*e){
	struct add_ramdisk*am=e->user_data;
	if(!am||e->target!=am->size)return;
	am->size_changed=true;
}

static int do_cleanup(struct gui_activity*act){
	struct add_ramdisk*am=act->data;
	if(!am)return 0;
	free(am);
	act->data=NULL;
	return 0;
}

static int draw_add_ramdisk(struct gui_activity*act){
	static lv_coord_t grid_2fr[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_CONTENT,
		LV_GRID_TEMPLATE_LAST
	},grid_dropdown[]={
		LV_GRID_CONTENT,
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	struct add_ramdisk*am=act->data;

	am->box=lv_obj_create(act->page);
	lv_obj_set_flex_flow(am->box,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(am->box,gui_font_size/2,0);
	lv_obj_set_style_max_width(am->box,lv_pct(80),0);
	lv_obj_set_style_max_height(am->box,lv_pct(80),0);
	lv_obj_set_style_min_width(am->box,gui_dpi*2,0);
	lv_obj_set_style_min_height(am->box,gui_font_size*2,0);
	lv_obj_set_height(am->box,LV_SIZE_CONTENT);
	lv_obj_center(am->box);

	// Title
	am->lbl_title=lv_label_create(am->box);
	lv_obj_set_width(am->lbl_title,lv_pct(100));
	lv_label_set_text(am->lbl_title,_("Add Ramdisk"));
	lv_label_set_long_mode(am->lbl_title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(am->lbl_title,LV_TEXT_ALIGN_CENTER,0);

	lv_draw_input(am->box,"Image:",NULL,NULL,&am->source,&am->btn_source);
	lv_draw_input(am->box,"Size:",NULL,NULL,&am->size,NULL);

	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_obj_add_event_cb(am->source,source_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_add_event_cb(am->size,size_cb,LV_EVENT_DEFOCUSED,am);
	lv_textarea_set_accepted_chars(am->size,HEX"x");
	lv_textarea_set_text(am->size,"0");

	lv_obj_t*fields=lv_obj_create(am->box);
	lv_obj_set_style_border_width(fields,0,0);
	lv_obj_set_style_pad_all(fields,gui_font_size/4,0);
	lv_obj_set_grid_dsc_array(fields,grid_dropdown,grid_2fr);
	lv_obj_set_size(fields,lv_pct(100),LV_SIZE_CONTENT);

	// Memory Type
	am->lbl_mem_type=lv_label_create(fields);
	lv_label_set_text(am->lbl_mem_type,_("Memory Type:"));
	lv_obj_set_grid_cell(am->lbl_mem_type,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,0,1);

	am->mem_type=lv_dropdown_create(fields);
	lv_obj_set_user_data(am->mem_type,am);
	lv_obj_add_event_cb(am->mem_type,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_grid_cell(am->mem_type,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,0,1);
	lv_dropdown_clear_options(am->mem_type);
	lv_dropdown_add_option(am->mem_type,_("Boot Services Data"),0);
	lv_dropdown_add_option(am->mem_type,_("Reserved Memory"),1);
	lv_dropdown_add_option(am->mem_type,_("Conventional Memory"),2);

	// Disk Type
	am->lbl_type=lv_label_create(fields);
	lv_label_set_text(am->lbl_type,_("Disk Type:"));
	lv_obj_set_grid_cell(am->lbl_type,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);

	am->type=lv_dropdown_create(fields);
	lv_obj_set_user_data(am->type,am);
	lv_obj_add_event_cb(am->type,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_set_grid_cell(am->type,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_CENTER,1,1);
	lv_dropdown_clear_options(am->type);
	lv_dropdown_add_option(am->type,_("Virtual CD"),0);
	lv_dropdown_add_option(am->type,_("Virtual Disk"),1);
	lv_dropdown_add_option(am->type,_("Persistent Virtual CD"),2);
	lv_dropdown_add_option(am->type,_("Persistent Virtual Disk"),3);
	lv_dropdown_set_selected(am->type,1);

	lv_obj_t*btns=lv_obj_create(am->box);
	lv_obj_set_style_border_width(btns,0,0);
	lv_obj_set_style_pad_all(btns,gui_font_size/4,0);
	lv_obj_set_grid_dsc_array(btns,grid_2fr,grid_row);
	lv_obj_set_size(btns,lv_pct(100),LV_SIZE_CONTENT);

	// OK Button
	am->ok=lv_btn_create(btns);
	lv_obj_set_enabled(am->ok,true);
	lv_obj_add_event_cb(am->ok,ok_cb,LV_EVENT_CLICKED,am);
	lv_obj_set_grid_cell(am->ok,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_ok=lv_label_create(am->ok);
	lv_label_set_text(lbl_ok,_("OK"));
	lv_obj_center(lbl_ok);

	// Cancel Button
	am->cancel=lv_btn_create(btns);
	lv_obj_set_enabled(am->cancel,true);
	lv_obj_add_event_cb(am->cancel,cancel_cb,LV_EVENT_CLICKED,am);
	lv_obj_set_grid_cell(am->cancel,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_STRETCH,0,1);
	lv_obj_t*lbl_cancel=lv_label_create(am->cancel);
	lv_label_set_text(lbl_cancel,_("Cancel"));
	lv_obj_center(lbl_cancel);

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
	.get_focus=add_ramdisk_get_focus,
	.lost_focus=add_ramdisk_lost_focus,
	.draw=draw_add_ramdisk,
	.back=true,
	.mask=true,
};
#endif
#endif

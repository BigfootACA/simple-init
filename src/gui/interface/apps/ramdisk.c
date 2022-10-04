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
#include"filesystem.h"
#include"compatible.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "ramdisk"

struct add_ramdisk{
	bool size_changed;
	lv_obj_t*box,*ok,*cancel;
	lv_obj_t*btn_source,*lbl_title;
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
	fsh*f;
	size_t len=0;
	char size[64];
	const char*source;
	if(am->size_changed)return;
	source=lv_textarea_get_text(am->source);
	if(!source||!*source)return;
	if(fs_open(NULL,&f,source,FILE_FLAG_READ)!=0)return;
	memset(size,0,sizeof(size));
	if(fs_get_size(f,&len)==0){
		snprintf(size,sizeof(size)-1,"0x%zx",len);
		lv_textarea_set_text(am->size,size);
	}
	fs_close(&f);
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
		int r;
		fsh*f=NULL;
		size_t size=0;
		if((r=fs_open(NULL,&f,source,FILE_FLAG_READ))!=0){
			gBS->FreePool(buffer);
			msgbox_alert(
				"Failed to open file %s: %s",
				source,strerror(r)
			);
			return;
		}
		if(fs_get_size(f,&size)){
			fs_close(&f);
			gBS->FreePool(buffer);
			msgbox_alert(
				"Get file size failed: %s",
				strerror(r)
			);
			return;
		}
		if(ms<size){
			fs_close(&f);
			gBS->FreePool(buffer);
			msgbox_alert("Memory too small to read file");
			return;
		}
		if((r=fs_full_read(f,buffer,size))!=0){
			fs_close(&f);
			gBS->FreePool(buffer);
			msgbox_alert(
				"Read file failed: %s",
				strerror(r)
			);
			return;
		}
		fs_close(&f);
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
	struct add_ramdisk*am=act->data;
	am->box=lv_draw_dialog_box(act->page,&am->lbl_title,"Add Ramdisk");
	lv_draw_input(am->box,"Image:",NULL,NULL,&am->source,&am->btn_source);
	lv_draw_input(am->box,"Size:",NULL,NULL,&am->size,NULL);
	lv_textarea_set_text(am->source,act->args?(char*)act->args:"");
	lv_obj_add_event_cb(am->source,source_cb,LV_EVENT_DEFOCUSED,am);
	lv_obj_add_event_cb(am->size,size_cb,LV_EVENT_DEFOCUSED,am);
	lv_textarea_set_accepted_chars(am->size,HEX"x");
	lv_textarea_set_text(am->size,"0");
	lv_draw_dropdown(am->box,"Memory Type:",&am->mem_type);
	lv_draw_dropdown(am->box,"Disk Type:",&am->type);
	lv_draw_btns_ok_cancel(am->box,&am->ok,&am->cancel,ok_cb,am);
	lv_dropdown_clear_options(am->mem_type);
	lv_dropdown_add_option(am->mem_type,_("Boot Services Data"),0);
	lv_dropdown_add_option(am->mem_type,_("Reserved Memory"),1);
	lv_dropdown_add_option(am->mem_type,_("Conventional Memory"),2);
	lv_dropdown_clear_options(am->type);
	lv_dropdown_add_option(am->type,_("Virtual CD"),0);
	lv_dropdown_add_option(am->type,_("Virtual Disk"),1);
	lv_dropdown_add_option(am->type,_("Persistent Virtual CD"),2);
	lv_dropdown_add_option(am->type,_("Persistent Virtual Disk"),3);
	lv_dropdown_set_selected(am->type,1);

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

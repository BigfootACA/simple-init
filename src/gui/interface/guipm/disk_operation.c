/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_FDISK
#define _GNU_SOURCE
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"logger.h"
#include"guipm.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "guipm"

static bool mk_label_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct fdisk_context*ctx=user_data;
	char*type=NULL;
	switch(id){
		case 0:return false;
		case 1:type="gpt";break;
		case 2:type="dos";break;
		case 3:type="bsd";break;
		case 4:type="sgi";break;
		case 5:type="sun";break;
		default:msgbox_alert("Unknown disk label");break;
	}
	tlog_debug("make new disk label %s on %s",type,fdisk_get_devname(ctx));
	if((errno=fdisk_create_disklabel(ctx,type))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("create disk label failed");
		msgbox_alert("Create disk label failed: %m");
	}
	return false;
}

static bool ask_label_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	static const char*btns[]={
		"Cancel",
		"GPT (EFI Guid)",
		"MS-DOS (MBR)",
		"BSD",
		"SGI",
		"SUN",
		""
	};
	if(id==0)msgbox_set_user_data(msgbox_create_custom(mk_label_cb,btns,"Select disk label"),user_data);
	return false;
}

static bool add_mass_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct fdisk_context*ctx=user_data;
	if(id==0){
		if(!guipm_save_label(ctx))return false;
		guiact_start_activity_by_name(
			"usb-gadget-add-mass",
			(void*)fdisk_get_devname(ctx)
		);
	}
	return false;
}

static bool disk_menu_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct fdisk_context*ctx=user_data;
	struct fdisk_label*lbl=fdisk_get_label(ctx,NULL);
	bool ro=fdisk_is_readonly(ctx);
	bool changed=fdisk_label_is_changed(lbl);
	switch(id){
		case 0:break;
		case 1:
			if(ro)goto readonly;
			msgbox_set_user_data(msgbox_create_yesno(
				ask_label_cb,
				"You will recreate the partition table of the disk. "
				"All partitions of the disk will be DELETED and ALL DATA "
				"IN THE DISK WILL BE LOST. Are you sure you want to continue?"
			),user_data);
		break;
		case 5:
			if(!changed)add_mass_cb(0,NULL,user_data);
			else msgbox_set_user_data(msgbox_create_yesno(
				add_mass_cb,
				"Partition table has been modified. "
				"Do you want to save it?"
			),user_data);
		break;
	}
	return false;
	readonly:msgbox_alert("Disk is read-only mode");
	return false;
}

void guipm_disk_operation_menu(struct fdisk_context*ctx){
	static const char*btns[]={
		"Cancel",
		"Create new disk label",
		"Erase the disk",
		"Save disk image",
		"Restore disk image",
		"USB Gadget Mass Storage",
		""
	};
	msgbox_set_user_data(msgbox_create_custom(disk_menu_cb,btns,"Select operation"),ctx);
}
#endif
#endif

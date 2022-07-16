/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdbool.h>
#include<libfdisk/libfdisk.h>
#include"logger.h"
#include"guipm.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "guipm"

enum disk_operation{
	DISK_OPER_CANCEL,
	DISK_OPER_MOUNT,
	DISK_OPER_CREATE_LABEL,
	DISK_OPER_ERASE,
	DISK_OPER_SAVE_IMAGE,
	DISK_OPER_RESTORE_IMAGE,
	DISK_OPER_USB_MASS,
	DISK_OPER_LAST,
};

static const char*disk_operations[]={
	[DISK_OPER_CANCEL]        = "Cancel",
	[DISK_OPER_MOUNT]         = "Mount the whole disk",
	[DISK_OPER_CREATE_LABEL]  = "Create new disk label",
	[DISK_OPER_ERASE]         = "Erase the disk",
	[DISK_OPER_SAVE_IMAGE]    = "Save disk image",
	[DISK_OPER_RESTORE_IMAGE] = "Restore disk image",
	[DISK_OPER_USB_MASS]      = "USB Gadget Mass Storage",
	[DISK_OPER_LAST]          = ""
};

static bool mk_label_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
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
	tlog_debug(
		"make new disk label %s on %s",
		type,fdisk_get_devname(ctx)
	);
	if((errno=fdisk_create_disklabel(ctx,type))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("create disk label failed");
		msgbox_alert("Create disk label failed: %m");
	}
	struct gui_activity*act=guiact_get_last();
	if(act)act->data_changed=true;
	return false;
}

static bool ask_label_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	static const char*btns[]={
		"Cancel",
		"GPT (EFI Guid)",
		"MS-DOS (MBR)",
		"BSD",
		"SGI",
		"SUN",
		""
	};
	if(id==0)msgbox_set_user_data(msgbox_create_custom(
		mk_label_cb,btns,"Select disk label"
	),user_data);
	return false;
}

static bool add_mass_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
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

static bool add_mount_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct fdisk_context*ctx=user_data;
	if(id==0){
		if(!guipm_save_label(ctx))return false;
		guiact_start_activity_by_name(
			"add-mount",
			(void*)fdisk_get_devname(ctx)
		);
	}
	return false;
}

static bool disk_menu_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct fdisk_context*ctx=user_data;
	bool ro=fdisk_is_readonly(ctx);
	switch(id){
		case DISK_OPER_CANCEL:break;
		case DISK_OPER_MOUNT:
			guipm_ask_save_label(ctx,add_mount_cb,user_data);
		break;
		case DISK_OPER_CREATE_LABEL:
			if(ro)goto readonly;
			msgbox_set_user_data(msgbox_create_yesno(
				ask_label_cb,
				"You will recreate the partition table of the disk. "
				"All partitions of the disk will be DELETED and ALL DATA "
				"IN THE DISK WILL BE LOST. Are you sure you want to continue?"
			),user_data);
		break;
		case DISK_OPER_USB_MASS:
			guipm_ask_save_label(ctx,add_mass_cb,user_data);
		break;
		default:msgbox_alert("This function does not implemented");break;
	}
	return false;
	readonly:msgbox_alert("Disk is read-only mode");
	return false;
}

void guipm_disk_operation_menu(struct fdisk_context*ctx){
	msgbox_set_user_data(msgbox_create_custom(
		disk_menu_cb,disk_operations,
		"Select operation"
	),ctx);
}
#endif

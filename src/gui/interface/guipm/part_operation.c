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
#include"gui/inputbox.h"
#define TAG "guipm"

enum part_operation{
	PART_OPER_CANCEL,
	PART_OPER_MOUNT,
	PART_OPER_DELETE,
	PART_OPER_RESIZE,
	PART_OPER_CHANGE_TYPE,
	PART_OPER_CHANGE_NAME,
	PART_OPER_WIPE,
	PART_OPER_ERASE,
	PART_OPER_SAVE_IMAGE,
	PART_OPER_RESTORE_IMAGE,
	PART_OPER_USB_MASS,
	PART_OPER_LAST,
};

static const char*part_operations[]={
	[PART_OPER_CANCEL]        = "Cancel",
	[PART_OPER_MOUNT]         = "Mount partition",
	[PART_OPER_DELETE]        = "Delete partition",
	[PART_OPER_RESIZE]        = "Resize partition",
	[PART_OPER_CHANGE_TYPE]   = "Change partition type",
	[PART_OPER_CHANGE_NAME]   = "Change partition name",
	[PART_OPER_WIPE]          = "Wipe partition label",
	[PART_OPER_ERASE]         = "Erase partition",
	[PART_OPER_SAVE_IMAGE]    = "Save partition image",
	[PART_OPER_RESTORE_IMAGE] = "Restore partition image",
	[PART_OPER_USB_MASS]      = "USB Gadget Mass Storage",
	[PART_OPER_LAST]          = ""
};

static bool delete_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct part_partition_info*pi=user_data;
	struct fdisk_context*ctx=pi->di->ctx;
	struct fdisk_partition*pa=pi->part;
	size_t pn=fdisk_partition_get_partno(pa);
	if(id!=0)return false;
	tlog_debug(
		"delete partition %zu on disk %s",
		pn+1,fdisk_get_devname(ctx)
	);
	if((errno=fdisk_delete_partition(ctx,pn))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("delete partition failed");
		msgbox_alert("Delete partition failed: %m");
	}
	struct gui_activity*act=guiact_get_last();
	if(act)act->data_changed=true;
	return false;
}

static bool wipe_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct part_partition_info*pi=user_data;
	struct fdisk_context*ctx=pi->di->ctx;
	struct fdisk_partition*pa=pi->part;
	size_t pn=fdisk_partition_get_partno(pa);
	if(id!=0)return false;
	tlog_debug(
		"wipe partition label %zu on disk %s",
		pn+1,fdisk_get_devname(ctx)
	);
	if((errno=fdisk_wipe_partition(ctx,pn,true))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("wipe partition label failed");
		msgbox_alert("Wipe partition label failed: %m");
	}
	fdisk_label_set_changed(fdisk_get_label(ctx,NULL),true);
	struct gui_activity*act=guiact_get_last();
	if(act)act->data_changed=true;
	return false;
}

static char*get_part_path(struct part_partition_info*pi){
	static char dev[PATH_MAX];
	if(strncmp(fdisk_get_devname(pi->di->ctx),"/dev/",5)!=0){
		msgbox_alert("Invalid disk");
		return NULL;
	}
	size_t pn=fdisk_partition_get_partno(pi->part);
	char*name=fdisk_partname(pi->di->target,pn+1);
	memset(dev,0,PATH_MAX);
	snprintf(dev,PATH_MAX-1,_PATH_DEV"/%s",name);
	return dev;
}

static bool add_mass_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct part_partition_info*pi=user_data;
	if(id==0){
		if(!guipm_save_label(pi->di->ctx))return false;
		char*dev=get_part_path(pi);
		if(!dev)return false;
		guiact_start_activity_by_name("usb-gadget-add-mass",dev);
	}
	return false;
}

static bool add_mount_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct part_partition_info*pi=user_data;
	if(id==0){
		if(!guipm_save_label(pi->di->ctx))return false;
		char*dev=get_part_path(pi);
		if(!dev)return false;
		guiact_start_activity_by_name("add-mount",dev);
	}
	return false;
}

static bool change_name_cb(
	bool ok,
	const char*content,
	void*user_data
){
	struct part_partition_info*pi=user_data;
	if(!ok||!content)return false;
	size_t pn=fdisk_partition_get_partno(pi->part);
	struct fdisk_partition*np=fdisk_new_partition();
	if(!np)return true;
	if((errno=fdisk_partition_set_name(np,content))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("set partition name failed");
		msgbox_alert("Set partition name failed: %m");
		fdisk_unref_partition(np);
		return true;
	}
	if((errno=fdisk_set_partition(pi->di->ctx,pn,np))!=0){
		if(errno<0)errno=-(errno);
		telog_warn("change partition name failed");
		msgbox_alert("Change partition name failed: %m");
		fdisk_unref_partition(np);
		return true;
	}
	tlog_debug("change partition %zu name to %s",pn+1,content);
	fdisk_unref_partition(np);
	struct gui_activity*act=guiact_get_last();
	if(act)act->data_changed=true;
	return false;
}

void do_change_name(struct part_partition_info*pi){
	struct inputbox*in=inputbox_create(change_name_cb,"Input new name");
	const char*name=fdisk_partition_get_name(pi->part);
	if(name)inputbox_set_content(in,"%s",name);
	inputbox_set_one_line(in,true);
	inputbox_set_user_data(in,pi);
}

static bool part_menu_cb(
	uint16_t id,
	const char*btn __attribute__((unused)),
	void*user_data
){
	struct part_partition_info*pi=user_data;
	struct fdisk_context*ctx=pi->di->ctx;
	bool ro=fdisk_is_readonly(ctx);
	switch(id){
		case PART_OPER_CANCEL:break;
		case PART_OPER_MOUNT:
			guipm_ask_save_label(ctx,add_mount_cb,user_data);
		break;
		case PART_OPER_DELETE:
			if(ro)goto readonly;
			msgbox_set_user_data(msgbox_create_yesno(
				delete_cb,
				"You will delete the partition. "
				"ALL DATA IN THE PARTITION WILL BE LOST. "
				"Are you sure you want to continue?"
			),user_data);
		break;
		case PART_OPER_RESIZE:
			if(ro)goto readonly;
			guiact_start_activity(
				&guireg_guipm_resize_partition,
				user_data
			);
		break;
		case PART_OPER_CHANGE_TYPE:
			if(ro)goto readonly;
			guiact_start_activity(
				&guireg_guipm_change_partition_type,
				user_data
			);
		break;
		case PART_OPER_CHANGE_NAME:
			if(ro)goto readonly;
			do_change_name(pi);
		break;
		case PART_OPER_WIPE:
			if(ro)goto readonly;
			msgbox_set_user_data(msgbox_create_yesno(
				wipe_cb,
				"You will wipe the partition label. "
				"ALL DATA IN THE PARTITION WILL BE LOST. "
				"Are you sure you want to continue?"
			),user_data);
		break;
		case PART_OPER_USB_MASS:
			guipm_ask_save_label(ctx,add_mass_cb,user_data);
		break;
		default:msgbox_alert("This function does not implemented");break;
	}
	return false;
	readonly:msgbox_alert("Disk is read-only mode");
	return false;
}

void guipm_part_operation_menu(struct part_partition_info*pi){
	if(pi->free)return;
	msgbox_set_user_data(msgbox_create_custom(
		part_menu_cb,part_operations,
		"Select operation"
	),pi);
}
#endif

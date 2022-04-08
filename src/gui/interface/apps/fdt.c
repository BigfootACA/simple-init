/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include<Library/UefiLib.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/Fdt.h>
#include<comp_libfdt.h>
#include"gui.h"
#include"uefi.h"
#include"logger.h"
#include"language.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#define TAG "fdt"

static void load_fdt(const char*path){
	int s=0;
	UINTN ps=0;
	EFI_STATUS st;
	lv_fs_res_t r;
	lv_fs_file_t f;
	bool inv=false;
	VOID*data=NULL;
	uint32_t size=0,buf=0;
	if(!path)return;
	r=lv_fs_open(&f,path,LV_FS_MODE_RD);
	if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
		"open file %s failed: %s",
		path,lv_fs_res_to_string(r)
	));
	r=lv_fs_size(&f,&size);
	if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
		"get file %s size failed: %s",
		path,lv_fs_res_to_string(r)
	));
	if(size<=sizeof(EFI_ACPI_DESCRIPTION_HEADER))
		EDONE(tlog_warn("invalid aml %s",path));
	ps=EFI_SIZE_TO_PAGES(size);
	if(!(data=AllocatePages(ps)))EDONE();
	ZeroMem(data,EFI_PAGES_TO_SIZE(ps));
	r=lv_fs_read(&f,data,size,&buf);
	if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
		"read file %s failed: %s",
		path,lv_fs_res_to_string(r)
	));
	if(buf!=size)EDONE(tlog_warn(
		"read file %s size mismatch",
		path
	));
	inv=true;
	if((s=fdt_check_header(data))<0)EDONE(tlog_warn(
		"fdt %s header invalid: %s",
		path,fdt_strerror(s)
	));
	if(fdt_totalsize(data)!=size)EDONE(tlog_warn(
		"fdt file %s size mismatch",
		path
	));
	lv_fs_close(&f);
	f.file_d=NULL;
	st=gBS->InstallConfigurationTable(
		&gFdtTableGuid,
		data
	);
	if(EFI_ERROR(st))EDONE(msgbox_alert(
		"Install device tree failed: %s",
		_(efi_status_to_string(st))
	));
	msgbox_alert("Load %s done",path);
	return;
	done:
	if(data)FreePages(data,ps);
	if(f.file_d){
		lv_fs_close(&f);
		if(inv)msgbox_alert("Invalid device tree %s",path);
		else msgbox_alert("Read file %s failed",path);
	}
}

static bool fdt_load_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	char*path=user_data;
	if(!path)return true;
	if(id==0)load_fdt(path);
	return false;
}

static int fdt_load_draw(struct gui_activity*act){
	if(!act||!act->args)return -1;
	char*path=act->args;
	msgbox_set_user_data(msgbox_create_yesno(
		fdt_load_cb,
		"Load device tree blob from '%s'?",
		path
	),path);
	return -10;
}

struct gui_register guireg_fdt_load={
	.name="fdt-load",
	.title="Load device tree blob",
	.icon="linux.svg",
	.open_regex=(char*[]){
		".*\\.dtb$",
		".*\\.fdt$",
		".*/dtb$",
		".*/fdt$",
		NULL
	},
	.show_app=false,
	.open_file=true,
	.draw=fdt_load_draw,
};

#endif

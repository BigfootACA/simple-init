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
#include<Library/DebugLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/AcpiSystemDescriptionTable.h>
#include<Protocol/AcpiTable.h>
#include<Protocol/SimpleFileSystem.h>
#include<IndustryStandard/Acpi.h>
#include<Guid/Acpi.h>
#include"str.h"
#include"gui.h"
#include"uefi.h"
#include"logger.h"
#include"language.h"
#include"compatible.h"
#include"gui/tools.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "acpi"

static lv_obj_t*lst=NULL,*last=NULL,*title=NULL;
static lv_obj_t*info,*btn_load,*btn_refresh,*btn_save;
struct acpi_info{
	bool enable;
	lv_obj_t*btn,*chk;
	lv_obj_t*name,*info;
	EFI_ACPI_DESCRIPTION_HEADER*table;
	CHAR8 type[8],oem[8];
};
static list*acpis=NULL;
static struct acpi_info*selected=NULL;

static void item_check(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected&&obj!=selected->chk){
		lv_checkbox_set_checked(selected->chk,false);
		lv_obj_set_checked(selected->btn,false);
	}
	selected=lv_obj_get_user_data(obj);
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_save,true);
}

static void free_acpi_info(struct acpi_info*m){
	if(!m)return;
	memset(m,0,sizeof(struct acpi_info));
	free(m);
}

static int item_free(void*d){
	struct acpi_info*mi=d;
	free_acpi_info(mi);
	return 0;
}

static int item_free_del(void*d){
	struct acpi_info*mi=d;
	lv_obj_del(mi->btn);
	free_acpi_info(mi);
	return 0;
}

static void acpi_clear(){
	selected=NULL;
	lv_obj_set_enabled(btn_save,false);
	if(info)lv_obj_del(info);
	list_free_all(acpis,item_free_del);
	info=NULL,last=NULL,acpis=NULL;
}

void acpi_set_info(char*text){
	acpi_clear();
	info=lv_label_create(lst,NULL);
	lv_label_set_long_mode(info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(info,lv_page_get_scrl_width(lst),gui_sh/16);
	lv_label_set_align(info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(info,text);
}

static bool acpi_add_item(EFI_ACPI_DESCRIPTION_HEADER*table){
	lv_coord_t bw=lv_page_get_scrl_width(lst);
	lv_coord_t bm=gui_dpi/20;
	if(!table)return false;
	struct acpi_info*k=malloc(sizeof(struct acpi_info));
	if(!k)return false;
	memset(k,0,sizeof(struct acpi_info));
	k->table=table;
	CopyMem(k->type,&table->Signature,sizeof(table->Signature));
	CopyMem(k->oem,&table->OemId,sizeof(table->OemId));

	// button
	k->btn=lv_btn_create(lst,NULL);
	lv_obj_set_size(k->btn,bw,gui_dpi/2);
	lv_obj_align(
		k->btn,last,
		last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
		0,last?gui_dpi/8:0
	);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);
	last=k->btn;

	lv_obj_t*line=lv_line_create(k->btn,NULL);
	lv_obj_set_width(line,bw);

	// checkbox
	k->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(k->chk,"");
	lv_obj_set_event_cb(k->chk,item_check);
	lv_obj_set_user_data(k->chk,k);
	lv_style_set_focus_checkbox(k->chk);
	lv_obj_align(k->chk,k->btn,LV_ALIGN_IN_TOP_LEFT,bm,bm);
	lv_group_add_obj(gui_grp,k->chk);

	// name
	k->name=lv_label_create(line,NULL);
	lv_obj_align(k->name,k->chk,LV_ALIGN_OUT_RIGHT_MID,0,0);
	lv_label_set_text_fmt(k->name,"%s v%d",k->type,table->Revision);
	lv_label_set_long_mode(k->name,LV_LABEL_LONG_DOT);

	// info
	k->info=lv_label_create(line,NULL);
	lv_obj_set_small_text_font(k->info,LV_LABEL_PART_MAIN);
	lv_obj_align(k->info,k->btn,LV_ALIGN_IN_BOTTOM_LEFT,bm,-bm);
	lv_label_set_text_fmt(
		k->info,
		_("Address: 0x%llx Size: %d OEM ID: %s"),
		(long long)table,table->Length,table->OemId
	);

	return true;
}

static void acpi_reload(){
	acpi_clear();
	bool found=false;
	for(UINTN i=0;i<gST->NumberOfTableEntries;i++){
		EFI_CONFIGURATION_TABLE*t=&gST->ConfigurationTable[i];
		EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER*sdt=t->VendorTable;
		if(
			!CompareGuid(&t->VendorGuid,&gEfiAcpi10TableGuid)&&
			!CompareGuid(&t->VendorGuid,&gEfiAcpi20TableGuid)
		)continue;
		if(!sdt||sdt->Signature!=EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE)continue;
		EFI_ACPI_SDT_HEADER*xsdt=(VOID*)sdt->XsdtAddress;
		if(!xsdt||xsdt->Signature!=EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)continue;
		UINT64*ptr=(UINT64*)(xsdt+1);
		UINTN cnt=(xsdt->Length-sizeof(EFI_ACPI_SDT_HEADER))/sizeof(UINT64);
		for(UINTN x=0;x<cnt;x++,ptr++) {
			EFI_ACPI_DESCRIPTION_HEADER*e=(VOID*)*ptr;
			if(acpi_add_item(e))found=true;
			if(e->Signature!=EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE)continue;
			EFI_ACPI_5_1_FIXED_ACPI_DESCRIPTION_TABLE*fadt=(VOID*)e;
			if(acpi_add_item(
				(VOID*)((e->Revision>=3&&fadt->XDsdt)?
				(UINTN)fadt->XDsdt:
				(UINTN)fadt->Dsdt
			)))found=true;
			if(acpi_add_item(
				(VOID*)((e->Revision>=3&&fadt->XFirmwareCtrl)?
				(UINTN)fadt->XFirmwareCtrl:
				(UINTN)fadt->FirmwareCtrl
			)))found=true;
		}
	}
	if(!found)acpi_set_info(_("No any ACPI tables found"));
}

static void load_aml(const char*path){
	UINTN k=0;
	EFI_STATUS st;
	lv_fs_res_t r;
	lv_fs_file_t f;
	uint32_t size=0,buf=0;
	EFI_ACPI_TABLE_PROTOCOL*p=NULL;
	EFI_ACPI_DESCRIPTION_HEADER*data=NULL;
	if(!path)return;
	st=gBS->LocateProtocol(
		&gEfiAcpiTableProtocolGuid,
		NULL,(VOID**)&p
	);
	if(EFI_ERROR(st)){
		msgbox_alert(
			"Locate ACPI Protocol failed: %s",
			_(efi_status_to_string(st))
		);
		return;
	}
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
	if(!(data=AllocateZeroPool(size+1)))EDONE();
	r=lv_fs_read(&f,data,size,&buf);
	if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
		"read file %s failed: %s",
		path,lv_fs_res_to_string(r)
	));
	if(buf!=size)EDONE(tlog_warn(
		"read file %s size mismatch",
		path
	));
	if(data->Length!=size)EDONE(tlog_warn(
		"aml file %s size mismatch",
		path
	));
	lv_fs_close(&f);
	f.file_d=NULL;
	st=p->InstallAcpiTable(p,data,size,&k);
	if(EFI_ERROR(st))EDONE(msgbox_alert(
		"Install ACPI Table failed: %s",
		_(efi_status_to_string(st))
	));
	FreePool(data);
	msgbox_alert("Load %s done",path);
	return;
	done:
	if(data)FreePool(data);
	if(f.file_d){
		lv_fs_close(&f);
		msgbox_alert("Read file %s failed",path);
	}
}

static void save_aml(const char*path,EFI_ACPI_DESCRIPTION_HEADER*table){
	UINTN s=0;
	EFI_STATUS st;
	lv_fs_res_t r;
	lv_fs_file_t f;
	EFI_FILE_PROTOCOL*fp;
	EFI_FILE_INFO*info=NULL;
	if(!path||!table)return;
	r=lv_fs_open(&f,path,LV_FS_MODE_WR);
	if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
		"open file %s failed: %s",
		path,lv_fs_res_to_string(r)
	));
	if(!(fp=lv_fs_file_to_fp(&f)))EDONE();
	st=efi_file_get_file_info(fp,&s,&info);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"get file info %s failed: %s",
		path,efi_status_to_string(st)
	));
	info->FileSize=0;
	st=fp->SetInfo(fp,&gEfiFileInfoGuid,s,info);
	if(EFI_ERROR(st)){
		st=fp->Delete(fp);
		r=lv_fs_open(&f,path,LV_FS_MODE_WR);
		if(r!=LV_FS_RES_OK)EDONE(tlog_warn(
			"open file %s failed: %s",
			path,lv_fs_res_to_string(r)
		));
		if(!(fp=lv_fs_file_to_fp(&f)))EDONE();
	}
	fp->SetPosition(fp,0);
	s=table->Length;
	st=fp->Write(fp,&s,table);
	if(EFI_ERROR(st))EDONE(tlog_warn(
		"write file %s failed: %s",
		path,efi_status_to_string(st)
	));
	if(s!=table->Length)EDONE(tlog_warn(
		"write file %s size mismatch",
		path
	));
	FreePool(info);
	lv_fs_close(&f);
	msgbox_alert("Save %s done",path);
	return;
	done:
	if(info)FreePool(info);
	if(f.file_d)lv_fs_close(&f);
	msgbox_alert("Save %s failed",path);
}

static bool load_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!path||!path[0]||path[1]||cnt!=1)return true;
	load_aml(path[0]);
	return false;
}

static bool save_cb(bool ok,const char**path,uint16_t cnt,void*user_data){
	struct acpi_info*k=user_data;
	if(!ok)return false;
	if(!k||!k->table||!path||!path[0]||path[1]||cnt!=1)return true;
	save_aml(path[0],k->table);
	return false;
}

static void load_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_load)return;
	tlog_debug("request load");
	struct filepicker*fp=filepicker_create(load_cb,"Select aml file to load");
	filepicker_set_max_item(fp,1);
}

static void save_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_save||!selected)return;
	tlog_debug("request save");
	struct filepicker*fp=filepicker_create(save_cb,"Select aml file to save");
	filepicker_set_max_item(fp,1);
	filepicker_set_user_data(fp,selected);
}

static void refresh_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_refresh)return;
	tlog_debug("request refresh");
	acpi_reload();
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	list_free_all(acpis,item_free);
	info=NULL,last=NULL,acpis=NULL;
	return 0;
}

static int do_load(struct gui_activity*d){
	acpi_reload();
	return 0;
}

static int acpi_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,btn_load);
	lv_group_add_obj(gui_grp,btn_refresh);
	lv_group_add_obj(gui_grp,btn_save);
	return 0;
}

static int acpi_lost_focus(struct gui_activity*d __attribute__((unused))){
	list*o=list_first(acpis);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct acpi_info*);
		lv_group_remove_obj(item->chk);
	}while((o=o->next));
	lv_group_remove_obj(btn_load);
	lv_group_remove_obj(btn_refresh);
	lv_group_remove_obj(btn_save);
	return 0;
}

static int acpi_resize(struct gui_activity*act){
	lv_coord_t btm=gui_dpi/10;
	lv_coord_t btw=act->w/3-btm;
	lv_coord_t bth=gui_font_size+gui_dpi/10;

	lv_obj_set_pos(title,0,gui_font_size);
	lv_obj_set_width(title,act->w);

	lv_obj_align(lst,title,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size);
	lv_obj_set_x(lst,gui_font_size/2);
	lv_obj_set_size(lst,act->w-gui_font_size,act->h-lv_obj_get_y(lst)-bth-btm*2);

	lv_obj_set_size(btn_load,btw,bth);
	lv_obj_align(btn_load,lst,LV_ALIGN_OUT_BOTTOM_LEFT,0,btm);

	lv_obj_set_size(btn_refresh,btw,bth);
	lv_obj_align(btn_refresh,lst,LV_ALIGN_OUT_BOTTOM_MID,0,btm);

	lv_obj_set_size(btn_save,btw,bth);
	lv_obj_align(btn_save,lst,LV_ALIGN_OUT_BOTTOM_RIGHT,0,btm);

	acpi_reload();
	return 0;
}

static int acpi_draw(struct gui_activity*act){

	// function title
	title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("ACPI Manager"));

	// options list
	lst=lv_page_create(act->page,NULL);
	lv_obj_set_style_local_border_width(lst,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(lst,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(lst,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);

	// load button
	btn_load=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_load,load_click);
	lv_style_set_action_button(btn_load,true);
	lv_label_set_text(lv_label_create(btn_load,NULL),_("Load"));

	// refresh button
	btn_refresh=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_refresh,refresh_click);
	lv_style_set_action_button(btn_refresh,true);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),_("Refresh"));

	// save button
	btn_save=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(btn_save,save_click);
	lv_style_set_action_button(btn_save,true);
	lv_label_set_text(lv_label_create(btn_save,NULL),_("Save"));

	return 0;
}

static bool acpi_load_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	char*path=user_data;
	if(!path)return true;
	if(id==0)load_aml(path);
	return false;
}

static int acpi_load_draw(struct gui_activity*act){
	if(!act||!act->args)return -1;
	char*path=act->args;
	msgbox_set_user_data(msgbox_create_yesno(
		acpi_load_cb,
		"Load ACPI table from '%s'?",
		path
	),path);
	return -10;
}

struct gui_register guireg_acpi_load={
	.name="acpi-load",
	.title="Load ACPI Table",
	.icon="acpi.svg",
	.open_regex=(char*[]){
		".*\\.aml$",
		NULL
	},
	.show_app=false,
	.open_file=true,
	.draw=acpi_load_draw,
};

struct gui_register guireg_acpi_manager={
	.name="acpi-manager",
	.title="ACPI Manager",
	.icon="acpi.svg",
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.aml$",
		NULL
	},
	.show_app=true,
	.draw=acpi_draw,
	.resize=acpi_resize,
	.quiet_exit=do_cleanup,
	.get_focus=acpi_get_focus,
	.lost_focus=acpi_lost_focus,
	.data_load=do_load,
	.back=true,
};
#endif

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
#include"filesystem.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/filepicker.h"
#define TAG "acpi"

struct acpi_mgr{
	list*acpis;
	struct acpi_info*selected;
	lv_obj_t*lst,*title;
	lv_obj_t*info,*btn_load,*btn_refresh,*btn_save;
};

struct acpi_info{
	struct acpi_mgr*am;
	bool enable;
	lv_obj_t*btn;
	lv_obj_t*name,*info;
	EFI_ACPI_DESCRIPTION_HEADER*table;
	CHAR8 type[8],oem[8];
};

static void item_check(lv_event_t*e){
	struct acpi_info*ai=e->user_data;
	struct acpi_mgr*am=ai->am;
	lv_obj_set_checked(e->target,true);
	if(am->selected&&e->target!=am->selected->btn)
		lv_obj_set_checked(am->selected->btn,false);
	am->selected=ai;
	lv_obj_set_checked(am->selected->btn,true);
	lv_obj_set_enabled(am->btn_save,true);
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
	lv_group_remove_obj(mi->btn);
	lv_obj_del(mi->btn);
	free_acpi_info(mi);
	return 0;
}

static void acpi_clear(struct acpi_mgr*am){
	am->selected=NULL;
	lv_obj_set_enabled(am->btn_save,false);
	if(am->info)lv_obj_del(am->info);
	list_free_all(am->acpis,item_free_del);
	am->info=NULL,am->acpis=NULL;
}

static void acpi_set_info(struct acpi_mgr*am,char*text){
	acpi_clear(am);
	am->info=lv_label_create(am->lst);
	lv_label_set_long_mode(am->info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(am->info,lv_pct(100));
	lv_obj_set_style_text_align(am->info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(am->info,text);
}

static bool acpi_add_item(struct acpi_mgr*am,EFI_ACPI_DESCRIPTION_HEADER*table){
	static lv_coord_t grid_col[]={
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	},grid_row[]={
		LV_GRID_FR(1),
		LV_GRID_FR(1),
		LV_GRID_TEMPLATE_LAST
	};
	if(!table)return false;
	struct acpi_info*k=malloc(sizeof(struct acpi_info));
	if(!k)return false;
	memset(k,0,sizeof(struct acpi_info));
	k->table=table,k->am=am;
	CopyMem(k->type,&table->Signature,sizeof(table->Signature));
	CopyMem(k->oem,&table->OemId,sizeof(table->OemId));

	// button
	k->btn=lv_btn_create(am->lst);
	lv_obj_set_size(k->btn,lv_pct(100),gui_dpi/2);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_grid_dsc_array(k->btn,grid_col,grid_row);
	lv_obj_add_event_cb(k->btn,item_check,LV_EVENT_CLICKED,k);
	lv_group_add_obj(gui_grp,k->btn);

	// name
	k->name=lv_label_create(k->btn);
	lv_label_set_text_fmt(k->name,"%s v%d",k->type,table->Revision);
	lv_label_set_long_mode(k->name,LV_LABEL_LONG_DOT);
	lv_obj_set_grid_cell(k->name,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,0,1);

	// info
	k->info=lv_label_create(k->btn);
	lv_obj_set_small_text_font(k->info,LV_PART_MAIN);
	lv_label_set_text_fmt(
		k->info,
		_("Address: 0x%llx Size: %d OEM ID: %s"),
		(long long)(UINTN)table,table->Length,table->OemId
	);
	lv_obj_set_grid_cell(k->info,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);

	list_obj_add_new(&am->acpis,k);
	return true;
}

static void acpi_reload(struct acpi_mgr*am){
	bool found=false;
	acpi_clear(am);
	for(UINTN i=0;i<gST->NumberOfTableEntries;i++){
		EFI_CONFIGURATION_TABLE*t=&gST->ConfigurationTable[i];
		EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER*sdt=t->VendorTable;
		if(
			!CompareGuid(&t->VendorGuid,&gEfiAcpi10TableGuid)&&
			!CompareGuid(&t->VendorGuid,&gEfiAcpi20TableGuid)
		)continue;
		if(!sdt||sdt->Signature!=EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE)continue;
		EFI_ACPI_SDT_HEADER*xsdt=(VOID*)(UINTN)sdt->XsdtAddress;
		if(!xsdt||xsdt->Signature!=EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)continue;
		UINT64*ptr=(UINT64*)(xsdt+1);
		UINTN cnt=(xsdt->Length-sizeof(EFI_ACPI_SDT_HEADER))/sizeof(UINT64);
		for(UINTN x=0;x<cnt;x++,ptr++) {
			EFI_ACPI_DESCRIPTION_HEADER*e=(VOID*)(UINTN)*ptr;
			if(acpi_add_item(am,e))found=true;
			if(e->Signature!=EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE)continue;
			EFI_ACPI_5_1_FIXED_ACPI_DESCRIPTION_TABLE*fadt=(VOID*)e;
			if(acpi_add_item(
				am,
				(VOID*)((e->Revision>=3&&fadt->XDsdt)?
				(UINTN)fadt->XDsdt:
				(UINTN)fadt->Dsdt
			)))found=true;
			if(acpi_add_item(
				am,
				(VOID*)((e->Revision>=3&&fadt->XFirmwareCtrl)?
				(UINTN)fadt->XFirmwareCtrl:
				(UINTN)fadt->FirmwareCtrl
			)))found=true;
		}
	}
	if(!found)acpi_set_info(am,_("No any ACPI tables found"));
}

static void load_aml(const char*path){
	int r=0;
	UINTN k=0;
	fsh*f=NULL;
	size_t size;
	EFI_STATUS st;
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
	r=fs_open(NULL,&f,path,FILE_FLAG_READ);
	if(r!=0)EDONE(tlog_warn(
		"open file %s failed: %s",
		path,strerror(r)
	));
	r=fs_read_all(f,(void**)&data,&size);
	if(r!=0)EDONE(tlog_warn(
		"read file %s failed: %s",
		path,strerror(r)
	));
	if(size<=sizeof(EFI_ACPI_DESCRIPTION_HEADER))
		EDONE(tlog_warn("invalid aml %s",path));
	if(data->Length!=size)EDONE(tlog_warn(
		"aml file %s size mismatch",path
	));
	fs_close(&f);
	f=NULL;
	st=p->InstallAcpiTable(p,data,size,&k);
	if(EFI_ERROR(st))EDONE(msgbox_alert(
		"Install ACPI Table failed: %s",
		_(efi_status_to_string(st))
	));
	free(data);
	msgbox_alert("Load %s done",path);
	return;
	done:
	if(data)free(data);
	if(f){
		fs_close(&f);
		msgbox_alert("Read file %s failed",path);
	}
}

static void save_aml(const char*path,EFI_ACPI_DESCRIPTION_HEADER*table){
	int r;
	fsh*f=NULL;
	if(!path||!table)return;
	r=fs_open(
		NULL,&f,path,
		FILE_FLAG_READWRITE|
		FILE_FLAG_CREATE|
		FILE_FLAG_TRUNCATE
	);
	if(r!=0)EDONE(tlog_warn(
		"open file %s failed: %s",
		path,strerror(r)
	));
	r=fs_full_write(f,table,table->Length);
	if(r!=0)EDONE(tlog_warn(
		"write file %s failed: %s",
		path,strerror(r)
	));
	fs_close(&f);
	msgbox_alert("Save %s done",path);
	return;
	done:
	if(f)fs_close(&f);
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

static void load_click(lv_event_t*e){
	struct acpi_mgr*am=e->user_data;
	if(!am||e->target!=am->btn_load)return;
	tlog_debug("request load");
	struct filepicker*fp=filepicker_create(load_cb,"Select aml file to load");
	filepicker_set_max_item(fp,1);
}

static void save_click(lv_event_t*e){
	struct acpi_mgr*am=e->user_data;
	if(!am||e->target!=am->btn_save||!am->selected)return;
	tlog_debug("request save");
	struct filepicker*fp=filepicker_create(save_cb,"Select aml file to save");
	filepicker_set_max_item(fp,1);
	filepicker_set_user_data(fp,am->selected);
}

static void refresh_click(lv_event_t*e){
	struct acpi_mgr*am=e->user_data;
	if(!am||e->target!=am->btn_refresh)return;
	tlog_debug("request refresh");
	acpi_reload(am);
}

static int do_cleanup(struct gui_activity*d){
	struct acpi_mgr*am=d->data;
	if(!am)return 0;
	list_free_all(am->acpis,item_free);
	am->info=NULL,am->acpis=NULL;
	free(am);
	d->data=NULL;
	return 0;
}

static int do_load(struct gui_activity*d){
	struct acpi_mgr*am=d->data;
	if(am)acpi_reload(am);
	return 0;
}

static int acpi_get_focus(struct gui_activity*d){
	struct acpi_mgr*am=d->data;
	if(!am)return 0;
	lv_group_add_obj(gui_grp,am->btn_load);
	lv_group_add_obj(gui_grp,am->btn_refresh);
	lv_group_add_obj(gui_grp,am->btn_save);
	return 0;
}

static int acpi_lost_focus(struct gui_activity*d){
	struct acpi_mgr*am=d->data;
	if(!am)return 0;
	list*o=list_first(am->acpis);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct acpi_info*);
		lv_group_remove_obj(item->btn);
	}while((o=o->next));
	lv_group_remove_obj(am->btn_load);
	lv_group_remove_obj(am->btn_refresh);
	lv_group_remove_obj(am->btn_save);
	return 0;
}

static int acpi_init(struct gui_activity*act){
	struct acpi_mgr*am;
	if(!(am=malloc(sizeof(struct acpi_mgr))))ERET(ENOMEM);
	memset(am,0,sizeof(struct acpi_mgr));
	act->data=am;
	return 0;
}

static int acpi_draw(struct gui_activity*act){
	struct acpi_mgr*am=act->data;
	if(!am)return -1;
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// function title
	am->title=lv_label_create(act->page);
	lv_obj_set_width(am->title,lv_pct(100));
	lv_label_set_long_mode(am->title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(am->title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(am->title,_("ACPI Manager"));

	// options list
	am->lst=lv_obj_create(act->page);
	lv_obj_set_flex_flow(am->lst,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(am->lst,1);
	lv_obj_set_width(am->lst,lv_pct(100));

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,cb,x)&(struct button_dsc){\
			&tgt,true,_(title),cb,am,x,1,0,1,NULL\
		}
		BTN(am->btn_load,    "Load",    load_click,    0),
		BTN(am->btn_refresh, "Refresh", refresh_click, 1),
		BTN(am->btn_save,    "Save",    save_click,    2),
		#undef BTN
		NULL
	);

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
	.open_file=true,
	.open_regex=(char*[]){
		".*\\.aml$",
		NULL
	},
	.show_app=true,
	.init=acpi_init,
	.draw=acpi_draw,
	.quiet_exit=do_cleanup,
	.get_focus=acpi_get_focus,
	.lost_focus=acpi_lost_focus,
	.data_load=do_load,
	.back=true,
};
#endif

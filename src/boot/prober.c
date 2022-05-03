/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include"boot.h"
#include"list.h"
#include"confd.h"
#include"keyval.h"
#include"locate.h"
#include"logger.h"
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/UefiBootManagerLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/DevicePath.h>
#include<Protocol/SimpleFileSystem.h>
#define TAG "prober"

static struct boot_config def={
	.mode=BOOT_EFI,
	.save=false,.replace=false,
	.show=true,.enabled=true
};

static bool boot_scan_part(int part,int*id,char*tag,EFI_FILE_PROTOCOL*root){
	list*probes=NULL;
	CHAR16*path16=NULL;
	CHAR8*path8=NULL,*locate=NULL;
	UINTN ls=PATH_MAX*sizeof(CHAR8);
	UINTN ps=PATH_MAX*sizeof(CHAR16);
	EFI_FILE_PROTOCOL*fp=NULL;
	EFI_STATUS st;
	bool found=false;
	char*dir,*name;
	struct efi_path*ep;
	struct boot_config*use=NULL;
	if(!(use=AllocateZeroPool(sizeof(struct boot_config))))goto done;
	if(!(path16=AllocateZeroPool(ps)))goto done;
	if(!(locate=AllocateZeroPool(ls)))goto done;
	for(size_t p=0;(ep=&boot_efi_paths[p])&&ep->title;p++){
		if(
			!ep->enable||
			!ep->name||!ep->name[0]||
			!ep->title||!ep->title[0]
		)continue;
		if(ep->cpu!=CPU_ANY&&ep->cpu!=current_cpu)continue;
		for(size_t d=0;(dir=ep->dir[d]);d++)for(size_t d=0;(name=ep->name[d]);d++){
			ZeroMem(locate,ls);
			ZeroMem(path16,ps);
			AsciiSPrint(locate,ls,"@%a:%a%a",tag,dir,name);
			path8=locate+strlen(tag)+2;
			UnicodeSPrint(path16,ps,L"%a%a",dir,name);
			if(list_search_case_string(probes,path8))continue;
			st=root->Open(root,&fp,path16,EFI_FILE_MODE_READ,0);
			if(EFI_ERROR(st))continue;
			tlog_debug("found %s at %s from part %d",ep->title,path8,part);
			list_obj_add_new_strdup(&probes,path8);
			fp->Close(fp);
			CopyMem(use,&def,sizeof(struct boot_config));
			AsciiSPrint(use->desc,sizeof(use->desc),"%a on #%d",ep->title,part);
			AsciiSPrint(use->ident,sizeof(use->ident),"prober-%d",*id);
			AsciiStrCpyS(use->icon,sizeof(use->icon),ep->icon);
			boot_create_config(use,NULL);
			confd_set_string_base(use->key,"efi_file",locate);
			if(ep->load_opt){
				confd_set_string_base(use->key,"options",ep->load_opt);
				confd_set_boolean_base(use->key,"options_widechar",ep->unicode);
			}
			(*id)++,found=true;
		}
	}
	done:
	if(use)FreePool(use);
	if(path16)FreePool(path16);
	if(locate)FreePool(locate);
	list_free_all_def(probes);
	return found;
}

void boot_scan_efi(){
	int id=0;
	UINTN cnt=0;
	EFI_STATUS st;
	CHAR16*dt=NULL;
	CHAR8*xt=NULL,loc_name[255];
	EFI_HANDLE*hands=NULL;
	EFI_FILE_PROTOCOL*root=NULL;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs=NULL;
	if(!confd_get_boolean("boot.uefi_probe",true))return;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSimpleFileSystemProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"locate Simple File System Protocol failed: %s",
			efi_status_to_string(st)
		);
		return;
	}
	if(!(xt=AllocatePool(PATH_MAX)))return;
	for(UINTN i=0;i<cnt;i++){
		if(
			!(dp=DevicePathFromHandle(hands[i]))||
			!(dt=ConvertDevicePathToText(dp,FALSE,FALSE))
		){
			tlog_warn("get device path failed");
			continue;
		}
		if(!locate_find_name(loc_name,sizeof(loc_name)))break;
		st=gBS->HandleProtocol(
			hands[i],
			&gEfiSimpleFileSystemProtocolGuid,(VOID**)&fs);
		if(EFI_ERROR(st)||!fs){
			tlog_warn(
				"locate Simple File System Protocol failed: %s",
				efi_status_to_string(st)
			);
			continue;
		}
		st=fs->OpenVolume(fs,&root);
		if(EFI_ERROR(st)||!root){
			tlog_warn(
				"open volume failed: %s",
				efi_status_to_string(st)
			);
			continue;
		}
		ZeroMem(xt,PATH_MAX);
		UnicodeStrToAsciiStrS(dt,xt,PATH_MAX);
		tlog_debug("scan part #%d %s",(int)i,xt);
		if(boot_scan_part((int)i,&id,loc_name,root))
			locate_add_by_device_path(loc_name,false,dp);
	}
	FreePool(xt);
	tlog_debug("%d items found",id);
}
#endif

/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"
static list*locate_cache=NULL;

static void init_locate(locate_dest*loc,const char*tag,EFI_HANDLE*hand){
	ZeroMem(loc,sizeof(locate_dest));
	AsciiStrCpyS(loc->tag,sizeof(loc->tag)-1,tag);
	loc->file_hand=hand;
	gBS->HandleProtocol(
		hand,
		&gEfiPartitionInfoProtocolGuid,
		(VOID**)&loc->part_proto
	);
	gBS->HandleProtocol(
		hand,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&loc->file_proto
	);
	gBS->HandleProtocol(
		hand,
		&gEfiBlockIoProtocolGuid,
		(VOID**)&loc->block_proto
	);
	if(loc->file_proto)loc->file_proto->OpenVolume(
		loc->file_proto,
		&loc->root
	);
}

static locate_dest*new_locate(const char*tag,EFI_HANDLE*hand){
	locate_dest*loc=malloc(sizeof(locate_dest));
	if(!loc)return NULL;
	init_locate(loc,tag,hand);
	return loc;
}

static int init_default(){
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	EFI_STATUS st=gBS->OpenProtocol(
		gImageHandle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,
		gImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"open protocol failed: %s",
			efi_status_to_string(st)
		);
		return -1;
	}
	locate_dest*loc=new_locate("default",li->DeviceHandle);
	if(!loc){
		tlog_error("create default locate failed");
		return -1;
	}
	list_obj_add_new(&locate_cache,loc);
	return 0;
}

static const char*get_match_name(enum locate_match_state st){
	switch(st){
		case MATCH_NONE:return "none";
		case MATCH_SKIP:return "skip";
		case MATCH_SUCCESS:return "success";
		case MATCH_INVALID:return "invalid";
		case MATCH_FAILED:return "failed";
		default:return "unknown";
	}
}

static bool try_protocol(const char*tag,locate_dest*loc,EFI_GUID*protocol){
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	CHAR16*dpt=NULL;
	char dpx[PATH_MAX],guid[64];
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	AsciiSPrint(guid,sizeof(guid),"%g",protocol);
	tlog_verbose("try locate protocol in guid %s",guid);
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		protocol,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_warn(
			"locate protocol %s failed: %s",
			guid,efi_status_to_string(st)
		);
		return false;
	}
	tlog_debug(
		"found %llu handles in protocol %s",
		(unsigned long long)cnt,guid
	);
	for(UINTN i=0;i<cnt;i++){
		init_locate(loc,tag,hands[i]);
		tlog_verbose(
			"try match handle %p (%llu)",
			hands[i],(unsigned long long)i
		);
		for(UINTN s=0;locate_matches[s];s++){
			enum locate_match_state st=locate_matches[s](loc);
			if(st!=MATCH_SKIP)tlog_verbose(
				"handle %p (%llu) match %llu state %s",
				hands[i],(unsigned long long)i,
				(unsigned long long)s,get_match_name(st)
			);
			switch(st){
				case MATCH_NONE:
				case MATCH_SKIP:
				case MATCH_SUCCESS:continue;
				case MATCH_INVALID:return false;
				case MATCH_FAILED:goto match_fail;
			}
		}
		ZeroMem(dpx,sizeof(dpx));
		if(
			(dp=DevicePathFromHandle(hands[i]))&&
			(dpt=ConvertDevicePathToText(dp,TRUE,FALSE))
		){
			UnicodeStrToAsciiStrS(dpt,dpx,sizeof(dpx)-1);
			FreePool(dpt);
		}
		if(!dpx[0])AsciiStrCpyS(dpx,sizeof(dpx)-1,"(Unknown)");
		tlog_info("found locate %s",dpx);
		return true;
		match_fail:;
	}
	return false;
}

static locate_dest*load_locate(const char*tag){
	locate_dest*loc=AllocatePool(sizeof(locate_dest));
	if(!loc)return NULL;
	if(try_protocol(tag,loc,&gEfiPartitionInfoProtocolGuid))return loc;
	if(try_protocol(tag,loc,&gEfiSimpleFileSystemProtocolGuid))return loc;
	if(try_protocol(tag,loc,&gEfiBlockIoProtocolGuid))return loc;
	if(loc)FreePool(loc);
	return NULL;
}

static bool locate_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,locate_dest*);
	return d&&x&&AsciiStrCmp(x->tag,(char*)d)==0;
}

static locate_dest*get_locate(const char*tag){
	list*l;
	if(!tag||!*tag)return NULL;
	if(!locate_cache)init_default();
	if((l=list_search_one(locate_cache,locate_cmp,(void*)tag)))
		return LIST_DATA(l,locate_dest*);
	if(confd_get_type_base(BASE,tag)!=TYPE_KEY)
		return NULL;
	locate_dest*loc=load_locate(tag);
	if(loc)list_obj_add_new(&locate_cache,loc);
	return loc;
}

EFI_HANDLE*locate_get_handle_by_tag(const char*tag){
	locate_dest*loc=get_locate(tag);
	return loc?loc->file_hand:NULL;
}

char*locate_find_name(char*buf,size_t len){
	CHAR8 name[255];
	ZeroMem(buf,len);
	for(int i=0;i<4096;i++){
		ZeroMem(name,sizeof(name));
		AsciiSPrint(name,sizeof(name),"auto-%d",i);
		if(confd_get_type_base("locates",name)==TYPE_KEY)continue;
		AsciiStrCpyS(buf,len-1,name);
		return buf;
	}
	tlog_warn("no available locate name found");
	return NULL;
}

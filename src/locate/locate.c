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

static locate_dest*load_locate(const char*tag){
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	CHAR16*dpt=NULL;
	char dpx[PATH_MAX];
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	locate_dest*loc=AllocatePool(sizeof(locate_dest));
	if(!loc)return NULL;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiPartitionInfoProtocolGuid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"locate partition info protocol failed: %s",
			efi_status_to_string(st)
		);
		goto done;
	}
	for(UINTN i=0;i<cnt;i++){
		init_locate(loc,tag,hands[i]);
		for(UINTN s=0;locate_matches[s];s++){
			switch(locate_matches[s](loc)){
				case MATCH_NONE:
				case MATCH_SKIP:
				case MATCH_SUCCESS:continue;
				case MATCH_INVALID:goto done;
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
		return loc;
		match_fail:;
	}
	done:
	if(loc)FreePool(loc);
	return NULL;

}

locate_dest*get_locate(const char*tag){
	list*l;
	if(!tag||!*tag)return NULL;
	if(!locate_cache)init_default();
	if((l=list_first(locate_cache)))do{
		LIST_DATA_DECLARE(d,l,locate_dest*);
		if(!d)continue;
		if(AsciiStrCmp(d->tag,tag)==0)return d;
	}while((l=l->next));
	if(confd_get_type_base(BASE,tag)!=TYPE_KEY)
		return NULL;
	locate_dest*loc=load_locate(tag);
	if(loc)list_obj_add_new(&locate_cache,loc);
	return loc;
}

static locate_dest*init_locate_ret(locate_ret*ret,const char*file){
	locate_dest*loc;
	char buf[PATH_MAX],*tag=NULL,*path=NULL;
	if(!ret||!file||!*file)return NULL;
	ZeroMem(ret,sizeof(locate_ret));
	ZeroMem(buf,sizeof(buf));
	AsciiStrCpyS(buf,sizeof(buf)-1,file);
	ret->type=LOCATE_FILE;
	if(buf[0]=='@'){
		tag=buf+1;
		char*d=strchr(buf,':');
		if(d)*d=0,path=d+1;
	}else if(buf[0]=='#'){
		tag=buf+1;
		tlog_info("tag %s",tag);
		ret->type=LOCATE_BLOCK;
	}else path=buf,tag="default";
	AsciiStrCpyS(
		ret->tag,
		sizeof(ret->tag)-1,
		tag
	);
	if(path)AsciiStrCpyS(
		ret->path,
		sizeof(ret->path)-1,
		path
	);
	if(!(loc=get_locate(tag)))return NULL;
	switch(ret->type){
		case LOCATE_FILE:
			ret->hand=loc->file_hand;
			ret->fs=loc->file_proto;
			ret->root=loc->root;
		break;
		case LOCATE_BLOCK:
			ret->block=loc->block_proto;
		break;
		default:;
	}
	ret->part=loc->part_proto;
	return loc;
}

static bool locate_open_file(locate_dest*loc,locate_ret*ret){
	EFI_STATUS st;
	CHAR16 xp[PATH_MAX];
	if(!ret->path[0]){
		tlog_warn("no path specified");
		return false;
	}
	if(!loc->root){
		tlog_warn(
			"locate '%s' have no file system",
			ret->tag
		);
		return false;
	}
	ZeroMem(xp,sizeof(xp));
	AsciiStrToUnicodeStrS(
		ret->path,xp,
		sizeof(xp)/sizeof(CHAR16)
	);
	st=loc->root->Open(
		loc->root,&ret->file,xp,
		EFI_FILE_MODE_READ,0
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"open file '%s' failed: %s",
			ret->path,efi_status_to_string(st)
		);
		return false;
	}
	if(!ret->file){
		tlog_error(
			"open file '%s' failed",
			ret->path
		);
		return false;
	}
	if(!(ret->device=FileDevicePath(loc->file_hand,xp))){
		tlog_error(
			"get file '%s' device path failed",
			ret->path
		);
		return false;
	}
	return true;
}

static bool locate_open_block(locate_dest*loc,locate_ret*ret){
	if(!(ret->device=DevicePathFromHandle(loc->file_hand))){
		tlog_error(
			"get block '%s' device path failed",
			ret->tag
		);
		return false;
	}
	return true;
}

bool boot_locate(locate_ret*ret,const char*file){
	locate_dest*loc=init_locate_ret(ret,file);
	if(loc)switch(ret->type){
		case LOCATE_FILE:return locate_open_file(loc,ret);
		case LOCATE_BLOCK:return locate_open_block(loc,ret);
		default:;
	}
	return false;
}
